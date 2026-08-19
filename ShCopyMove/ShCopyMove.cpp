//***************************************************************************
// ShCopyMove.cpp : Defines the entry point for the console application.
// 싱글 생산자 - 멀티 소비자(SPMC : Single-Producer Multi-Consumer) 패턴을
// 이용한 고성능 병렬 파일 복사/이동 프로그램.
// std::filesystem 기반으로 동작하여 Windows/Linux/macOS에서 동일하게 빌드된다.
//***************************************************************************

#include "pch.h"

#include <shared_mutex>	// EnsureDestFolder 캐시 조회를 shared_lock으로 최적화하기 위해 사용
#include <filesystem>
#include <string>
#include <cstdio>
#include <cstdlib>

// Log.h를 수정하지 않고, ShCopyMove.cpp 내에서만 LOG_INFO의 시간을 출력하지 않도록 재정의
#undef LOG_INFO
#define LOG_INFO(...) CLogManager::Instance().Write(ELOG_TYPE::LOG_TYPE_INFO, false, __VA_ARGS__)

#ifdef _WIN32
#include <windows.h>	// 콘솔 출력 코드페이지(UTF-8) 설정에만 사용
#endif

namespace fs = std::filesystem;

// FO_COPY/FO_MOVE: 원래 Windows <shellapi.h>의 SHFileOperation 상수였으나, 이 파일은
// std::filesystem으로 직접 구현하여 SHFileOperation을 쓰지 않으므로 실제 값은 의미가 없고
// 내부적으로 복사/이동 모드를 구분하는 용도로만 쓰인다. pch.h 경로로 이미 <shellapi.h> 등에서
// 정의된 환경(Windows)이라면 그 값을 그대로 쓰고, 아니라면 여기서 정의한다.
#ifndef FO_MOVE
#define FO_MOVE 1
#endif
#ifndef FO_COPY
#define FO_COPY 2
#endif

//***************************************************************************
// @brief std::error_code의 메시지를 현재 프로젝트의 문자셋(_UNICODE 여부)에 맞는
//        _tstring 타입으로 안전하게 변환하는 헬퍼 함수
// @param ec 변환할 std::error_code 객체
// @return _UNICODE 환경에서는 std::wstring, 멀티바이트 환경에서는 std::string으로 변환된 문자열
//***************************************************************************
inline _tstring ErrorToString(const std::error_code& ec)
{
#ifdef _UNICODE
	std::string narrow = ec.message();
	return _tstring(narrow.begin(), narrow.end());
#else
	return ec.message();
#endif
}

//***************************************************************************
// @brief std::exception의 메시지를 현재 프로젝트의 문자셋(_UNICODE 여부)에 맞는
//        _tstring 타입으로 안전하게 변환하는 헬퍼 함수
// @param e 변환할 std::exception 객체
// @return _UNICODE 환경에서는 std::wstring, 멀티바이트 환경에서는 std::string으로 변환된 문자열
//***************************************************************************
inline _tstring ExceptionToString(const std::exception& e)
{
#ifdef _UNICODE
	std::string narrow = e.what();
	return _tstring(narrow.begin(), narrow.end());
#else
	return e.what();
#endif
}

//***************************************************************************
// @brief FO_MOVE 시 소스 폴더의 참조 카운트 기반 정리를 위한 노드
// @details pendingCount는 1(자체 탐색-진행-중 토큰)로 시작하며, 파일/하위폴더를
//         발견할 때마다 +1, 각 참조가 해소될 때마다 -1 한다. 0이 되는 순간
//         (탐색도 끝났고, 하위 파일/폴더도 모두 처리된 시점) 자기 자신을 삭제하고
//         부모 노드에도 동일하게 전파한다.
//***************************************************************************
struct DirNode {
	fs::path             path;				// 삭제 대상 소스 폴더 경로
	DirNode* parent;						// 상위 폴더 노드 (루트면 nullptr)
	std::atomic<int>     pendingCount{ 1 };	// 1 = 자체 탐색 진행 중 토큰
	bool                 isRoot;				// 루트 폴더는 삭제 대상에서 제외

	DirNode(fs::path p, DirNode* par, bool root)
		: path(std::move(p)), parent(par), isRoot(root) {
	}
};

//***************************************************************************
// @brief 개별 파일 작업(복사 또는 이동)에 필요한 경로 및 명령 정보를 담는 구조체
//***************************************************************************
struct FileTask {
	fs::path srcPath;					// 원본 파일 전체 경로
	fs::path destPath;					// 대상 파일 전체 경로
	int nFunc;							// 작업 종류(FO_COPY 또는 FO_MOVE)
	DirNode* dirNode = nullptr;		// FO_MOVE일 때만 사용 (파일 처리 완료 시 참조 해제)
};

//***************************************************************************
// @brief 생산자(Producer)와 소비자(Consumer) 스레드 간 안전한 데이터 교환 및
//         상태 공유를 위한 컨텍스트 구조체
//***************************************************************************
struct FileProcessContext {
	// 파일 작업 태스크 큐 (락/CV/producerDone 플래그를 내부에서 처리)
	// 무제한 성장 방지를 위해 최대 10000개로 백프레셔 설정 (필요 시 조정)
	CChunkedBlockingQueue<struct FileTask> taskQueue{ 10000 };

	std::atomic<bool>    allSuccess{ true };					// 모든 파일 작업이 성공했는지 여부 플래그
	std::atomic<size_t> fileSuccessCount{ 0 };				// 성공적으로 복사/이동된 파일 수
	std::atomic<size_t> fileFailCount{ 0 };					// 실패한 파일 수
	std::atomic<size_t> folderCount{ 0 };					// 매칭 파일이 하나라도 있었던(복사/이동 대상이 된) 소스 폴더 수
	std::atomic<size_t> deletedFolderCount{ 0 };			// 실제로 삭제된 빈 소스 폴더 개수 (FO_MOVE 전용)
	std::atomic<size_t> scanErrorCount{ 0 };				// 디렉터리 열람 실패로 일부/전체 항목을 건너뛴 폴더 수

	// 대상(Destination) 측 폴더 생성 집계용. 여러 컨슈머 스레드가 동시에
	// 같은 대상 폴더를 만들려고 경쟁할 수 있으므로 shared_mutex로 보호.
	// 조회(읽기)가 압도적으로 많고 실제 생성(쓰기)은 드물게 발생하므로,
	// 읽기 시에는 shared_lock으로 여러 스레드가 동시에 캐시를 조회할 수 있게 한다.
	// 캐시 키는 fs::path::string_type(native 인코딩) — std::hash가 표준으로 지원된다.
	std::shared_mutex                         createdFoldersMutex;
	std::unordered_set<fs::path::string_type>     createdFolders;		// 이미 생성(확인)된 대상 폴더 경로 집합(중복 생성/중복 집계 방지 겸 캐시)
	std::atomic<size_t>                         createdFolderCount{ 0 };	// 실제로 새로 생성된 대상 폴더 수
};

//***************************************************************************
// @brief DirNode 참조를 하나 해소한다. 마지막 참조라면 해당 폴더를 삭제하고
//         부모로 전파한다. (여러 컨슈머 스레드에서 동시 호출되어도 안전)
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
			// 빈 폴더가 아니면(이동 실패 잔여 파일 등) remove가 알아서 실패하고 무시됨
			std::error_code ec;
			if( fs::remove(node->path, ec) )
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
//         ctx.createdFolderCount를 증가시킨다. 여러 컨슈머 스레드가 동시에
//         같은 경로를 생성하려 해도 정확히 1번만 카운트된다.
// @param destFolder 존재를 보장해야 할 대상 폴더 경로 (파일이 아닌 폴더)
// @param ctx 공유 컨텍스트
// @return 성공 시 true, 생성 실패 시 false
// @note 대부분의 호출(이미 생성된 폴더)은 shared_lock 상태의 캐시 조회만으로
//        (파일시스템 syscall 없이) 빠르게 반환되며, 실제 생성이 필요한 드문
//        경우에만 unique_lock을 잡는다.
//***************************************************************************
bool EnsureDestFolder(const fs::path& destFolder, FileProcessContext& ctx)
{
	if( destFolder.empty() )
		return true;

	const fs::path::string_type key = destFolder.native();

	// 빠른 경로: 캐시(createdFolders)를 shared_lock으로 먼저 조회한다.
	// 이미 생성이 확인된 폴더라면 파일시스템 syscall 없이 메모리 조회만으로 끝난다.
	{
		std::shared_lock<std::shared_mutex> readLock(ctx.createdFoldersMutex);
		if( ctx.createdFolders.find(key) != ctx.createdFolders.end() )
			return true;
	}

	std::unique_lock<std::shared_mutex> lock(ctx.createdFoldersMutex);

	// 더블 체크: 위 shared_lock 해제 후 unique_lock 획득 사이에 다른
	// 스레드가 이미 같은 경로를 캐시에 넣었을 수 있으므로 재확인한다.
	if( ctx.createdFolders.find(key) != ctx.createdFolders.end() )
		return true;

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
// @param src 원본 파일 경로
// @param dest 대상 파일 경로
// @param nFunc 작업 종류 (FO_COPY 또는 FO_MOVE)
// @param ctx 대상 폴더 생성 집계를 위한 공유 컨텍스트
// @return 작업 성공 시 true, 실패 시 false
// @note
//   - FO_MOVE는 우선 fs::rename()을 시도한다. 같은 볼륨 내 이동이면
//     원자적(atomic)으로 처리되어 훨씬 빠르고 중간 상태가 남지 않는다.
//   - rename()이 실패하는 대표적 케이스(다른 드라이브/볼륨 간 이동 등)에는
//     copy_file() + remove()로 폴백한다. 이때 remove()의 성공 여부도
//     반드시 확인하여, 복사는 됐지만 원본 삭제가 실패한 상태를 실패로 보고한다.
//   - 실패 시 원인 파악을 위해 예외 메시지와 대상 경로를 로그 파일로 남긴다.
//***************************************************************************
bool ProcessSingleFile(const fs::path& src, const fs::path& dest, int nFunc, FileProcessContext& ctx) 
{
	try 
	{
		// 대상 디렉토리가 존재하지 않으면 자동으로 생성 (실제 생성 수는 ctx에 집계됨)
		if( dest.has_parent_path() )
		{
			if( !EnsureDestFolder(dest.parent_path(), ctx) )
			{
				LOG_ERROR(_T("[오류] 대상 폴더 생성 실패: %s"), dest.parent_path().native().c_str());
				return false;
			}
		}

		if( nFunc == FO_COPY ) 
		{
			fs::copy_file(src, dest, fs::copy_options::overwrite_existing);
		}
		else if( nFunc == FO_MOVE ) 
		{
			std::error_code ec;
			fs::rename(src, dest, ec);

			if( ec )
			{
				// rename 실패(예: 서로 다른 볼륨/드라이브 간 이동) -> copy + remove로 폴백
				fs::copy_file(src, dest, fs::copy_options::overwrite_existing);

				if( !fs::remove(src, ec) || ec )
				{
					// 복사는 성공했으나 원본 삭제가 실패한 경우: 중복 파일이 남으므로 실패로 취급
					LOG_WARNING(_T("[MOVE 경고] 원본 삭제 실패: %s (오류: %s)"), src.native().c_str(), ErrorToString(ec).c_str());
					return false;
				}
			}
		}
		return true;
	}
	catch( const std::exception& e ) 
	{
		LOG_ERROR(_T("[오류] 파일 처리 실패: %s -> %s (사유: %s)"), src.native().c_str(), dest.native().c_str(), ExceptionToString(e).c_str());
		return false;
	}
}

//***************************************************************************
// @brief 지정한 소스 폴더를 재귀적으로 탐색하며 조건에 맞는 파일을 찾아 큐에 적재하는 헬퍼 함수
// @param sourceFolder 탐색할 원본 폴더 경로
// @param destFolder 복사/이동될 대상 폴더 경로
// @param ShApplyFileInfo 파일 필터링 조건 정보 (확장자, 날짜 등)
//         * m_nFilterMode 의미:
//			- 0 : 필터링 없음 (전체 허용)
//          - 1 : 화이트리스트 (지정한 확장자만 허용)
//          - 2 : 블랙리스트 (지정한 확장자는 비허용/제외)
// @param nFunc 작업 종류 (FO_COPY 또는 FO_MOVE)
// @param ctx 스레드 간 공유 컨텍스트 (스레드 안전한 작업 큐와 동기화 객체 포함)
// @param parentNode FO_MOVE일 때, 이 폴더의 상위 폴더를 나타내는 참조 카운트 노드
//         (nullptr이면 최상위 호출 = 소스 루트 폴더)
// @note std::filesystem::directory_iterator는 "."/".." 항목을 애초에 반환하지
//        않으므로 별도 필터링이 필요 없다. 정션/심볼릭 링크는 재귀 무한 루프
//        방지를 위해 하위 폴더 탐색 대상에서만 제외한다(파일 자체는 그대로 처리).
//***************************************************************************
void DirectoryRecursiveSearch(const fs::path& sourceFolder, const fs::path& destFolder,
	SH_APPLY_FILEINFO& ShApplyFileInfo, int nFunc, FileProcessContext& ctx,
	DirNode* parentNode = nullptr)
{
	// 이 폴더에서 매칭 파일을 큐잉했는지 여부 (folderCount 중복 집계 방지용, producer 단일 스레드 로컬 변수)
	bool bFolderCounted = false;

	if( sourceFolder.empty() || destFolder.empty() ) return;

	// FO_MOVE일 때만 이 폴더에 대한 참조 카운트 노드 생성 (COPY는 삭제 대상 아니므로 nullptr 유지)
	DirNode* dirNode = nullptr;
	if( nFunc == FO_MOVE )
	{
		dirNode = new DirNode(sourceFolder, parentNode, parentNode == nullptr);
	}

	std::error_code dirEc;
	fs::directory_iterator it(sourceFolder, fs::directory_options::skip_permission_denied, dirEc);
	fs::directory_iterator end;

	if( dirEc )
	{
		// 폴더 자체를 열람하지 못함 (권한/경로 문제 등) — 조용히 넘기지 않고 반드시 남긴다
		LOG_WARNING(_T("[경고] 폴더 열람 실패, 건너뜀: %s (오류: %s)"), sourceFolder.native().c_str(), ErrorToString(dirEc).c_str());
		ctx.scanErrorCount.fetch_add(1, std::memory_order_relaxed);
		ctx.allSuccess.store(false);
	}
	else
	{
		while( it != end )
		{
			const fs::directory_entry& entry = *it;

			std::error_code typeEc;
			bool bIsDir = entry.is_directory(typeEc);

			if( typeEc )
			{
				// 파일/폴더 상태 조회 자체가 실패한 항목 — 예전에는 로그도 카운트도 없이
				// 그냥 통째로 건너뛰어서, 이 경로로 빠진 파일이 있어도 알 방법이 없었다.
				LOG_WARNING(_T("[경고] 상태 조회 실패로 건너뜀: %s (오류: %s)"), entry.path().native().c_str(), ErrorToString(typeEc).c_str());
				ctx.scanErrorCount.fetch_add(1, std::memory_order_relaxed);
				ctx.allSuccess.store(false);
			}
			else
			{
				if( bIsDir )
				{
					// 재귀 무한 루프 방지를 위해 심볼릭 링크/정션은 하위 탐색에서 제외
					std::error_code linkEc;
					if( !fs::is_symlink(entry.symlink_status(linkEc)) )
					{
						const fs::path& srcFullPath = entry.path();
						fs::path destFullPath = destFolder / srcFullPath.filename();

						// 하위 폴더에 대한 참조를 미리 등록한 뒤 재귀 호출
						if( dirNode ) dirNode->pendingCount.fetch_add(1, std::memory_order_relaxed);
						DirectoryRecursiveSearch(srcFullPath, destFullPath, ShApplyFileInfo, nFunc, ctx, dirNode);
					}
				}
				else
				{
					const fs::path& srcFullPath = entry.path();

					// 필터 조건(모드 0, 1, 2 + 날짜 범위)에 부합하는 파일일 경우에만 처리 대상을 큐에 적재
					if( IsAbleFile(srcFullPath, ShApplyFileInfo) )
					{
						fs::path destFullPath = destFolder / srcFullPath.filename();

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
						ctx.taskQueue.Push({ srcFullPath, destFullPath, nFunc, dirNode });
					}
				}
			}

			// increment 직후 바로 오류를 확인해야 한다 — 표준 규격상 실패 시 it이 곧바로 end가 되어
			// 버려서, 루프 조건 재평가 시점에는 이미 오류 정보를 관찰할 수 없기 때문이다.
			it.increment(dirEc);
			if( dirEc )
			{
				LOG_WARNING(_T("[경고] 폴더 열람 중 오류로 나머지 항목을 건너뜀: %s (오류: %s)"), sourceFolder.native().c_str(), ErrorToString(dirEc).c_str());
				ctx.scanErrorCount.fetch_add(1, std::memory_order_relaxed);
				ctx.allSuccess.store(false);
				break;
			}
		}
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
void ProducerFunc(const fs::path& srcPath, const fs::path& destPath,
	SH_APPLY_FILEINFO& ShApplyFileInfo, int nFunc, FileProcessContext& ctx)
{
	DirectoryRecursiveSearch(srcPath, destPath, ShApplyFileInfo, nFunc, ctx);

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
//***************************************************************************
int main(int argc, TCHAR* argv[])
{
#ifdef	_MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef _WIN32
	// 1. C 런타임 로케일 설정
	setlocale(LC_ALL, ".UTF8");		// printf, scanf 등 C 스타일의 입출력 함수나 일부 문자열 처리 함수들이 UTF-8 문자열을 올바르게 인식하고 처리할 수 있게 함.

	// 2. 콘솔 입출력 코드페이지를 UTF-8(65001)로 변경
	SetConsoleOutputCP(CP_UTF8);	// 프로그램이 콘솔창에 텍스트를 출력할 때(std::cout, printf 등), 유니코드 문자가 깨지지 않고 올바른 모양(한글 등)으로 그려지도록 지정
	SetConsoleCP(CP_UTF8);			// 사용자가 콘솔창에 키보드로 입력하는 텍스트(std::cin, scanf 등)를 프로그램이 UTF-8 인코딩으로 정확하게 읽어들이도록 보장
#endif

	// 로그 매니저 초기화 (실행 파일 경로 기준 "Log" 디렉토리에 로그 파일 생성)
	// CLogManager::Instance().Create(_T("Log\\"));

	// ==========================================
	// [디버깅용] 인자 강제 오버라이드 (F5 테스트용)
	// ==========================================
#ifdef _DEBUG
	static const TCHAR* mockArgv[] = {
		_T("ShCopyMove.exe"), // argv[0]: 프로그램 이름
		_T("C"),              // argv[1]: 작업 모드(C 또는 M)
		_T("D:\\Source"),     // argv[2]: 원본 경로
		_T("D:\\Dest"),       // argv[3]: 대상 경로
		_T("0"),              // argv[4]: 필터링 적용 여부(0/1/2 : 전체 허용/지정한 확장자만 허용/지정한 확장자는 제외)
		_T("aspx;resx"),      // argv[5]: 확장자 필터
		_T("20120101"),       // argv[6]: 수정일 기준 시작일
		_T("20261231"),       // argv[7]: 수정일 기준 종료일
		_T("0")               // argv[8]: 소비자 스레드 수(0 또는 미지정 시 CoreCount() 사용)
	};

	argc = static_cast<int>(std::size(mockArgv));
	argv = const_cast<TCHAR**>(mockArgv);
#endif

	int nFunc = 0;

	SH_APPLY_FILEINFO ShApplyFileInfo;

	if( argc < 5 )
	{
		LOG_ERROR(_T("알림 : 잘못된 요청입니다. 인자 개수가 부족합니다."));
		return 1;
	}

	// [인자 1] 작업 모드 설정 (M: 파일 이동, 그 외: 파일 복사)
	nFunc = (argv[1][0] == _T('M')) ? FO_MOVE : FO_COPY;

	// [인자 2, 3] 원본/대상 전체 경로 설정 (공백 치환 처리 후 fs::path로 구성)
	_tstring strSrc = replaceAll(_tstring(argv[2]), _T(";32;"), _T(" "));
	_tstring strDest = replaceAll(_tstring(argv[3]), _T(";32;"), _T(" "));
	fs::path srcFullPath(strSrc);
	fs::path destFullPath(strDest);

	// [인자 4] 필터 모드 설정 (0: 미적용, 1: 화이트리스트, 2: 블랙리스트)
	ShApplyFileInfo.m_nFilterMode = std::stoi(_tstring(argv[4]));

	// [인자 5] 필터링할 대상 확장자 설정 (예: txt, log 등)
	if( argc > 5 ) ShApplyFileInfo.m_tszApplyExt = argv[5];

	// [인자 6] 날짜 필터 시작일 설정
	if( argc > 6 ) ShApplyFileInfo.m_tszModifyStDate = argv[6];

	// [인자 7] 날짜 필터 종료일 설정
	if( argc > 7 ) ShApplyFileInfo.m_tszModifyEdDate = argv[7];

	// [인자 8] 소비자 스레드 수 설정(선택, 미지정 또는 0이면 이후 CoreCount()로 대체)
	size_t nRequestedThreads = 0;
	if( argc > 8 ) nRequestedThreads = static_cast<size_t>(std::stoi(_tstring(argv[8])));

	FileProcessContext ctx;
	CThreadManager threadManager;

	// 소비자 스레드 개수 설정: argv[8]로 지정된 값이 있으면 그 값을 사용하고,
	// 값이 0이거나 인자 자체가 없으면 시스템 프로세서 코어 개수로 대체한다.
	size_t numThreads = (nRequestedThreads != 0)
		? nRequestedThreads
		: static_cast<size_t>(SYSTEM::CoreCount());

	LOG_INFO(_T("싱글 생산자 - 멀티 소비자 병렬 처리를 시작합니다 (소비자 스레드 수: %zu)..."), numThreads);
	LOG_INFO(_T("작업 설정 - 원본: %s, 대상: %s"), srcFullPath.native().c_str(), destFullPath.native().c_str());

	// 소비자(Consumer) 스레드 풀 생성 및 실행
	for( size_t t = 0; t < numThreads; ++t )
	{
		threadManager.CreateThread([&ctx]() {
			ConsumerFunc(ctx);
			});
	}

	// 싱글 생산자(Producer) 실행(메인 스레드에서 디렉토리 탐색 및 작업 공급 전담)
	ProducerFunc(srcFullPath, destFullPath, ShApplyFileInfo, nFunc, ctx);

	// 모든 소비자 스레드가 잔여 태스크를 모두 처리하고 종료될 때까지 대기
	threadManager.JoinThreads();

	// 전체 작업 결과 출력
	if( ctx.allSuccess.load() )
	{
		LOG_INFO(_T("알림 : 모든 작업이 성공적으로 완료되었습니다."));
	}
	else
	{
		LOG_WARNING(_T("알림 : 일부 파일 작업 중 오류가 발생했습니다."));
	}

	const _tstring actionLabel = (nFunc == FO_MOVE) ? _T("이동") : _T("복사");

	LOG_INFO(_T("- %s된 폴더 수 (소스 기준) : %s"), actionLabel.c_str(), addCommas(static_cast<int64>(ctx.folderCount.load())).c_str());
	if( nFunc == FO_MOVE )
		LOG_INFO(_T("- 삭제된 폴더 수 (소스 기준) : %s"), addCommas(static_cast<int64>(ctx.deletedFolderCount.load())).c_str());

	LOG_INFO(_T("- 실제 생성된 대상 폴더 수 : %s"), addCommas(static_cast<int64>(ctx.createdFolderCount.load())).c_str());

	if( ctx.fileFailCount.load() > 0 )
	{
		LOG_INFO(_T("- %s된 대상 파일 수 : %s (실패 %s)"), actionLabel.c_str(),
			addCommas(static_cast<int64>(ctx.fileSuccessCount.load())).c_str(),
			addCommas(static_cast<int64>(ctx.fileFailCount.load())).c_str());
	}
	else
	{
		LOG_INFO(_T("- %s된 대상 파일 수 : %s"), actionLabel.c_str(),
			addCommas(static_cast<int64>(ctx.fileSuccessCount.load())).c_str());
	}

	if( ctx.scanErrorCount.load() > 0 )
	{
		LOG_WARNING(_T("- [경고] 폴더 열람 오류로 건너뛴 폴더 수 : %s (원본/복사본 개수가 다를 수 있음, 자세한 내용은 위 오류 로그 참고)"),
			addCommas(static_cast<int64>(ctx.scanErrorCount.load())).c_str());
	}

#ifdef _WIN32
	system("pause");
#endif

	return 0;
}