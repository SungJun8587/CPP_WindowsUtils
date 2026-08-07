//***************************************************************************
// ShCopyMove.cpp : Defines the entry point for the console application.
// 싱글 생산자 - 멀티 소비자(SPMC : Single-Producer Multi-Consumer) 패턴을 
// 이용한 고성능 병렬 파일 복사/이동 프로그램.
//***************************************************************************

#include "pch.h"

namespace fs = std::filesystem;

//***************************************************************************
// @brief 개별 파일 작업(복사 또는 이동)에 필요한 경로 및 명령 정보를 담는 구조체
//***************************************************************************
struct FileTask {
	_tstring srcPath;   // 원본 파일 전체 경로
	_tstring destPath;  // 대상 파일 전체 경로
	int nFunc;          // 작업 종류(FO_COPY 또는 FO_MOVE)
};

//***************************************************************************
// @brief 생산자(Producer)와 소비자(Consumer) 스레드 간 안전한 데이터 교환 및 
//        상태 공유를 위한 컨텍스트 구조체
//***************************************************************************
struct FileProcessContext {
	CQueue<FileTask> taskQueue;					// 파일 작업 태스크 큐

	std::mutex queueMutex;						// 태스크 큐 동기화를 위한 뮤텍스
	std::condition_variable cv;					// 생산자-소비자 간 통신용 조건 변수
	std::atomic<bool> isProducerDone{ false };	// 생산자의 디렉토리 탐색 완료 여부 플래그
	std::atomic<bool> allSuccess{ true };		// 모든 파일 작업이 성공했는지 여부 플래그
};

//***************************************************************************
// @brief 단일 파일의 복사 또는 이동 작업을 처리하는 핵심 함수
// @param srcPath 원본 파일 경로
// @param destPath 대상 파일 경로
// @param nFunc 작업 종류 (FO_COPY 또는 FO_MOVE)
// @return 작업 성공 시 true, 실패 시 false
// @note
//   - FO_MOVE는 우선 fs::rename()을 시도한다. 같은 볼륨 내 이동이면
//     원자적(atomic)으로 처리되어 훨씬 빠르고 중간 상태가 남지 않는다.
//   - rename()이 실패하는 대표적 케이스(다른 드라이브/볼륨 간 이동 등)에는
//     copy_file() + remove()로 폴백한다. 이때 remove()의 성공 여부도
//     반드시 확인하여, 복사는 됐지만 원본 삭제가 실패한 상태를 실패로 보고한다.
//   - 실패 시 원인 파악을 위해 예외 메시지와 대상 경로를 stderr로 남긴다.
//***************************************************************************
bool ProcessSingleFile(const _tstring& srcPath, const _tstring& destPath, int nFunc) {
	try {
		fs::path src(srcPath);
		fs::path dest(destPath);

		// 대상 디렉토리가 존재하지 않으면 자동으로 생성
		if( dest.has_parent_path() && !fs::exists(dest.parent_path()) ) {
			fs::create_directories(dest.parent_path());
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
//***************************************************************************
void DirectoryRecursiveSearch(const TCHAR* ptszSourceFolder, const TCHAR* ptszDestFolder,
	SH_APPLY_FILEINFO& ShApplyFileInfo, int nFunc, FileProcessContext& ctx)
{
	BOOL		bResult = true;
	TCHAR		tszActiveFolder[DIRECTORY_STRLEN + 16];
	TCHAR		tszSourceFullPath[FULLPATH_STRLEN];
	TCHAR		tszSourceFolder[DIRECTORY_STRLEN];
	TCHAR		tszDestFullPath[FULLPATH_STRLEN];
	TCHAR		tszDestFolder[DIRECTORY_STRLEN];

	WIN32_FIND_DATA		FindData;
	HANDLE				hFindFile;

	// 인자 유효성 검사
	if( !ptszSourceFolder || !ptszDestFolder ) return;
	if( _tcslen(ptszSourceFolder) < 1 || _tcslen(ptszDestFolder) < 1 ) return;

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

					// 하위 폴더에 대해 재귀 호출
					DirectoryRecursiveSearch(tszSourceFullPath, tszDestFullPath, ShApplyFileInfo, nFunc, ctx);
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
					{
						// 멀티스레드 동기화를 위한 뮤텍스 락 영역
						std::lock_guard<std::mutex> lock(ctx.queueMutex);
						ctx.taskQueue.push({ tszSourceFullPath, tszDestFullPath, nFunc });
					}
					// 큐에 새로운 작업이 추가되었음을 대기 중인 소비자 스레드(Worker Thread)에 알림
					ctx.cv.notify_one();
				}
			}

			bResult = FindNextFile(hFindFile, &FindData);
		}

		FindClose(hFindFile);
	}
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

	// 디렉토리 탐색 완료 플래그를 설정하고 대기 중인 모든 소비자 스레드를 깨움
	ctx.isProducerDone.store(true);
	ctx.cv.notify_all();
}

//***************************************************************************
// @brief [2] 소비자(Consumer) 스레드 함수: 큐에서 작업을 꺼내어 파일 복사/이동을 병렬 처리
// @param ctx 스레드 간 공유 컨텍스트
//***************************************************************************
void ConsumerFunc(FileProcessContext& ctx)
{
	while( true )
	{
		FileTask task;
		{
			std::unique_lock<std::mutex> lock(ctx.queueMutex);

			// 큐가 비어있고 생산자가 아직 끝나지 않았다면 대기 상태로 진입
			ctx.cv.wait(lock, [&ctx]() {
				return !ctx.taskQueue.empty() || ctx.isProducerDone.load();
				});

			// 큐가 비어있고 생산 작업까지 모두 종료되었다면 루프 탈출
			if( ctx.taskQueue.empty() && ctx.isProducerDone.load() )
			{
				break;
			}

			// 불필요한 문자열 복사를 피하기 위해 move로 꺼낸다.
			task = std::move(ctx.taskQueue.front());
			ctx.taskQueue.pop();
		}

		// 할당받은 파일 작업 수행
		bool success = ProcessSingleFile(task.srcPath, task.destPath, task.nFunc);
		if( !success )
		{
			ctx.allSuccess.store(false);
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
		_T("0"),              // argv[4]: 필터링 적용 여부(0/1/2 : 전체 허용/지정한 확장자만 허용/지정한 확장자는 제외)
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
		printf("알림 : 모든 작업이 성공적으로 완료되었습니다.\n");
	else
		printf("알림 : 일부 파일 작업 중 오류가 발생했습니다.\n");

	system("pause");

	return 0;
}