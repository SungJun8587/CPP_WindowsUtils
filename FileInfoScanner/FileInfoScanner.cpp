// FileInfoScanner.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include "pch.h"
#include "Excel/XlntUtil.h"
#include <Util/EncodingConvert.h>

namespace fs = std::filesystem;

//***************************************************************************
// @brief 파일의 상세 정보를 저장하는 구조체입니다. (CXlntUtil 직렬화 연동)
// @details ExcelSerializable을 상속받아 엑셀 행 데이터와의 매핑을 지원합니다.
//***************************************************************************
struct FileInfo : public Xlnt::ExcelSerializable<std::string, std::string, std::uintmax_t, std::string, std::string, std::string>
{
    _tstring folder;              // 폴더 경로
    _tstring filename;            // 파일 이름
    std::uintmax_t size;          // 파일 크기 (Byte)
    _tstring creationTime;        // 생성일시 (YYYY-MM-DD HH:MM:SS)
    _tstring modifiedTime;        // 수정일시 (YYYY-MM-DD HH:MM:SS)
    _tstring encodingStr;         // 인코딩 타입 문자열

    FileInfo() : ExcelSerializable("", "", 0, "", "", "") {}

    //***************************************************************************
    // @brief FileInfo 객체를 생성하며, 콘솔 출력용(_tstring)과 엑셀 직렬화용(std::string)
    //        두 가지 형태로 동일한 값을 동시에 보관합니다.
    // @param[in] f  폴더 경로
    // @param[in] fn 파일 이름
    // @param[in] s  파일 크기 (Byte)
    // @param[in] ct 생성일시 문자열
    // @param[in] mt 수정일시 문자열
    // @param[in] es 인코딩 타입 문자열
    // @details ExcelSerializable 베이스 클래스는 std::string 튜플만 다루므로,
    //          각 _tstring 인자를 TStringToString()으로 narrow 변환하여 베이스에
    //          전달합니다(엑셀 저장 시 Serialize()가 이 값을 사용). 이 변환은
    //          생성자에서 필드당 1회만 발생하며, 이후 재변환은 없습니다.
    //          동시에 원본 _tstring 값을 FileInfo 자신의 멤버로도 보관하는데,
    //          이는 콘솔 출력 모드(_tcout)가 _tstring을 그대로 사용하기 때문입니다.
    //          즉 동일한 데이터를 용도별로 두 벌 유지하는 구조입니다.
    //***************************************************************************
    FileInfo(const _tstring& f, const _tstring& fn, std::uintmax_t s, const _tstring& ct, const _tstring& mt, const _tstring& es)
        // 1. std::string 필드(fields) 초기화 — 엑셀 저장(Serialize)용
        : ExcelSerializable(
            TStringToString(f),
            TStringToString(fn),
            s,
            TStringToString(ct),
            TStringToString(mt),
            TStringToString(es)
        ),
        // 2. _tstring 멤버 초기화 — 콘솔 출력(_tcout)용
        folder(f), filename(fn), size(s), creationTime(ct), modifiedTime(mt), encodingStr(es) {
    }

    //***************************************************************************
    // @brief 엑셀 상단에 들어갈 필드(헤더) 이름 목록을 반환합니다.
    // @return std::vector<std::string> 헤더 문자열 벡터
    //***************************************************************************
    static std::vector<std::string> get_field_names()
    {
        return { "폴더 이름", "파일 이름", "크기(Bytes)", "생성일시", "수정일시", "인코딩 타입" };
    }
};

//***************************************************************************
// @brief EEncoding 값을 문자열로 변환합니다.
// @param[in] encoding 변환할 인코딩 타입
// @return _tstring 인코딩 이름을 나타내는 문자열
//***************************************************************************
inline _tstring GetEncodingString(EEncoding encoding)
{
    switch( encoding )
    {
    case EEncoding::DEFAULT:     return _T("DEFAULT");
    case EEncoding::ANSI:        return _T("ANSI");
    case EEncoding::UTF16_LE:    return _T("UTF-16 LE");
    case EEncoding::UTF16_BE:    return _T("UTF-16 BE");
    case EEncoding::UTF8_BOM:    return _T("UTF-8 BOM");
    case EEncoding::UTF8_NOBOM:  return _T("UTF-8 NO BOM");
    default:                     return _T("UNKNOWN");
    }
}

//***************************************************************************
// @brief 엑셀 헤더 영역에 스타일을 적용합니다.
// @param[in,out] excel 스타일을 적용할 CXlntUtil 객체 참조
// @param[in] start_cell 시작 셀 주소 (예: "A1")
// @param[in] end_cell 끝 셀 주소 (예: "F1")
// @details 헤더 행 높이, 배경색, 테두리, 폰트 및 정렬 방식을 설정합니다.
//***************************************************************************
void ToHeaderStyle(Xlnt::CXlntUtil& excel, const std::string& start_cell, const std::string& end_cell)
{
    // 1. 헤더 행 높이 설정
    excel.SetRowHeight(1, 20);

    // 2. 스타일을 적용할 셀 범위 생성
    auto borderRange = excel.CreateRange(start_cell, end_cell);

    // 3. 배경색 설정
    xlnt::fill header_fill;
    header_fill = xlnt::pattern_fill().type(xlnt::pattern_fill_type::solid);
    header_fill = header_fill.pattern_fill().foreground(xlnt::color::black());
    header_fill = header_fill.pattern_fill().background(xlnt::color::black());

    // 4. 테두리 설정
    xlnt::border cell_border;
    xlnt::border::border_property outer_prop;
    outer_prop.style(xlnt::border_style::thin);

    cell_border.side(xlnt::border_side::bottom, outer_prop);
    cell_border.side(xlnt::border_side::start, outer_prop);
    cell_border.side(xlnt::border_side::top, outer_prop);
    cell_border.side(xlnt::border_side::end, outer_prop);

    // 5. 폰트 스타일 설정
    xlnt::font title_font;
    title_font.name("Gulim");                    // 폰트 패밀리
    title_font.bold(true);                      // 폰트 굵게
    title_font.size(10);                        // 폰트 크기
    title_font.color(xlnt::color::white());     // 폰트 색깔

    // 6. 정렬 설정
    xlnt::alignment center_align;
    center_align.horizontal(xlnt::horizontal_alignment::center);
    center_align.vertical(xlnt::vertical_alignment::center);

    // 7. 범위에 스타일 일괄 적용
    borderRange.border(cell_border);
    borderRange.fill(header_fill);
    borderRange.font(title_font);
    borderRange.alignment(center_align);
}

//***************************************************************************
// @brief 특정 디렉토리를 재귀 검색하여 파일 정보 목록을 벡터 파라미터에 대입하는 함수입니다.
// @param[in] dirPath 검색 대상 디렉토리 경로
// @param[out] outFileList 추출된 파일 정보들이 저장될 FileInfo 벡터 참조 변수
// @return bool 디렉토리 검색 성공 여부 (경로가 유효하지 않으면 false 반환)
// @details recursive_directory_iterator로 하위 디렉토리를 전부 순회하며,
//          접근 불가(권한 부족 등) 항목은 건너뜁니다.
//***************************************************************************
bool DirectoryRecursiveSearch(const fs::path& dirPath, std::vector<FileInfo>& outFileList)
{
    // 1. 경로 존재 유무 및 디렉토리 여부 확인
    if( !fs::exists(dirPath) || !fs::is_directory(dirPath) )
    {
        return false;
    }

    // 2. 재할당 횟수 축소를 위한 벡터 용량 사전 확보
    outFileList.reserve(outFileList.size() + 1024);

    // 3. 디렉토리 전체를 재귀 순회 (권한 없는 항목은 건너뜀)
    std::error_code ec;
    fs::recursive_directory_iterator it(dirPath, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;

    for( ; it != end && !ec; it.increment(ec) )
    {
        const auto& entry = *it;

        // 4. 일반 파일인 경우에만 정보 추출 수행
        std::error_code fileEc;
        if( !entry.is_regular_file(fileEc) || fileEc )
        {
            continue;
        }

        // 5. 경로/크기 등 기본 정보 추출
        _tstring folder = entry.path().parent_path().c_str();
        _tstring filename = entry.path().filename().c_str();
        std::uintmax_t size = entry.file_size(fileEc);
        _tstring creationTime;
        _tstring modifiedTime;
        EEncoding encoding = EEncoding::DEFAULT;

#ifdef _WIN32
        // 6. 핸들 1회 오픈으로 파일 상세 정보 + 인코딩 타입 동시 조회
        BY_HANDLE_FILE_INFORMATION fileInfo;
        if( GetFileInfoAndEncoding(entry.path().c_str(), &fileInfo, encoding) )
        {
            creationTime = FileTimeToLocalString(fileInfo.ftCreationTime);
            modifiedTime = FileTimeToLocalString(fileInfo.ftLastWriteTime);
        }
        else
        {
            creationTime = _T("N/A");
            modifiedTime = _T("N/A");
        }
#else
        // 6. Win32 전용 API(CreateFile/BY_HANDLE_FILE_INFORMATION)를 쓸 수 없는 환경.
        //    생성일시는 표준 라이브러리로 이식성 있게 구할 방법이 없어 N/A 처리하고,
        //    수정일시는 std::filesystem::last_write_time(), 인코딩은 std::ifstream 기반의
        //    이식성 있는 GetFileEncodingType(const _tstring&) 오버로드로 판별한다.
        creationTime = _T("N/A");
        auto ftime = fs::last_write_time(entry.path(), fileEc);
        if( !fileEc )
        {
            // file_clock -> system_clock 변환 (C++17 호환 방식; C++20이면 std::chrono::clock_cast로 대체 가능)
            auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
            std::time_t tt = std::chrono::system_clock::to_time_t(sctp);

            modifiedTime = ptime::Format(tt, _T("%Y-%m-%d %H:%M:%S"));
        }
        else
        {
            modifiedTime = _T("N/A");
        }
    
        encoding = GetFileEncodingType(_tstring(entry.path().c_str()));
#endif

        _tstring encodingStr = GetEncodingString(encoding);

        // 7. FileInfo 객체 생성 후 결과 벡터에 대입
        outFileList.emplace_back(folder, filename, size, creationTime, modifiedTime, encodingStr);
    }

    return true;
}

//***************************************************************************
// @brief 프로그램 진입점 (Main 함수)
// @param argc 전달된 인자 개수
// @param argv 전달된 인자 배열
// @return 성공 시 0, 오류 시 1
// @note
//	[FileInfoScanner.exe 인자(Arguments) 상세 설명]
//
//	사용 형식:
//	FileInfoScanner.exe [원본경로] [출력모드]
//
//	1. [원본경로] (argv[1])
//    - 파일 정보를 스캔할 원본 폴더의 전체 경로입니다.
//    - 경로 내 공백이 포함된 경우 공백 대신 ";32;"를 사용할 수 있습니다.
//
//	2. [출력모드] (argv[2], 선택)
//    - 0 : 콘솔 출력 모드 (스캔된 파일 정보를 콘솔 화면에 테이블 형태로 출력합니다.)
//    - 1 : 엑셀 저장 모드 (스캔된 파일 정보를 원본 경로 내에 "FileInfoResult.xlsx" 파일로 저장합니다.)
//    - 값을 지정하지 않거나 생략할 경우 기본값은 0 (콘솔 출력)입니다.
//
//	[프로그램 실행 예제 및 사용법 안내]
//	1. 콘솔 출력 예제:
//		FileInfoScanner.exe "C:\Source" 0
//
//	2. 엑셀 저장 예제:
//		FileInfoScanner.exe "C:\Source" 1
//
//	[명령행 인수(Arguments) 설정하고, 디버깅 모드로 실행]
//	1. 비주얼 스튜디오 상단 메뉴에서 [프로젝트] -> [속성(Properties)]을 클릭합니다. (단축키: Alt + F7)
//	2. 창 왼쪽 메뉴에서 [구성 속성] -> [디버깅(Debugging)]을 선택합니다.
//	3. 오른쪽 항목 중 [명령 인수(Command Arguments)] 칸을 클릭합니다.
//	4. 앞서 만든 실행 예제 중 테스트할 문자열을 입력합니다. (예: "C:\Source" 1)
//	5. 오른쪽 아래의 [적용] 버튼을 누르고 [확인]을 누릅니다.
//	6. 키보드의 F5 키를 누르거나, 상단 메뉴의 [디버그] -> [디버깅 시작]을 클릭합니다.
//
//***************************************************************************
int main(int argc, TCHAR* argv[])
{
#ifdef _MSC_VER
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

#ifdef _WIN32
    // 1. C 런타임 로케일 설정
    setlocale(LC_ALL, ".UTF8");		// printf, scanf 등 C 스타일의 입출력 함수나 일부 문자열 처리 함수들이 UTF-8 문자열을 올바르게 인식하고 처리할 수 있게 함.

    // 2. 콘솔 입출력 코드페이지를 UTF-8(65001)로 변경
    SetConsoleOutputCP(CP_UTF8);	// 프로그램이 콘솔창에 텍스트를 출력할 때(std::cout, printf 등), 유니코드 문자가 깨지지 않고 올바른 모양(한글 등)으로 그려지도록 지정
    SetConsoleCP(CP_UTF8);			// 사용자가 콘솔창에 키보드로 입력하는 텍스트(std::cin, scanf 등)를 프로그램이 UTF-8 인코딩으로 정확하게 읽어들이도록 보장
#endif

#ifdef _DEBUG
    static const TCHAR* mockArgv[] = {
        _T("FileInfoScanner.exe"),      // argv[0]: 프로그램 이름
        _T("C:\\Source"),               // argv[1]: 원본 경로
        _T("1")                         // argv[2]: 0(콘솔 출력) / 1(엑셀 저장)
    };

    argc = _countof(mockArgv);
    argv = const_cast<TCHAR**>(mockArgv);
#endif

    TCHAR tszSrcFullPath[FULLPATH_STRLEN] = { 0 };

    // 3. 필수 인자(대상 경로) 확인
    if( argc < 2 )
    {
        _tcout << _T("알림 : 잘못된 요청입니다.\n");
        return 1;
    }

    std::vector<FileInfo> fileList;
    _tcsncpy_s(tszSrcFullPath, _countof(tszSrcFullPath), argv[1], _TRUNCATE);

    // 4. 출력 모드 인자 파싱 (0: 콘솔, 1: 엑셀)
    int mode = 0; // 기본값: 콘솔 출력
    if( argc >= 3 )
    {
        mode = _ttoi(argv[2]);
    }

    // 5. 대상 디렉토리 재귀 검색 수행
    if( DirectoryRecursiveSearch(tszSrcFullPath, fileList) )
    {
        if( mode == 1 )
        {
            try
            {
                // 6-1. 엑셀 저장 모드 — 워크북 생성 및 시트 준비
                Xlnt::CXlntUtil xlntUtil;

                // 새로운 시트를 추가하는 대신, 첫 번째 기본 시트의 이름을 변경하고 활성화합니다.
                xlntUtil.RenameSheet("FileInfo");
                xlntUtil.ActiveSheet("FileInfo");

                // 6-2. 헤더 + 본문 데이터 일괄 기록 (ExcelSerializable 기반)
                xlntUtil.Serialize(fileList, true);

                // 6-3. 헤더 스타일 적용 ("A1"부터 "F1"까지)
                ToHeaderStyle(xlntUtil, "A1", "F1");

                // 6-4. 파일 저장
                _tstring savePath = (fs::path(tszSrcFullPath) / _T("FileInfoResult.xlsx")).native();
                xlntUtil.SaveAs(savePath);
                _tcout << _T("[+] 엑셀 파일 저장 완료: FileInfoResult.xlsx\n");
            }
            catch( const std::exception& ex )
            {
                _tcout << _T("[-] 엑셀 저장 중 예외 발생: ") << Utf8ToTString(ex.what()) << _T("\n");
            }
        }
        else
        {
            // 7-1. 콘솔 출력 모드 — 버퍼(oss)에 모아서 한 번에 flush
            _tstringstream oss;

            // 7-2. 상단 구분선 + 헤더 행 작성
            oss << _T("\n") << _tstring(125, _T('=')) << _T("\n");
            oss << std::left
                << std::setw(25) << _T("폴더 이름")
                << std::setw(25) << _T("파일 이름")
                << std::setw(12) << _T("크기(Bytes)")
                << std::setw(22) << _T("생성일시")
                << std::setw(22) << _T("수정일시")
                << std::setw(22) << _T("인코딩 타입") << _T("\n");
            oss << _tstring(125, _T('=')) << _T("\n");

            // 7-3. 본문 행 작성
            for( const auto& file : fileList )
            {
                oss << std::left
                    << std::setw(25) << file.folder
                    << std::setw(25) << file.filename
                    << std::setw(12) << file.size
                    << std::setw(22) << file.creationTime
                    << std::setw(22) << file.modifiedTime
                    << std::setw(22) << file.encodingStr
                    << _T("\n");
            }
            oss << _tstring(125, _T('=')) << _T("\n");

            // 7-4. 버퍼 내용을 콘솔에 일괄 출력
            _tcout << oss.str();
        }
    }
    else
    {
        std::cerr << "[-] 유효하지 않은 디렉토리 경로이거나 읽을 수 없습니다.\n";
    }

    return 0;
}