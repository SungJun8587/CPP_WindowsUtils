
//***************************************************************************
// ShCopyMove.cpp : Defines the entry point for the console application.
// 싱글 생산자 - 멀티 소비자(SPMC : Single-Producer Multi-Consumer) 패턴을 
// 이용한 고성능 병렬 파일 복사/이동 프로그램.
//***************************************************************************

#include "pch.h"

namespace fs = std::filesystem;

//***************************************************************************
// @brief FO_MOVE 시 소스 폴더의 참조 카운트 기반 정리를 위한 노드
// @details pendingCount는 1(자체 탐색-진행-중 토큰)로 시작하며, 파일/하위폴더를
//          발견할 때마다 +1, 각 참조가 해소될 때마다 -1 한다. 0이 되는 순간
//          (탐색도 끝났고, 하위 파일/폴더도 모두 처리된 시점) 자기 자신을 삭제하고
//          부모 노드에도 동일하게 전파한다.
//***************************************************************************
struct DirNode {
	_tstring          path;					// 삭제 대상 소스 폴더 경로 (트레일링 백슬래시 없음)
	DirNode* parent;						// 상위 폴더 노드 (루트면 nullptr)
	std::atomic<int>  pendingCount{ 1 };	// 1 = 자체 탐색 진행 중 토큰
	bool              isRoot;				// 루트 폴더는 삭제 대상에서 제외

	DirNode(_tstring p, DirNode* par, bool root)
		: path(std::move(p)), parent(par), isRoot(root) {
	}
};

//***************************************************************************
// @brief 개별 파일 작업(복사 또는 이동)에 필요한 경로 및 명령 정보를 담는 구조체
//***************************************************************************
struct FileTask {
	_tstring srcPath;				// 원본 파일 전체 경로
	_tstring destPath;				// 대상 파일 전체 경로
	int nFunc;						// 작업 종류(FO_COPY 또는 FO_MOVE)
	DirNode* dirNode = nullptr;		// FO_MOVE일 때만 사용 (파일 처리 완료 시 참조 해제)
};

//***************************************************************************
// @brief 생산자(Producer)와 소비자(Consumer) 스레드 간 안전한 데이터 교환 및 
//        상태 공유를 위한 컨텍스트 구조체
//***************************************************************************
struct FileProcessContext {
	// 파일 작업 태스크 큐 (락/CV/producerDone 플래그를 내부에서 처리)
	// 무제한 성장 방지를 위해 최대 10000개로 백프레셔 설정 (필요 시 조정)
	CChunkedBlockingQueue<struct FileTask> taskQueue{ 10000 };

	std::atomic<bool>   allSuccess{ true };					// 모든 파일 작업이 성공했는지 여부 플래그
	std::atomic<size_t> fileSuccessCount{ 0 };				// 성공적으로 복사/이동된 파일 수
	std::atomic<size_t> fileFailCount{ 0 };					// 실패한 파일 수
	std::atomic<size_t> folderCount{ 0 };					// 매칭 파일이 하나라도 있었던(복사/이동 대상이 된) 소스 폴더 수
	std::atomic<size_t> deletedFolderCount{ 0 };			// 실제로 삭제된 빈 소스 폴더 개수 (FO_MOVE 전용)

	// 대상(Destination) 측 폴더 생성 집계용. 여러 컨슈머 스레드가 동시에
	// 같은 대상 폴더를 만들려고 경쟁할 수 있으므로 뮤텍스로 보호.
	std::mutex                    createdFoldersMutex;
	std::unordered_set<_tstring>  createdFolders;			// 이미 생성(확인)된 대상 폴더 경로 집합(중복 생성/중복 집계 방지 겸 캐시)
	std::atomic<size_t>           createdFolderCount{ 0 };	// 실제로 새로 생성된 대상 폴더 수
};

//***************************************************************************
// @brief DirNode 참조를 하나 해소한다. 마지막 참조라면 해당 폴더를 삭제하고
//        부모로 전파한다. (여러 컨슈머 스레드에서 동시 호출되어도 안전)
// @param ctx 삭제 성공 시 deletedFolderCount 집계를 위한 공유 컨텍스트
//***************************************************************************
void ReleaseDirNode(DirNode* node, FileProcessContext& ctx)
{
	while( node != nullptr )
	{
		int prev = node->pendingCount.fetch_sub(1, std::memory_order_acq_rel);
		if( prev != 1 )
			return; // 아직 남은 참조가 있음(다른 파일/하위폴더 처리 중)

		// prev == 1 : 이 노드에 대한 마지막 참조 -> 정리할 차례
		DirNode* parent = node->parent;

		if( !node->isRoot )
		{
			// 빈 폴더가 아니면(이동 실패 잔여 파일 등) RemoveDirectory가 알아서 실패하고 무시됨
			if( RemoveDirectory(node->path.c_str()) )
			{
				ctx.deletedFolderCount.fetch_add(1, std::memory_order_relaxed);
			}
		}

		delete node;
		node = parent; // 상위 폴더로 전파 (연쇄 삭제)
	}
}

//***************************************************************************
// @brief 대상 폴더가 없으면 생성하고, 실제로 새로 생성한 세그먼트 수만큼
//        ctx.createdFolderCount를 증가시킨다. 여러 컨슈머 스레드가 동시에
//        같은 경로를 생성하려 해도 정확히 1번만 카운트된다.
// @param destFolder 존재를 보장해야 할 대상 폴더 경로 (파일이 아닌 폴더)
// @param ctx 공유 컨텍스트
// @return 성공 시 true, 생성 실패 시 false
// @note 대부분의 호출(이미 생성된 폴더)은 락 없이 fs::exists()만으로 빠르게
//       반환되며, 실제 생성이 필요한 드문 경우에만 락을 잡는다.
//***************************************************************************
bool EnsureDestFolder(const fs::path& destFolder, FileProcessContext& ctx)
{
	if( destFolder.empty() )
		return true;

	// 빠른 경로: 락 없이 존재 확인만으로 대부분의 호출을 끝냄
	std::error_code existsEc;
	if( fs::exists(destFolder, existsEc) )
		return true;

	std::lock_guard<std::mutex> lock(ctx.createdFoldersMutex);

	// 경로를 상위 -> 하위 순으로 순회하며, 아직 만들어지지 않은 세그먼트만 생성
	fs::path current;
	for( const auto& part : destFolder )
	{
		current /= part;

		// 이미 이번 실행 중 생성/확인 완료된 세그먼트는 스킵 (캐시 히트)
		if( ctx.createdFolders.find(current.native()) != ctx.createdFolders.end() )
			continue;

		std::error_code ec;
		if( fs::exists(current, ec) )
		{
			ctx.createdFolders.insert(current.native());
			continue;
		}

		if( !fs::create_directory(current, ec) )
		{
			// 다른 원인(권한 등)으로 실패했는지 최종 확인
			if( !fs::exists(current, ec) )
				return false;
		}

		ctx.createdFolders.insert(current.native());
		ctx.createdFolderCount.fetch_add(1, std::memory_order_relaxed);
	}

	return true;
}

//***************************************************************************
// @brief 단일 파일의 복사 또는 이동 작업을 처리하는 핵심 함수
// @param srcPath 원본 파일 경로
// @param destPath 대상 파일 경로
// @param nFunc 작업 종류 (FO_COPY 또는 FO_MOVE)
// @param ctx 대상 폴더 생성 집계를 위한 공유 컨텍스트
// @return 작업 성공 시 true, 실패 시 false
// @note
//   - FO_MOVE는 우선 fs::rename()을 시도한다. 같은 볼륨 내 이동이면
//     원자적(atomic)으로 처리되어 훨씬 빠르고 중간 상태가 남지 않는다.
//   - rename()이 실패하는 대표적 케이스(다른 드라이브/볼륨 간 이동 등)에는
//     copy_file() + remove()로 폴백한다. 이때 remove()의 성공 여부도
//     반드시 확인하여, 복사는 됐지만 원본 삭제가 실패한 상태를 실패로 보고한다.
//   - 실패 시 원인 파악을 위해 예외 메시지와 대상 경로를 stderr로 남긴다.
//***************************************************************************
bool ProcessSingleFile(const _tstring& srcPath, const _tstring& destPath, int nFunc, FileProcessContext& ctx) {
	try {
		fs::path src(srcPath);
		fs::path dest(destPath);

		// 대상 디렉토리가 존재하지 않으면 자동으로 생성 (실제 생성 수는 ctx에 집계됨)
		if( dest.has_parent_path() )
		{
			if( !EnsureDestFolder(dest.parent_path(), ctx) )
			{
				_ftprintf(stderr, _T("[오류] 대상 폴더 생성 실패: %s\n"), dest.parent_path().c_str());
				return false;
			}
		}

		if( nFunc == FO_COPY ) {
			fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
		}
		else if( nFunc == FO_MOVE ) {
			std::error_code ec;
			fs::rename(src, dest, ec);

			if( ec )
			{
				// rename 실패(예: 서로 다른 볼륨/드라이브 간 이동) -> copy + remove로 폴백
				fs::copy_file(src, dest, fs::copy_options::overwrite_existing);

				if( !fs::remove(src, ec) || ec )
				{
					// 복사는 성공했으나 원본 삭제가 실패한 경우: 중복 파일이 남으므로 실패로 취급
					_ftprintf(stderr, _T("[MOVE 경고] 원본 삭제 실패: %s (오류: %hs)\n"),
						src.c_str(), ec.message().c_str());
					return false;
				}
			}
		}
		return true;
	}
	catch( const std::exception& e ) {
		_ftprintf(stderr, _T("[오류] 파일 처리 실패: %s -> %s (사유: %hs)\n"),
			srcPath.c_str(), destPath.c_str(), e.what());
		return false;
	}
}

//***************************************************************************
// @brief 지정한 소스 폴더를 재귀적으로 탐색하며 조건에 맞는 파일을 찾아 큐에 적재하는 헬퍼 함수
// @param ptszSourceFolder 탐색할 원본 폴더 경로
// @param ptszDestFolder 복사/이동될 대상 폴더 경로
// @param ShApplyFileInfo 파일 필터링 조건 정보 (확장자, 날짜 등)
//        * m_nFilterMode 의미:
//			- 0 : 필터링 없음 (전체 허용)
//          - 1 : 화이트리스트 (지정한 확장자만 허용)
//          - 2 : 블랙리스트 (지정한 확장자는 비허용/제외)
// @param nFunc 작업 종류 (FO_COPY 또는 FO_MOVE)
// @param ctx 스레드 간 공유 컨텍스트 (스레드 안전한 작업 큐와 동기화 객체 포함)
// @param parentNode FO_MOVE일 때, 이 폴더의 상위 폴더를 나타내는 참조 카운트 노드
//        (nullptr이면 최상위 호출 = 소스 루트 폴더)
//***************************************************************************
void DirectoryRecursiveSearch(const TCHAR* ptszSourceFolder, const TCHAR* ptszDestFolder,
	SH_APPLY_FILEINFO& ShApplyFileInfo, int nFunc, FileProcessContext& ctx,
	DirNode* parentNode = nullptr)
{
	BOOL		bResult = true;
	TCHAR		tszActiveFolder[DIRECTORY_STRLEN + 16];
	TCHAR		tszSourceFullPath[FULLPATH_STRLEN];
	TCHAR		tszSourceFolder[DIRECTORY_STRLEN];
	TCHAR		tszDestFullPath[FULLPATH_STRLEN];
	TCHAR		tszDestFolder[DIRECTORY_STRLEN];

	WIN32_FIND_DATA		FindData;
	HANDLE				hFindFile;

	// 이 폴더에서 매칭 파일을 큐잉했는지 여부 (folderCount 중복 집계 방지용, producer 단일 스레드 로컬 변수)
	bool bFolderCounted = false;

	// 인자 유효성 검사
	if( !ptszSourceFolder || !ptszDestFolder ) return;
	if( _tcslen(ptszSourceFolder) < 1 || _tcslen(ptszDestFolder) < 1 ) return;

	// FO_MOVE일 때만 이 폴더에 대한 참조 카운트 노드 생성 (COPY는 삭제 대상 아니므로 nullptr 유지)
	DirNode* dirNode = nullptr;
	if( nFunc == FO_MOVE )
	{
		dirNode = new DirNode(ptszSourceFolder, parentNode, parentNode == nullptr);
	}

	// 원본 경로 문자열 끝에 역슬래시(\)가 없으면 추가하여 검색 패턴 정규화
	if( ptszSourceFolder[_tcslen(ptszSourceFolder) - 1] != '/' && ptszSourceFolder[_tcslen(ptszSourceFolder) - 1] != '\\' )
	{
		_sntprintf_s(tszSourceFolder, _countof(tszSourceFolder), _TRUNCATE, _T("%s\\"), ptszSourceFolder);
		_sntprintf_s(tszActiveFolder, _countof(tszActiveFolder), _TRUNCATE, _T("%s\\*.*"), ptszSourceFolder);
	}
	else
	{
		_sntprintf_s(tszSourceFolder, _countof(tszSourceFolder), _TRUNCATE, _T("%s"), ptszSourceFolder);
		_sntprintf_s(tszActiveFolder, _countof(tszActiveFolder), _TRUNCATE, _T("%s*.*"), ptszSourceFolder);
	}

	// 대상 경로 문자열 끝에 역슬래시(\)가 없으면 추가
	if( ptszDestFolder[_tcslen(ptszDestFolder) - 1] != '/' && ptszDestFolder[_tcslen(ptszDestFolder) - 1] != '\\' )
		_sntprintf_s(tszDestFolder, _countof(tszDestFolder), _TRUNCATE, _T("%s\\"), ptszDestFolder);
	else
		_sntprintf_s(tszDestFolder, _countof(tszDestFolder), _TRUNCATE, _T("%s"), ptszDestFolder);

	// 파일 및 디렉토리 검색 핸들 열기
	hFindFile = FindFirstFile(tszActiveFolder, &FindData);

	// FindFirstFile 실패 시(핸들 무효) 아래 루프/FindClose를 모두 건너뛴다.
	if( INVALID_HANDLE_VALUE != hFindFile )
	{
		while( bResult )
		{
			if( FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				// 현재 디렉토리('.')와 상위 디렉토리('..')는 제외하고 하위 폴더 탐색
				// 재귀 무한 루프 방지를 위해 심볼릭 링크/정션(REPARSE POINT)은 하위 탐색에서 제외
				if( _tcscmp(FindData.cFileName, _T(".")) != 0 && _tcscmp(FindData.cFileName, _T("..")) != 0
					&& !(FindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) )
				{
					_sntprintf_s(tszSourceFullPath, _countof(tszSourceFullPath), _TRUNCATE, _T("%s%s"), tszSourceFolder, FindData.cFileName);
					_sntprintf_s(tszDestFullPath, _countof(tszDestFullPath), _TRUNCATE, _T("%s%s"), tszDestFolder, FindData.cFileName);

					// 하위 폴더에 대한 참조를 미리 등록한 뒤 재귀 호출
					if( dirNode ) dirNode->pendingCount.fetch_add(1, std::memory_order_relaxed);
					DirectoryRecursiveSearch(tszSourceFullPath, tszDestFullPath, ShApplyFileInfo, nFunc, ctx, dirNode);
				}
			}
			else
			{
				// 파일 경로 조합
				_sntprintf_s(tszSourceFullPath, _countof(tszSourceFullPath), _TRUNCATE, _T("%s%s"), tszSourceFolder, FindData.cFileName);
				_sntprintf_s(tszDestFullPath, _countof(tszDestFullPath), _TRUNCATE, _T("%s%s"), tszDestFolder, FindData.cFileName);

				// 필터 조건(모드 0, 1, 2)에 부합하는 파일일 경우에만 처리 대상을 큐에 적재
				if( IsAbleFile(tszSourceFullPath, ShApplyFileInfo) )
				{
					// 이 폴더에서 처음 매칭된 파일이면 folderCount 1회 증가
					if( !bFolderCounted )
					{
						ctx.folderCount.fetch_add(1, std::memory_order_relaxed);
						bFolderCounted = true;
					}

					// 이 파일이 처리 완료될 때까지 이 폴더가 삭제되지 않도록 참조 등록
					if( dirNode ) dirNode->pendingCount.fetch_add(1, std::memory_order_relaxed);

					// 큐에 새로운 작업을 삽입 (락/알림은 CChunkedBlockingQueue 내부에서 처리됨)
					// maxQueueSize에 도달하면 컨슈머가 소비할 때까지 여기서 블로킹됨
					ctx.taskQueue.Push({ tszSourceFullPath, tszDestFullPath, nFunc, dirNode });
				}
			}

			bResult = FindNextFile(hFindFile, &FindData);
		}

		FindClose(hFindFile);
	}

	// 이 폴더의 탐색이 완전히 끝남 -> 자체 참조(1) 해소.
	// 파일/하위폴더가 하나도 없었다면 여기서 즉시 pendingCount가 0이 되어
	// 이 스레드(생산자)에서 바로 삭제 + 상위 폴더로 전파됨.
	if( dirNode ) ReleaseDirNode(dirNode, ctx);
}

//***************************************************************************
// @brief [1] 생산자(Producer) 스레드 함수: 디렉토리 트리를 탐색하며 파일 작업을 큐에 공급
// @param srcPath 탐색 시작 원본 경로
// @param destPath 복사/이동 대상 경로
// @param ShApplyFileInfo 파일 필터링 조건 정보
// @param nFunc 작업 종류 (FO_COPY 또는 FO_MOVE)
// @param ctx 스레드 간 공유 컨텍스트
//***************************************************************************
void ProducerFunc(const _tstring& srcPath, const _tstring& destPath,
	SH_APPLY_FILEINFO& ShApplyFileInfo, int nFunc, FileProcessContext& ctx)
{
	DirectoryRecursiveSearch(srcPath.c_str(), destPath.c_str(), ShApplyFileInfo, nFunc, ctx);

	// 디렉토리 탐색 완료 신호 (내부적으로 대기 중인 모든 소비자 스레드를 깨움)
	ctx.taskQueue.SetProducerDone();
}

//***************************************************************************
// @brief [2] 소비자(Consumer) 스레드 함수: 큐에서 작업을 청크 단위로 꺼내어 파일 복사/이동을 병렬 처리
// @param ctx 스레드 간 공유 컨텍스트
//***************************************************************************
void ConsumerFunc(FileProcessContext& ctx)
{
	constexpr size_t CHUNK_SIZE = 64; // 한 번에 가져올 최대 작업 개수

	while( true )
	{
		CQueue<FileTask> chunk;

		// false: Stop() 호출 시(현재 미사용) 또는 큐가 비고 생산 완료된 경우 -> 루프 탈출
		if( !ctx.taskQueue.PopChunk(chunk, CHUNK_SIZE) )
			break;

		// 가짜 깨어남으로 chunk가 비어있을 수 있음 -> while이 자연히 스킵되고 다음 반복으로
		while( !chunk.empty() )
		{
			FileTask task = std::move(chunk.front());
			chunk.pop();

			bool success = ProcessSingleFile(task.srcPath, task.destPath, task.nFunc, ctx);
			if( success )
			{
				ctx.fileSuccessCount.fetch_add(1, std::memory_order_relaxed);
			}
			else
			{
				ctx.allSuccess.store(false);
				ctx.fileFailCount.fetch_add(1, std::memory_order_relaxed);
			}

			// 이 파일에 대한 참조를 해제. 폴더 내 마지막 파일이었다면
			// (그리고 탐색도 이미 끝났다면) 즉시 폴더 삭제 + 상위로 전파됨.
			if( task.dirNode ) ReleaseDirNode(task.dirNode, ctx);
		}
	}
}

//***************************************************************************
// @brief 프로그램 진입점 (Main 함수)
// @param argc 전달된 인자 개수
// @param argv 전달된 인자 배열
// @return 성공 시 0, 오류 시 1
// @note 
//	[ShCopyMove.exe 인자(Arguments) 상세 설명]
// 
//	사용 형식:
//	ShCopyMove.exe [모드] [원본경로] [대상경로] [필터적용여부] [확장자필터] [시작일] [종료일]
//
//	1. [모드] (argv[1])
//    - C : 파일을 지정된 경로로 복사합니다 (FO_COPY).
//    - M : 파일을 지정된 경로로 이동합니다 (FO_MOVE).
//          (이동 후 소스 하위 폴더에 파일이 남지 않으면 해당 폴더를 자동 삭제합니다.
//           단, 인자로 지정한 최상위 소스 폴더 자체는 삭제하지 않습니다.)
//
//	2. [원본경로] (argv[2])
//    - 복사 또는 이동 작업을 수행할 원본 폴더(또는 파일)의 전체 경로입니다.
//    - 경로 내 공백이 포함된 경우 공백 대신 ";32;"를 사용해야 합니다. (예: "C:\My;32;Folder")
//
//	3. [대상경로] (argv[3])
//    - 파일이 복사되거나 이동되어 저장될 목적지 폴더의 전체 경로입니다.
//    - 대상 디렉토리가 존재하지 않을 경우 프로그램이 자동으로 생성합니다.
//
//	4. [필터적용여부] (argv[4])
//    - 0 : 필터링 없음 (모든 파일 대상)
//    - 1 : 화이트리스트 (지정한 확장자만 허용)
//    - 2 : 블랙리스트 (지정한 확장자는 비허용/제외)
//
//	5. [확장자필터] (argv[5])
//    - 필터링할 파일 확장자를 지정합니다. (예: "txt")
//    - 여러 개의 확장자를 동시에 지정할 경우 세미콜론(;)으로 구분합니다. (예: "txt;log;csv")
//    - 필터링을 사용하지 않는 경우 빈 문자열("")을 입력합니다.
//
//	6. [시작일] (argv[6])
//    - 파일 수정일(Modified Date)을 기준으로 필터링할 시작 날짜입니다.
//    - YYYYMMDD 형식으로 입력합니다. (예: "20260101" = 2026년 1월 1일 이후 수정된 파일)
//
//	7. [종료일] (argv[7])
//    - 파일 수정일(Modified Date)을 기준으로 필터링할 종료 날짜입니다.
//    - YYYYMMDD 형식으로 입력합니다. (예: "20260807" = 2026년 8월 7일 이전 수정된 파일)
//
//	8. [소비자 스레드 수] (argv[8], 선택)
//    - 파일 복사/이동을 병렬로 처리할 소비자(Consumer) 스레드 개수입니다.
//    - 값을 지정하지 않거나 0을 입력하면 시스템 프로세서 코어 개수
//      (SYSTEM::CoreCount())로 자동 설정됩니다.
//    - 원본/대상 저장장치의 종류(SSD/HDD/네트워크 드라이브 등)에 따라
//      최적 스레드 수가 다르므로 필요 시 직접 조정할 수 있습니다.
//
//	[프로그램 실행 예제 및 사용법 안내]
//	1. 파일 복사 예제 (필터링 적용):
//		ShCopyMove.exe C "C:\Source" "D:\Dest" 1 "txt" "20260101" "20260807"
//
//  2. 파일 이동 예제 (필터링 미적용):
//		ShCopyMove.exe M "C:\Work" "D:\Archive" 0 "" "" ""
// 
//  3. 공백 경로 처리 주의사항:
//		경로 내에 공백이 포함된 경우, 공백 대신 ";32;" 문자열을 사용해야 합니다.
//		예: ShCopyMove.exe C "C:\My;32;Documents" "D:\Backup" 0 "" "" ""
//
//  4. 소비자 스레드 수 직접 지정 예제 (8개 스레드로 처리):
//		ShCopyMove.exe C "C:\Source" "D:\Dest" 0 "" "" "" 8
//
//	[명령행 인수(Arguments) 설정하고, 디버깅 모드로 실행] : 인수에 문자열 깨짐 현상으로 [디버깅용] 인자 강제 오버라이드 (F5 테스트용) 설정
//	1. 비주얼 스튜디오 상단 메뉴에서 [프로젝트] -> [속성(Properties)]을 클릭합니다.(단축키: Alt + F7)
//	2. 창 왼쪽 메뉴에서[구성 속성] -> [디버깅(Debugging)]을 선택합니다.
//	3. 오른쪽 항목 중[명령 인수(Command Arguments)] 칸을 클릭합니다.
//	4. 앞서 만든 실행 예제 중 테스트할 문자열을 입력합니다.(C "C:\Source" "D:\Dest" 1 "txt" "20260101" "20260807")
//	5. 오른쪽 아래의 [적용] 버튼을 누르고 [확인]을 누릅니다.
//	6. 키보드의 F5 키를 누르거나, 상단 메뉴의 [디버그] -> [디버깅 시작]을 클릭합니다.
// 
//***************************************************************************
int main(int argc, TCHAR* argv[])
{
#ifdef	_MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	// 한글 콘솔 출력 설정
	_tsetlocale(LC_ALL, _T("Korean"));

	// ==========================================
	// [디버깅용] 인자 강제 오버라이드 (F5 테스트용)
	// ==========================================
#ifdef _DEBUG
	// const TCHAR*로 안전하게 배열을 선언합니다.
	static const TCHAR* mockArgv[] = {
		_T("ShCopyMove.exe"), // argv[0]: 프로그램 이름
		_T("M"),              // argv[1]: 작업 모드(C 또는 M)
		_T("C:\\Source"),     // argv[2]: 원본 경로
		_T("D:\\Dest"),       // argv[3]: 대상 경로
		_T("1"),              // argv[4]: 필터링 적용 여부(0/1/2 : 전체 허용/지정한 확장자만 허용/지정한 확장자는 제외)
		_T("aspx;resx"),	  // argv[5]: 확장자 필터("txt;log;csv"만 허용)
		_T("20120101"),       // argv[6]: 수정일 기준 시작일
		_T("20260807"),       // argv[7]: 수정일 기준 종료일
		_T("0")               // argv[8]: 소비자 스레드 수(0 또는 미지정 시 CoreCount() 사용)
	};

	argc = _countof(mockArgv);
	// const 포인터 배열을 main의 인자 규격인 TCHAR**로 안전하게 변환합니다.
	argv = const_cast<TCHAR**>(mockArgv);
#endif

	int		nFunc = 0;
	TCHAR	tszProgressTitle[32];
	TCHAR	tszSrcFullPath[FULLPATH_STRLEN];
	TCHAR	tszDestFullPath[FULLPATH_STRLEN];

	SH_APPLY_FILEINFO	ShApplyFileInfo;
	memset(&ShApplyFileInfo, 0, sizeof(ShApplyFileInfo));

	if( argc < 5 )
	{
		printf("알림 : 잘못된 요청입니다.");
		return 1;
	}

	// [인자 1] 작업 모드 설정 (M: 파일 이동, 그 외: 파일 복사)
	if( argc > 1 )
	{
		if( argv[1][0] == 'M' )
		{
			_tcsncpy_s(tszProgressTitle, _countof(tszProgressTitle), _T("파일 이동"), _TRUNCATE);
			nFunc = FO_MOVE;
		}
		else
		{
			_tcsncpy_s(tszProgressTitle, _countof(tszProgressTitle), _T("파일 복사"), _TRUNCATE);
			nFunc = FO_COPY;
		}
	}

	// [인자 2] 원본 디렉토리/파일 전체 경로 설정
	if( argc > 2 ) _tcsncpy_s(tszSrcFullPath, _countof(tszSrcFullPath), argv[2], _TRUNCATE);

	// [인자 3] 대상 디렉토리/파일 전체 경로 설정
	if( argc > 3 ) _tcsncpy_s(tszDestFullPath, _countof(tszDestFullPath), argv[3], _TRUNCATE);

	// [인자 4] 필터 모드 설정 (0: 미적용, 1: 화이트리스트, 2: 블랙리스트)
	if( argc > 4 )
	{
		ShApplyFileInfo.m_nFilterMode = _ttoi(argv[4]); // 문자열을 숫자로 변환
	}

	// [인자 5] 필터링할 대상 확장자 설정 (예: txt, log 등)
	if( argc > 5 ) _tcsncpy_s(ShApplyFileInfo.m_tszApplyExt, _countof(ShApplyFileInfo.m_tszApplyExt), argv[5], _TRUNCATE);

	// [인자 6] 파일 수정일 기준 시작일 조건 설정
	if( argc > 6 ) _tcsncpy_s(ShApplyFileInfo.m_tszModifyStDate, _countof(ShApplyFileInfo.m_tszModifyStDate), argv[6], _TRUNCATE);

	// [인자 7] 파일 수정일 기준 종료일 조건 설정
	if( argc > 7 ) _tcsncpy_s(ShApplyFileInfo.m_tszModifyEdDate, _countof(ShApplyFileInfo.m_tszModifyEdDate), argv[7], _TRUNCATE);

	// [인자 8] 소비자 스레드 수 설정(선택, 미지정 또는 0이면 이후 CoreCount()로 대체)
	size_t nRequestedThreads = 0;
	if( argc > 8 ) nRequestedThreads = static_cast<size_t>(_ttoi(argv[8]));

	// 공백 치환 처리
	_tstring strSrc = replaceAll(tszSrcFullPath, _T(";32;"), _T(" "));
	_tstring strDest = replaceAll(tszDestFullPath, _T(";32;"), _T(" "));

	FileProcessContext ctx;
	CThreadManager threadManager;

	// 소비자 스레드 개수 설정: argv[8]로 지정된 값이 있으면 그 값을 사용하고,
	// 값이 0이거나 인자 자체가 없으면 시스템 프로세서 코어 개수로 대체한다.
	size_t numThreads = (nRequestedThreads != 0)
		? nRequestedThreads
		: static_cast<size_t>(SYSTEM::CoreCount());

	_tprintf(_T("\n싱글 생산자 - 멀티 소비자 병렬 처리를 시작합니다 (소비자 스레드 수: %zu)...\n"), numThreads);

	// 소비자(Consumer) 스레드 풀 생성 및 실행
	for( size_t t = 0; t < numThreads; ++t )
	{
		threadManager.CreateThread([&ctx]() {
			ConsumerFunc(ctx);
			});
	}

	// 싱글 생산자(Producer) 실행(메인 스레드에서 디렉토리 탐색 및 작업 공급 전담)
	ProducerFunc(strSrc, strDest, ShApplyFileInfo, nFunc, ctx);

	// 모든 소비자 스레드가 잔여 태스크를 모두 처리하고 종료될 때까지 대기
	threadManager.JoinThreads();

	// 전체 작업 결과 출력
	if( ctx.allSuccess.load() )
		_tprintf(_T("알림 : 모든 작업이 성공적으로 완료되었습니다.\n\n"));
	else
		_tprintf(_T("알림 : 일부 파일 작업 중 오류가 발생했습니다.\n\n"));

	_tprintf(_T("- %s된 폴더 수 (소스 기준) : %s\n"), (nFunc == FO_MOVE) ? _T("이동") : _T("복사"), addCommas(ctx.folderCount.load()).c_str());
	if( nFunc == FO_MOVE )
		_tprintf(_T("- 삭제된 폴더 수 (소스 기준) : %s\n"), addCommas(ctx.deletedFolderCount.load()).c_str());

	_tprintf(_T("- 실제 생성된 대상 폴더 수 : %s\n"), addCommas(ctx.createdFolderCount.load()).c_str());
	_tprintf(_T("- %s된 대상 파일 수 : %s"), (nFunc == FO_MOVE) ? _T("이동") : _T("복사"), addCommas(ctx.fileSuccessCount.load()).c_str());
	if( ctx.fileFailCount.load() > 0 )
		_tprintf(_T("- (실패 %s)"), addCommas(ctx.fileFailCount.load()).c_str());
	_tprintf(_T("\n\n"));

	system("pause");

	return 0;
}