# FileInfoScanner

## 개요

`FileInfoScanner`는 지정한 디렉토리를 하위 경로까지 재귀적으로 탐색하여, 각 파일의 경로/크기/생성일시/수정일시/텍스트 인코딩 타입을 수집하고 콘솔 또는 엑셀(.xlsx) 파일로 출력하는 Windows 콘솔 프로그램이다.

- 대상 디렉토리 하위의 모든 파일을 재귀 탐색
- 파일별 상세 정보(폴더, 파일명, 크기, 생성일시, 수정일시) 수집
- 파일의 텍스트 인코딩 타입(ANSI/UTF-8/UTF-16 등) 판별
- 결과를 콘솔 표 형태 또는 엑셀 파일(`FileInfoResult.xlsx`, 탐색 대상 디렉토리 내부에 저장)로 출력

## 실행 방법

```
FileInfoScanner.exe <대상 경로> [출력 모드]
```

| 인자 | 설명 |
|---|---|
| `argv[1]` | 탐색할 디렉토리의 전체 경로 (필수) |
| `argv[2]` | 출력 모드. `0` = 콘솔 출력(기본값), `1` = 엑셀 저장 |

예시:

```
FileInfoScanner.exe C:\Source 1
```

`_DEBUG` 빌드에서는 `argv`가 `{"FileInfoScanner.exe", "C:\Source", "1"}`로 하드코딩된 목(mock) 값으로 대체되어, IDE에서 실행 인자 설정 없이 바로 디버깅할 수 있도록 되어 있다.

## 콘솔 인코딩 설정

`main()` 진입 직후 다음 두 단계로 콘솔을 UTF-8 출력 환경으로 맞춘다.

1. `system("chcp 65001 > nul")` — 콘솔 코드페이지를 UTF-8(65001)로 전환
2. `_tcout.imbue(std::locale(".65001"))` — `_tcout`(TCHAR 기반 출력 스트림)이 UTF-8 로케일로 변환하여 출력하도록 설정

이 설정 덕분에 한글 경로/파일명이 포함된 결과도 콘솔에서 깨지지 않고 출력된다.

## 데이터 구조: `FileInfo`

```cpp
struct FileInfo : public Xlnt::ExcelSerializable<std::string, std::string, std::uintmax_t, std::string, std::string, std::string>
```

파일 하나의 정보를 담는 구조체로, `CXlntUtil`의 `ExcelSerializable`을 상속받아 엑셀 직렬화를 지원한다.

| 멤버 | 타입 | 설명 |
|---|---|---|
| `folder` | `_tstring` | 파일이 위치한 폴더 경로 |
| `filename` | `_tstring` | 파일 이름 |
| `size` | `std::uintmax_t` | 파일 크기 (Byte) |
| `creationTime` | `_tstring` | 생성일시 (`YYYY-MM-DD HH:MM:SS`) |
| `modifiedTime` | `_tstring` | 수정일시 (`YYYY-MM-DD HH:MM:SS`) |
| `encodingStr` | `_tstring` | 인코딩 타입 문자열 |

### 값을 두 벌로 보관하는 이유

`FileInfo`는 동일한 값을 두 가지 형태로 동시에 갖는다.

- **`_tstring` 멤버** (`folder`, `filename` 등): 콘솔 출력 모드에서 `_tcout`이 직접 사용
- **`std::string` 기반 `ExcelSerializable::fields`**: 엑셀 저장 모드에서 `CXlntUtil::Serialize()`가 사용

생성자에서 `TStringToString()`을 통해 `_tstring` → `std::string`(ANSI/CP949) 변환이 한 번 일어나며, 이후 두 표현은 각자의 소비 경로(콘솔/엑셀)에서 재변환 없이 그대로 쓰인다.

### `get_field_names()`

엑셀 헤더에 들어갈 6개 컬럼명(`폴더 이름`, `파일 이름`, `크기(Bytes)`, `생성일시`, `수정일시`, `인코딩 타입`)을 반환한다. `CXlntUtil::Serialize()`가 헤더 행을 작성할 때 이 함수를 호출한다.

## 인코딩 판별: `GetEncodingString()`

`FileUtil`에서 판별된 `EEncoding` 열거값을 사람이 읽을 수 있는 문자열로 변환한다.

| `EEncoding` | 출력 문자열 |
|---|---|
| `DEFAULT` | `DEFAULT` |
| `ANSI` | `ANSI` |
| `UTF16_LE` | `UTF-16 LE` |
| `UTF16_BE` | `UTF-16 BE` |
| `UTF8_BOM` | `UTF-8 BOM` |
| `UTF8_NOBOM` | `UTF-8 NO BOM` |
| 그 외 | `UNKNOWN` |

실제 인코딩 판별 로직(BOM 검사 및 UTF-8 휴리스틱 검사)은 `FileUtil`의 `GetFileInfoAndEncoding()`이 수행한다.

## 엑셀 헤더 스타일: `ToHeaderStyle()`

지정한 셀 범위(예: `"A1"` ~ `"F1"`)에 다음 스타일을 일괄 적용한다.

- 행 높이 20
- 검정 배경(`fill`)
- 얇은 테두리(상하좌우)
- 글꼴: 굴림, 10pt, 굵게, 흰색
- 가로/세로 중앙 정렬

`CXlntUtil::CreateRange()`로 셀 범위를 생성한 뒤, `xlnt::border`/`xlnt::fill`/`xlnt::font`/`xlnt::alignment`를 각각 구성해 범위에 한 번에 적용하는 구조다.

## 디렉토리 탐색: `DirectoryRecursiveSearch()`

```cpp
bool DirectoryRecursiveSearch(const fs::path& dirPath, std::vector<FileInfo>& outFileList)
```

대상 경로 하위의 모든 파일을 순회하며 `outFileList`에 `FileInfo`를 채워 넣는다.

1. **경로 유효성 확인**: `dirPath`가 존재하고 디렉토리인지 확인. 아니면 `false` 반환.
2. **용량 사전 확보**: `outFileList.reserve()`로 초기 여유 공간을 확보해 벡터 재할당 횟수를 줄인다.
3. **재귀 순회**: `std::filesystem::recursive_directory_iterator`로 하위 디렉토리를 포함한 모든 항목을 순회한다. `skip_permission_denied` 옵션으로 접근 권한이 없는 폴더/파일은 예외 없이 건너뛴다.
4. **파일 필터링**: 디렉토리 항목은 건너뛰고, 일반 파일(`is_regular_file`)만 처리한다.
5. **기본 정보 추출**: 경로에서 폴더/파일명을 분리하고, `entry.file_size()`로 크기를 얻는다.
6. **상세 정보 + 인코딩 조회**: Windows 환경(`_WIN32`)에서는 `GetFileInfoAndEncoding()`을 호출해 파일 핸들을 한 번만 열어 `BY_HANDLE_FILE_INFORMATION`(생성/수정 시각)과 `EEncoding`(인코딩 타입)을 동시에 얻는다. `FILETIME`을 로컬 시간대의 `SYSTEMTIME`으로 변환한 뒤 문자열로 포맷한다. 조회에 실패하면 생성/수정일시는 `"N/A"`로 대체한다.
7. **결과 적재**: 위에서 모은 값으로 `FileInfo`를 생성해 `outFileList`에 추가한다.

## 실행 흐름: `main()`

1. 콘솔 UTF-8 환경 설정
2. 인자 파싱 (대상 경로, 출력 모드)
3. `DirectoryRecursiveSearch()`로 대상 경로 전체를 탐색해 `fileList`를 채움
4. 탐색 성공 시, 출력 모드에 따라 분기

### 엑셀 저장 모드 (`mode == 1`)

1. `CXlntUtil` 인스턴스 생성
2. 기본 시트 이름을 `"FileInfo"`로 변경 후 활성화 (`RenameSheet` + `ActiveSheet`)
3. `xlntUtil.Serialize(fileList, true)` — `FileInfo::get_field_names()`로 헤더 행을, `FileInfo::serialize()`(상속받은 `ExcelSerializable::serialize()`)로 각 행의 데이터를 한 번에 기록한다. `isCastUtf8=true`이므로 ANSI(CP949) 문자열을 UTF-8로 변환해 셀에 기록한다.
4. `ToHeaderStyle()`로 헤더 영역(`A1:F1`)에 스타일 적용
5. `SaveAs()`로 결과 파일 저장. 저장 경로는 `fs::path(tszSrcFullPath) / "FileInfoResult.xlsx"`로 조합되어, 실행 파일 위치나 현재 작업 디렉토리가 아니라 **탐색 대상으로 지정한 디렉토리 안**에 저장된다. `fs::path`의 `/` 연산자를 쓰므로 `tszSrcFullPath`에 구분자(`\`)가 있든 없든 자동으로 정규화된다.
6. 저장 과정에서 예외 발생 시 `catch` 블록에서 오류 메시지를 콘솔에 출력

### 콘솔 출력 모드 (`mode == 0`, 기본값)

1. `_tstringstream` 버퍼(`oss`)에 구분선, 헤더 행, 각 파일의 정보 행을 순서대로 쌓는다.
2. 각 컬럼은 `std::setw()`로 폭을 맞춰 정렬한다(폴더 25, 파일명 25, 크기 12, 생성/수정일시 22, 인코딩 22).
3. 모든 내용을 다 쌓은 뒤 `_tcout << oss.str()`로 한 번에 출력한다.

### 오류 처리

대상 경로가 존재하지 않거나 디렉토리가 아니어서 `DirectoryRecursiveSearch()`가 `false`를 반환하면, `표준 에러(std::cerr)`로 "유효하지 않은 디렉토리 경로이거나 읽을 수 없습니다" 메시지를 출력하고 종료한다.

## 의존 모듈

| 모듈 | 역할 |
|---|---|
| `Excel/XlntUtil.h` (`Xlnt::CXlntUtil`, `Xlnt::ExcelSerializable`) | xlnt 라이브러리 기반 엑셀 워크북/워크시트 제어 및 객체 직렬화 |
| `Util/EncodingConvert.h` | `_tstring` ↔ `std::string` 간 ANSI/UTF-8/UTF-16 변환 함수 제공 (`TStringToString`, `AnsiToUtf8`, `Utf8ToTString` 등) |
| `FileUtil.h/.cpp` | `GetFileInfoAndEncoding()` 등 Win32 API 기반 파일 메타데이터 조회 및 인코딩 판별 |

## 주요 설계 특징 요약

- **경로 탐색**: `std::filesystem::recursive_directory_iterator` 기반으로 재귀 구조 없이 하위 디렉토리 전체를 순회하며, 접근 권한 문제가 있는 항목은 예외 없이 건너뛴다.
- **파일 I/O 최소화**: 파일 하나당 핸들을 한 번만 열어(`GetFileInfoAndEncoding`) 상세 정보와 인코딩 타입을 동시에 조회한다.
- **이중 표현 데이터 모델**: `FileInfo`가 `_tstring`(콘솔용)과 `std::string`(엑셀용) 두 형태를 함께 보관하여, 출력 모드별로 별도의 변환 과정 없이 바로 사용할 수 있게 한다.
- **일괄 기록**: 엑셀 저장은 셀 단위 개별 호출 대신 `Serialize()` 템플릿으로 헤더와 본문을 한 번에 기록한다.
- **버퍼링된 콘솔 출력**: 파일마다 스트림에 직접 쓰는 대신 문자열 스트림에 모아 한 번에 flush한다.
- **결과 파일 위치**: 엑셀 저장 시 `FileInfoResult.xlsx`는 `fs::path`로 대상 디렉토리 경로와 결합되어, 탐색 대상 디렉토리 안에 생성된다.
