# SystemInfoTool.cpp 상세 설명

## 1. 개요

`SystemInfoTool.cpp`는 Windows 시스템의 하드웨어/소프트웨어 정보를 WMI(Windows Management Instrumentation)와 각종 Win32 API를 통해 수집하여, 로그 형태로 콘솔·파일에 출력하는 **콘솔 애플리케이션의 진입점(entry point)**입니다.

BIOS, CPU, 메인보드, 메모리, 디스크, 사운드카드, 그래픽카드, 네트워크카드, CD-ROM, 키보드, 마우스, 모니터, OS, IE, DirectX, JVM, 설치된 소프트웨어 목록까지 총 **18개 항목**을 순차적으로 조회하여 출력합니다.

## 2. 전체 실행 흐름

```
main()
 ├─ 1. 디버그 메모리 누수 감지 설정 (_CRTDBG)
 ├─ 2. 콘솔 UTF-8 인코딩 설정 (Windows 전용)
 ├─ 3. 각 정보 수집 클래스 인스턴스 선언
 ├─ 4. winmgmt(WMI) 서비스를 자동 시작으로 설정
 ├─ 5. 로그 매니저(CLogManager) 초기화
 ├─ 6. COM 라이브러리 초기화 (CoInitializeEx)
 ├─ 7. COM 보안 설정 (CoInitializeSecurity)
 ├─ 8. { 중첩 스코프 시작
 │    ├─ CWmi 생성 및 Connect()
 │    ├─ 1~18번 정보 수집 및 LOG_WRITE 출력 (총 18개 섹션)
 │    │    └─ (디버그 빌드에서는 매 섹션마다 일시정지 + 화면 클리어)
 │    } ← Wmi 소멸 (COM이 살아있는 상태에서 안전하게 Release)
 ├─ 9. CoUninitialize()
 ├─ 10. (릴리즈 빌드만) 결과 파일 경로 안내 + 일시정지
 └─ 11. return 0
```

## 3. 코드 섹션별 상세 설명

### 3.1 헤더 및 매크로

```cpp
#include <pch.h>
#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>

#define VS_SEVICE_TITLE _T("winmgmt")
```

- `pch.h`(precompiled header) 하나로 이 파일에서 쓰는 모든 클래스·매크로·타입이 들어옵니다. `pch.h` 내용을 확인한 결과, 실제 include 체인은 다음과 같습니다(자세한 내용은 4절 참고):
  - `BaseDefine.h` → 상수 정의(`NUMERIC_STRING_LEN` 등)
  - `BaseRedefineDataType.h` → `_tstring`, `int8`~`uint64` 등 크로스플랫폼 타입 및 전역 `using namespace std;`(4.3절 참고)
  - `BaseMacro.h` → `InitUtf8Console()`, `SAFE_DELETE` 등 매크로
  - `Util/Log.h` → `LOG_WRITE`/`LOG_ERROR`/`CLogManager`/`ELOG_TYPE` (내용은 미제공이라 시그니처까지는 확인 불가)
  - `System/Wmi.h`, `System/OsInfo.h`, `System/SoftwareInfo.h`, `System/CpuInfo.h`, `System/HardwareInfo.h` → `CWmi`, `COsInfo`, `CIeInfo`/`CDirectXInfo`/`CJavaVMInfo`/`CInstallSwInfo`, `CCpuInfo`, 나머지 하드웨어 정보 클래스 전부
- `VS_SEVICE_TITLE`은 WMI 핵심 서비스인 `winmgmt`의 서비스 이름입니다.
- 이 파일 자체에는 `using namespace std;`를 별도로 선언하지 않습니다 — `pch.h`가 include하는 `BaseRedefineDataType.h`에 이미 전역 선언이 있어(4.3절), `std::` 접두사 없이 표준 라이브러리 심볼을 쓸 수 있는 상태가 `pch.h` 한 줄만으로 갖춰집니다.

### 3.2 진입점 초기화

```cpp
int main(int argc, char* argv[])
{
#ifdef	_MSC_VER
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
```
- MSVC 디버그 런타임에서 프로그램 종료 시 메모리 누수를 자동으로 검사·보고하도록 설정합니다. 릴리즈 빌드에는 영향이 없습니다.

```cpp
	InitUtf8Console();
```
- C 런타임 로케일과 콘솔 입출력 코드페이지를 모두 UTF-8(65001)로 맞춰, 한글을 포함한 유니코드 문자열이 콘솔에서 깨지지 않고 올바르게 표시/입력되도록 합니다.
- `InitUtf8Console()`은 `Util/ConsoleUtil.h`에 정의된 **`inline` 함수**입니다(자세한 내용은 4.5절 참고). Windows에서는 `setlocale(LC_ALL, ".UTF8")`, `SetConsoleOutputCP(CP_UTF8)`, `SetConsoleCP(CP_UTF8)`를, 비Windows에서는 `setlocale(LC_ALL, "")`을 호출하는 크로스플랫폼 함수로, `BaseMacro.h`/`Util/Log.h`와 함께 include되는 `Util/ConsoleUtil.h`(4.5절) 한 곳에서 `ClearConsoleScreen()`/`PauseConsole()`과 동일한 방식(매크로가 아닌 `inline` 함수)으로 관리됩니다.

### 3.3 정보 수집 클래스 인스턴스 선언

```cpp
CCpuInfo CpuInfo; CBiosInfo BiosInfo; CMainBoardInfo MainBoardInfo;
CMemoryInfo MemoryInfo; CHdDiskInfo HdDiskInfo; CDriveInfo DriveInfo;
CSoundCardInfo SoundCardInfo; CVideoCardInfo VideoCardInfo;
CNetworkCardInfo NetworkCardInfo; CCdromInfo CdromInfo;
CKeyBoardInfo KeyBoardInfo; CMouseInfo MouseInfo; CMonitorInfo MonitorInfo;

COsInfo OsInfo; CIeInfo IeInfo; CDirectXInfo DirectXInfo;
CJavaVMInfo JavaVMInfo; CInstallSwInfo InstallSwInfo;
```
각 클래스는 담당 하드웨어/소프트웨어 항목에 대한 `GetInformation()` 메서드와, 결과를 꺼내는 `Get*()` Getter들을 제공합니다.

| 클래스 | 정의 헤더 | WMI 사용 | 정보 출처 | 설명 |
|---|---|:---:|---|---|
| `CCpuInfo` | `System/CpuInfo.h` | ✗ | CPUID 명령어 | 제조사·모델명, 클럭 속도, 논리 프로세서 수, Family/Model/Stepping |
| `CBiosInfo` | `System/HardwareInfo.h` | ✓ | `Win32_BIOS` | 제조사, SMBIOS 버전, BIOS 버전, 식별 코드, 시리얼 번호, 출시일 |
| `CMainBoardInfo` | `System/HardwareInfo.h` | ✓ | `Win32_BaseBoard` | 제품명, 시리얼 번호, 제조사, 설명 |
| `CMemoryInfo` | `System/HardwareInfo.h` | ✓ | `Win32_PhysicalMemory`, `Win32_OperatingSystem` + `GlobalMemoryStatusEx` | 장착 RAM 모듈별 상세 정보(뱅크·용량·타입·속도) 및 시스템 전체 메모리/가상메모리/페이지파일 통계 |
| `CHdDiskInfo` | `System/HardwareInfo.h` | ✓ | `Win32_DiskDrive` | 물리 디스크별 모델명, 제조사, 설명, 총 용량 |
| `CDriveInfo` | `System/HardwareInfo.h` | ✓ | `Win32_LogicalDisk` | 논리 드라이브(`C:`, `D:` 등)별 파일시스템, 전체/여유 공간 |
| `CSoundCardInfo` | `System/HardwareInfo.h` | ✗ | WinMM API(`waveOutGetDevCaps`) | 볼륨 제어 지원 여부, 제품명, 회사명 |
| `CVideoCardInfo` | `System/HardwareInfo.h` | ✗ | 레지스트리(`DEVICEMAP\VIDEO`) | 그래픽카드 설명, 어댑터 문자열, 칩타입, 드라이버, 메모리 크기 |
| `CNetworkCardInfo` | `System/HardwareInfo.h` | ✓ | `Win32_NetworkAdapterConfiguration` | 네트워크 어댑터 설명 |
| `CCdromInfo` | `System/HardwareInfo.h` | ✓ | `Win32_CDROMDrive` | 광학 드라이브 이름, 제조사, 설명 |
| `CKeyBoardInfo` | `System/HardwareInfo.h` | ✓ | `Win32_Keyboard` + `GetKeyboardType` API | 키보드 설명, 레이아웃/인터페이스 유형 |
| `CMouseInfo` | `System/HardwareInfo.h` | ✓ | `Win32_PointingDevice` | 마우스 이름, 제조사, 설명 |
| `CMonitorInfo` | `System/HardwareInfo.h` | ✓ | `Win32_DesktopMonitor` | 모니터 제조사, 설명 |
| `COsInfo` | `System/OsInfo.h` | ✗ | `GetVersionEx`/`RtlGetVersion`, 레지스트리 | OS 버전·에디션 판별, 32/64비트 여부, 빌드 번호, 서비스팩 |
| `CIeInfo` | `System/SoftwareInfo.h` | ✗ | 레지스트리 | Internet Explorer 빌드, 버전 |
| `CDirectXInfo` | `System/SoftwareInfo.h` | ✗ | 레지스트리 | DirectX 버전, 설치 버전, 설명 |
| `CJavaVMInfo` | `System/SoftwareInfo.h` | ✗ | 레지스트리 + 파일 시스템 탐색 | MS/SUN JVM 설치 여부(상태 코드 0~3) |
| `CInstallSwInfo` | `System/SoftwareInfo.h` | ✗ | 레지스트리(`Uninstall` 키) | 설치된 소프트웨어 표시 이름 목록 |

이 중 `Wmi`를 인자로 받는 클래스(`BiosInfo`, `MainBoardInfo`, `MemoryInfo`, `HdDiskInfo`, `DriveInfo`, `NetworkCardInfo`, `CdromInfo`, `KeyBoardInfo`, `MouseInfo`, `MonitorInfo`)는 WMI 쿼리를 통해 정보를 얻고, 나머지(`CpuInfo`, `SoundCardInfo`, `VideoCardInfo`, `OsInfo`, `IeInfo`, `DirectXInfo`, `JavaVMInfo`, `InstallSwInfo`)는 CPUID·레지스트리·Win32 API 등 WMI가 아닌 별도 경로로 정보를 수집합니다.

> ⚠️ 이 변수들은 `CWmi Wmi` 선언(3.7절)보다 **먼저** 선언되어 있는데, 정작 `Wmi`를 인자로 받는 위 클래스들은 뒤에서 `Wmi`가 생성된 이후에만 사용됩니다. 순서 자체가 버그는 아니지만, 가독성 측면에서 "Wmi를 쓰는 클래스"와 "쓰지 않는 클래스"를 나눠 선언하면 더 명확해질 수 있습니다.

### 3.4 winmgmt 서비스 자동 시작 설정

```cpp
SC_HANDLE	hScm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
SC_HANDLE	hService = OpenService(hScm, VS_SEVICE_TITLE, SC_MANAGER_ALL_ACCESS);
if( hService )
{
	ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	CloseServiceHandle(hService);
}
if( hScm )
{
	CloseServiceHandle(hScm);
}
```
- 이후 WMI 쿼리가 정상 동작하려면 `winmgmt` 서비스가 실행 중이어야 하므로, 이 서비스의 시작 유형을 **자동 시작(`SERVICE_AUTO_START`)**으로 미리 바꿔둡니다.
- `OpenSCManager`/`OpenService`가 실패해도(`hScm`/`hService`가 NULL이어도) 프로그램은 계속 진행됩니다 — 관리자 권한이 없는 환경에서는 이 블록이 조용히 실패할 수 있으며, 이는 치명적 오류로 취급되지 않고 넘어갑니다.
- 반환값(`BOOL`)을 검사하지 않으므로, `ChangeServiceConfig` 자체가 실패해도 알 수 없습니다.

### 3.5 로그 매니저 초기화

```cpp
// EXE가 위치한 디렉터리를 구해 그 아래에 "Log\" 경로를 만듭니다.
TCHAR tszExePath[MAX_PATH] = { 0, };
DWORD dwLen = GetModuleFileName(NULL, tszExePath, MAX_PATH);
if( dwLen == 0 || dwLen == MAX_PATH )
{
	// 조회 실패 또는 경로가 MAX_PATH를 넘어 잘린 경우: 현재 작업 디렉터리로 대체
	_tcscpy_s(tszExePath, MAX_PATH, _T(".\\"));
}

_tstring strExePath = tszExePath;
size_t nPos = strExePath.find_last_of(_T("\\/"));
_tstring strExeDir = (nPos != _tstring::npos) ? strExePath.substr(0, nPos + 1) : _T(".\\");

_tstring strLogPath = strExeDir + _T("Log\\");

CreateDirectory(strLogPath.c_str(), NULL);

CLogManager::Instance().Create(strLogPath.c_str());
```
- 실행 파일 자신의 전체 경로를 `GetModuleFileName(NULL, ...)`으로 얻은 뒤, 마지막 `\`/`/` 위치까지 잘라 **exe가 있는 디렉터리 밑의 `Log\`** 경로를 로그 저장 위치로 사용합니다. 배포 환경마다 드라이브 구성이 다를 수 있으므로, 특정 드라이브 경로에 의존하지 않고 실행 파일 기준 상대 위치를 씁니다.
- `dwLen == 0`(조회 실패)이거나 `dwLen == MAX_PATH`(경로가 버퍼 크기와 정확히 같음 — 잘렸을 가능성)인 경우, 현재 작업 디렉터리(`.\`)로 안전하게 대체합니다.
- 실제 `Log.cpp`(`CLog::Write()`)를 확인한 결과, **대상 디렉터리를 자동으로 만들어주지 않습니다**:
  ```cpp
  FILE* fp = nullptr;
  _tfopen_s(&fp, tszFullPath, _T("a+"));
  if( fp == 0x00 ) return;
  ```
  폴더가 없으면 `fopen`이 그냥 실패하고 **에러 표시조차 없이 조용히 리턴**합니다. 즉 대상 폴더가 미리 존재하지 않으면 로그가 소리 없이 전부 유실됩니다. 그래서 `CLogManager::Instance().Create()` 호출 전에 `CreateDirectory()`를 먼저 호출해 폴더를 만들어 둡니다(이미 존재하면 실패하지만 무시해도 무방합니다).
- `CLog::Write()`가 디렉터리 문자열과 파일명을 `_T("%s%s")`로 그대로 이어붙이므로, `strLogPath`는 반드시 `\`로 끝나야 합니다(`strExeDir + _T("Log\\")`로 보장).
- `CLog`의 `_tszDirectory` 멤버는 `TCHAR[DIRECTORY_STRLEN]`(`BaseDefine.h` 기준 256) 고정 버퍼이고 내부에서 `_tcsncpy_s(..., _TRUNCATE)`로 안전하게 잘라 복사하므로 오버플로우는 없지만, 경로가 극단적으로 깊으면 잘린 채로 저장될 수 있습니다(실무에서는 거의 발생하지 않는 경우).

### 3.6 COM 초기화 및 보안 설정

```cpp
HRESULT hrCom = CoInitializeEx(NULL, COINIT_MULTITHREADED);
if( FAILED(hrCom) )
{
	LOG_ERROR(_T("CoInitializeEx Failed. HRESULT: 0x%08X"), hrCom);
	return -1;
}

HRESULT hr = CoInitializeSecurity(
	NULL, -1, NULL, NULL,
	RPC_C_AUTHN_LEVEL_DEFAULT,
	RPC_C_IMP_LEVEL_IMPERSONATE,
	NULL, EOAC_NONE, NULL
);

if( FAILED(hr) )
{
	LOG_ERROR(_T("CoInitializeSecurity Failed. HRESULT: 0x%08X"), hr);
	CoUninitialize();
	return -1;
}
```
- **`CoInitializeEx`**: 현재 스레드에서 COM 라이브러리를 멀티스레드 아파트먼트(MTA) 모델로 초기화합니다. WMI(및 그 하위의 `CWmi` 클래스)는 COM 기반이므로, WMI를 쓰기 전에 반드시 COM이 초기화되어 있어야 합니다.
- **`CoInitializeSecurity`**: 프로세스 차원의 COM 보안 수준(인증 레벨, 위장 레벨 등)을 한 번만 설정합니다. WMI 원격/로컬 호출 시 필요한 기본 보안 컨텍스트를 구성합니다.
- 두 호출 모두 실패 시 `LOG_ERROR`로 원인(HRESULT)을 남기고 `-1`을 반환하며 프로그램을 종료합니다. `CoInitializeSecurity` 실패 시에는 이미 성공한 `CoInitializeEx`를 짝 맞춰 `CoUninitialize()`로 정리한 뒤 종료합니다.
- **COM 생명주기 설계**: 이 파일에서 `CoInitializeEx`/`CoUninitialize`를 프로세스(main 함수) 차원에서 한 번만 관리하고, `CWmi` 클래스 자신은 COM을 초기화하지 않는 구조입니다. 이는 "COM 초기화 책임을 한 곳(main)에 집중시키는" 설계로, 여러 하위 컴포넌트가 각자 COM을 초기화/해제하려다 충돌하는 것을 방지합니다.

### 3.7 CWmi 스코프와 정보 수집 본문

```cpp
{
	CWmi Wmi;

	if( !Wmi.Connect() )
	{
		LOG_ERROR(_T("WMI Connection Failed."));
		CoUninitialize();
		return -1;
	}

	// 1~18번 섹션...

} // Wmi 소멸 (COM이 아직 살아있는 상태에서 안전하게 Release())

CoUninitialize();
```

**핵심 설계 포인트 — 중첩 스코프(`{ }`)의 역할:**

`CWmi Wmi;`가 별도의 중괄호 블록 안에 있는 것은 우연이 아니라 의도된 설계입니다.

- C++에서 지역 변수는 함수가 끝날 때가 아니라, **자신이 선언된 스코프(`{ }`)를 벗어나는 시점**에 소멸자가 호출됩니다.
- `CWmi`의 소멸자는 내부적으로 COM 인터페이스(`IWbemLocator`, `IWbemServices` 등)의 `Release()`를 호출하는데, 이 `Release()` 호출이 유효하려면 **그 시점에 COM이 아직 살아있어야(`CoUninitialize()`가 호출되기 전이어야)** 합니다.
- 만약 `Wmi`를 이 중첩 스코프 없이 `main()`의 최상위에서 선언했다면, `CoUninitialize()`가 먼저 실행된 뒤(함수 끝의 `return` 처리 과정에서) `Wmi`의 소멸자가 나중에 호출되어, **이미 종료된 COM 위에서 `Release()`를 호출하는 정의되지 않은 동작(크래시 위험)**이 발생할 수 있습니다.
- 이 코드는 `Wmi`를 명시적으로 좁은 스코프에 가둬 `}`에서 먼저 소멸시키고, 그 다음 줄에서 `CoUninitialize()`를 호출함으로써 **"COM이 살아있는 동안에만 COM 리소스가 정리되도록"** 순서를 보장합니다.

### 3.8 18개 정보 수집 섹션 공통 패턴

각 섹션은 대체로 다음과 같은 동일한 패턴을 따릅니다:

```cpp
// N. XXX INFORMATION
LOG_WRITE(..., _T("*** 구분선/제목 ***"));

XxxInfo.GetInformation(Wmi);              // (WMI 미사용 클래스는 인자 없음)
const std::vector<HWINFO_XXX*>* pVector = XxxInfo.GetXxxArray();  // 배열형 정보만 해당

if( pVector )
{
	for( size_t i = 0; i < pVector->size(); ++i )
	{
		// 항목별 LOG_WRITE 출력
	}
}

LOG_WRITE(..., _T("--- 구분선 ---\n"));

#ifdef _DEBUG
	PauseConsole(); ClearConsoleScreen();
#endif
```

**항목별 형태 분류:**

| 유형 | 특징 | 해당 항목 |
|---|---|---|
| 단일 값 | `GetXxxInfo()` 호출 후 Getter들로 값 하나씩 출력 | BIOS, MAINBOARD, SOUNDCARD, KEYBOARD, MOUSE, OS, IE, DIRECTX |
| 배열(포인터 벡터) | `Get*Array()`로 `vector<T*>` 를 받아 반복 출력 | MEMORY(RAM), DRIVES, LOGICAL DISK, VIDEO, NETWORKCARD, CDROM, MONITOR, INSTALL SOFTWARE |
| 특수 처리 | 별도 가공 로직 포함 | PROCESSOR(문자열 치환), JAVAVM(분기 출력) |

`_DEBUG` 빌드에서만 각 섹션 뒤에 `PauseConsole()`로 사용자 입력을 대기하고 `ClearConsoleScreen()`으로 화면을 지웁니다(릴리즈 빌드에서는 이 블록 자체가 컴파일에서 제외됨).

> 💡 **`system()` 대신 `PauseConsole()`/`ClearConsoleScreen()`을 쓰는 이유**: 이 파일에는 `PauseConsole(); ClearConsoleScreen();`이 총 **17곳**(섹션마다 1번, 마지막 섹션 제외)에 반복됩니다. `system("pause")`/`system("cls")`처럼 셸을 거치는 방식은 호출될 때마다 새 `cmd.exe` 프로세스를 fork/exec 해야 해서, 콘솔 창 하나 지우고 키 입력 하나 기다리는 작업치고는 비용이 과합니다(17회 반복이면 더더욱). 그래서 이 두 함수는 프로세스 생성 없이 동일한 동작을 구현합니다.
>   - `ClearConsoleScreen()`: Win32 콘솔 API(`FillConsoleOutputCharacter` 등)로 현재 버퍼를 직접 채워 지우고 커서만 원점으로 되돌립니다.
>   - `PauseConsole()`: `<conio.h>`의 `_getch()`로 Enter 없이 키 입력 1회만 즉시 감지합니다. 추가 개행 소모 없음, 프로세스 생성 없음.
>
> 두 함수는 `InitUtf8Console()`과 함께 `Util/ConsoleUtil.h`에 정의되어 있습니다(4.5절 참고) — 콘솔 관련 유틸리티는 이 도구뿐 아니라 다른 곳에서도 재사용될 가능성이 높고, 매크로가 아닌 실제 함수로 둬야 호출부(17곳)마다 코드가 중복 생성되지 않기 때문입니다.

### 3.9 섹션별 세부 내용

#### ① BIOS INFORMATION
```cpp
BiosInfo.GetInformation(Wmi);
```
`Manufacturer`, `SmVersion`, `Version`, `IdentificationCode`, `SerialNumber`, `ReleaseDate` 6개 항목을 WMI(`Win32_BIOS` 등으로 추정)를 통해 조회해 출력합니다.

#### ② PROCESSOR INFORMATION — 문자열 치환 로직 포함
```cpp
CpuInfo.GetInformation();

TCHAR tszBuffer1[3] = { ' ', ' ', '\0' };  // 공백 2칸

_tstring strCPUName = CpuInfo.GetProcessorName();
const _tstring strTarget = tszBuffer1;      // "  " (공백 2칸)
const _tstring strReplace = _T("");

if( !strTarget.empty() )
{
	size_t nPos = 0;
	while( (nPos = strCPUName.find(strTarget, nPos)) != _tstring::npos )
	{
		strCPUName.replace(nPos, strTarget.length(), strReplace);
		nPos += strReplace.length();
	}
}

LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("Vendor Name = %s"), CpuInfo.GetVendorName());
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("Processor Name = %s"), strCPUName.c_str());
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("Speed (MHz) = %d MHz"), CpuInfo.GetSpeedMHz());
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("Processors Count = %d"), CpuInfo.GetNumberOfProcessors());
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("Signature Info = Family %d, Model %d, Stepping %d"), CpuInfo.GetCPUFamily(), CpuInfo.GetCPUModel(), CpuInfo.GetCPUStepping());

LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("Instruction Set Flags:"));
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("    * MMX = %s"), CpuInfo.IsMMXSupported() ? _T("Supported") : _T("Not Supported"));
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("    * SSE = %s"), CpuInfo.IsSSESupported() ? _T("Supported") : _T("Not Supported"));
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("    * SSE2 = %s"), CpuInfo.IsSSE2Supported() ? _T("Supported") : _T("Not Supported"));
LOG_WRITE(ELOG_TYPE::LOG_TYPE_INFO, false, _T("    * 3DNow! = %s\n"), CpuInfo.Is3DNowSupported() ? _T("Supported") : _T("Not Supported"));
```
출력 예시:
```
Vendor Name = GenuineIntel
Processor Name = 11th Gen Intel(R) Core(TM) i5-1135G7 @ 2.40GHz
Speed (MHz) = 2419 MHz
Processors Count = 8
Signature Info = Family 6, Model 12, Stepping 1
Instruction Set Flags:
    * MMX = Not Supported
    * SSE = Not Supported
    * SSE2 = Not Supported
    * 3DNow! = Not Supported
```
- CPUID로 얻은 CPU 브랜드 문자열(예: `"Intel(R) Core(TM) i7-...  CPU @ 3.00GHz"`처럼 중간에 **공백 2칸**이 들어가는 경우가 흔함)에서, 연속된 공백 2칸(`"  "`)을 찾아 빈 문자열로 치환(=제거)하는 정리(cleanup) 로직입니다. 정리된 결과가 `Processor Name`으로 출력됩니다.
- `Family`/`Model`/`Stepping`은 개별 항목이 아니라 `Signature Info` 한 줄로 합쳐 출력합니다.
- `Instruction Set Flags`는 `CCpuInfo`가 이미 제공하는 `IsMMXSupported()`/`IsSSESupported()`/`IsSSE2Supported()`/`Is3DNowSupported()` 4개 API를 그대로 호출해 `Supported`/`Not Supported`로 표시합니다. 각 API는 `CCpuInfo::GetInformation()`이 CPUID로 채워둔 `m_Cpu.m_dwFeatures` 플래그를 검사하는 방식으로 동작합니다.
- 이 클래스(`CCpuInfo`)는 WMI가 아니라 CPUID 명령어(`<intrin.h>`)를 통해 직접 정보를 얻습니다(`Wmi` 인자를 받지 않음).

#### ③ MAINBOARD INFORMATION
```cpp
MainBoardInfo.GetInformation(Wmi);
```
`Product`, `SerialNumber`, `Manufacturer`, `Description` 4개 항목 출력.

#### ④ MEMORY INFORMATION — 가장 복잡한 섹션
```cpp
MemoryInfo.GetInformation(Wmi);
const std::vector<HWINFO_RAM*>* psRamVector = MemoryInfo.GetRamArray();
```
- 먼저 장착된 RAM 모듈 각각(`HWINFO_RAM`)에 대해 `BankLabel`, `Name`, `DeviceLocator`, 용량(`ChangeDataFormat`으로 사람이 읽기 좋은 단위로 변환), `FormFactor`, `MemoryType`, `Speed`를 개별 출력합니다.
- 이어서 시스템 전체 메모리 요약(`Total RAM Count`, `Total/Physical/Used Memory Size`, `Total/Free Virtual Memory Size`, `Total/Free PageFile Size`)을 출력합니다.
- `ChangeDataFormat()` 함수(별도 정의)는 바이트 단위 수치를 KB/MB/GB 등으로 변환해 `tszFormat` 버퍼에 채우는 유틸리티로 추정됩니다.

#### ⑤ DRIVES INFORMATION (물리 디스크)
```cpp
HdDiskInfo.GetInformation(Wmi);
```
각 물리 디스크(`HWINFO_HDDISK`)의 `Model`, `Name`, `Manufacturer`, `Description`, 총 용량을 출력합니다.

#### ⑥ LOGICAL DISK INFORMATION (드라이브 문자)
```cpp
DriveInfo.GetInformation(Wmi);
```
각 논리 드라이브(`HWINFO_DRIVE`, 예: `C:`, `D:`)의 이름, 파일시스템, 전체/여유/사용 공간을 출력한 뒤, 전체 드라이브 총합 통계를 출력합니다.

#### ⑦ SOUNDCARD INFORMATION
```cpp
SoundCardInfo.GetInformation();
```
`HasVolumeControl`, `HasSeparateLRVolCtrl`(좌우 볼륨 독립 제어 여부), 제품명, 회사명을 출력합니다. WMI를 사용하지 않는 클래스입니다.

#### ⑧ VIDEO INFORMATION (그래픽카드)
```cpp
VideoCardInfo.GetInformation();
```
장착된 각 그래픽카드(`HWINFO_VIDEOCARD`)의 `Description`, `AdapterString`, `ChipType`, `DacType`, `DisplayDrivers`, 메모리 크기를 출력합니다. WMI 미사용.

#### ⑨ NETWORKCARD INFORMATION
```cpp
NetworkCardInfo.GetInformation(Wmi);
```
각 네트워크 카드의 `Description`만 출력합니다.

#### ⑩ CDROM INFORMATION
```cpp
CdromInfo.GetInformation(Wmi);
```
각 광학 드라이브의 `Name`, `Manufacturer`, `Description`을 출력합니다.

#### ⑪ KEYBOARD INFORMATION / ⑫ MOUSE INFORMATION
단일 키보드/마우스 장치에 대해 각각 `Description`/`Type`(키보드), `Name`/`Manufacturer`/`Description`(마우스)을 출력합니다.

#### ⑬ MONITOR INFORMATION
연결된 각 모니터의 `Manufacturer`, `Description`을 출력합니다.

#### ⑭ OS INFORMATION
```cpp
OsInfo.GetInformation();

LOG_WRITE(..., _T("Description = %s"), OsInfo.GetDescription());

if( OsInfo.Is32bitPlatform() )
	LOG_WRITE(..., _T("Bit Platform = 32Bit Platform"));
else if( OsInfo.Is64bitPlatform() )
	LOG_WRITE(..., _T("Bit Platform = 64Bit Platform"));

LOG_WRITE(..., _T("BuildNumber = %d"), OsInfo.GetBuildNumber());
LOG_WRITE(..., _T("MajorVersion = %d"), OsInfo.GetMajorVersion());
LOG_WRITE(..., _T("MinorVersion = %d"), OsInfo.GetMinorVersion());
LOG_WRITE(..., _T("ServicePack = %s\n"), OsInfo.GetServicePack());
```
- 다른 17개 클래스와 동일하게 `GetInformation()`을 호출한 뒤 Getter로 값을 꺼내는 패턴을 따릅니다. `COsInfo`의 생성자는 멤버를 안전한 기본값으로만 초기화하고, `GetVersionEx`/`RtlGetVersion` 조회를 비롯한 실제 감지는 `GetInformation()` 내부에서 수행됩니다(`GetInformation()`은 버전 조회 성공 여부를 `BOOL`로 반환).
- `GetInformation()` 내부의 `DetectDescription()`은 `GetWindowsVersionDesc()`/`GetWindowsEditionDesc()` 두 private 헬퍼로 `WindowsVersion`/`WindowsEdition` 열거형 값을 각각 문자열로 변환한 뒤 `"%s [%s]"` 형식으로 합쳐 `m_tszDescription`을 채웁니다.
- OS 설명, 32/64비트 여부, 빌드 번호, 메이저/마이너 버전, 서비스팩 정보를 출력합니다.

#### ⑮ IE INFORMATION
```cpp
IeInfo.GetInformation();
```
Internet Explorer의 `Build`, `Version`을 출력합니다.

#### ⑯ DIRECTX INFORMATION
```cpp
DirectXInfo.GetInformation();
```
DirectX `Version`, `InstallVersion`, `Description`을 출력합니다.

#### ⑰ JAVAVM INFORMATION — 분기 출력
```cpp
JavaVMInfo.GetInformation();

if( JavaVMInfo.IsJVM() == 0 )
	LOG_WRITE(..., _T("Not Run Java Virtual Machine"));
else if( JavaVMInfo.IsJVM() == 1 )
	LOG_WRITE(..., _T("Run MS Java Virtual Machine"));
else if( JavaVMInfo.IsJVM() == 2 )
	LOG_WRITE(..., _T("Run SUN Java Virtual Machine"));
else if( JavaVMInfo.IsJVM() == 3 )
	LOG_WRITE(..., _T("Run MS, SUN Java Virtual Machine"));
```
`IsJVM()`이 반환하는 정수 코드(0~3, 비트 플래그 형태로 추정 — MS/SUN 각각 설치 여부의 조합)에 따라 4가지 메시지 중 하나를 출력합니다. 다른 섹션과 달리 개별 값 Getter 대신 **상태 코드 기반 분기**라는 점이 특징입니다. 18개 섹션 중 유일하게 이 섹션 뒤에는 `#ifdef _DEBUG` 일시정지 블록이 없어, 곧바로 다음 섹션(⑱)으로 이어집니다.

#### ⑱ INSTALL SOFTWARE INFORMATION
```cpp
InstallSwInfo.GetInformation();
const std::vector<INSTALL_SWINFO*>* psInstallSwInfoVector = InstallSwInfo.GetInstallSwInfoArray();

if( psInstallSwInfoVector )
{
	for( size_t i = 0; i < psInstallSwInfoVector->size(); ++i )
	{
		INSTALL_SWINFO* pInstallSwInfo = (*psInstallSwInfoVector)[i];
		LOG_WRITE(..., _T("[%zud]. %s"), i + 1, pInstallSwInfo->m_tszDisplayName);
	}
}
```
시스템에 설치된 소프트웨어 목록(레지스트리 `Uninstall` 키 기반으로 추정)을 번호를 매겨 나열합니다. 다른 섹션과 달리 각 항목의 표시 이름(`m_tszDisplayName`)만 출력하고, 별도 상세 정보(설치 경로, 버전 등)는 출력하지 않습니다. 실제 마지막 섹션임에도 `#ifdef _DEBUG` 일시정지 블록이 붙어 있어, `Wmi` 스코프가 닫히기 직전에 `PauseConsole()`/`ClearConsoleScreen()`이 한 번 더 실행됩니다.

### 3.10 종료 처리

```cpp
	} // Wmi 소멸 (COM이 아직 살아있는 상태에서 안전하게 Release())

	CoUninitialize();

#ifndef _DEBUG
	_tprintf(_T("정보 파일이 생성되었습니다: %s\n"), strLogPath.c_str());
	PauseConsole(); ClearConsoleScreen();
#endif

	return 0;
```
- `Wmi`가 스코프를 벗어나며 소멸 → COM 인터페이스 정상 해제
- `CoUninitialize()`로 COM 라이브러리 정리
- **릴리즈 빌드에서만** 결과가 저장된 로그 디렉터리 경로(3.5절에서 계산해 둔 `strLogPath`)를 안내하고, 사용자가 메시지를 읽을 시간을 갖도록 `PauseConsole()`/`ClearConsoleScreen()`을 한 번 더 실행합니다. 릴리즈 빌드는 `_CONSOLE_LOG`가 꺼지고 `_FILE_LOG`만 켜져 있어(4.1절 참고) 각 섹션의 상세 내용이 콘솔에 전혀 출력되지 않으므로, 프로그램이 정상적으로 끝났는지·결과를 어디서 확인해야 하는지 알려주는 마지막 안내가 필요합니다. 디버그 빌드는 이미 각 섹션 내용이 콘솔에 전부 출력되었으므로 이 블록 자체가 컴파일에서 제외됩니다.
- 정상 종료 시 `0` 반환

## 4. 프로젝트 공통 인프라 (`pch.h` 및 공용 헤더)

`SystemInfoTool.cpp`는 `#include <pch.h>` 한 줄 외에 별도 include가 거의 없습니다. 실제로 어떤 기반 위에서 동작하는지 `pch.h`와 `BaseMacro.h`/`BaseDefine.h`/`BaseRedefineDataType.h`를 통해 확인한 내용을 정리합니다.

### 4.1 `pch.h` — 전역 사전 컴파일 헤더

```cpp
#define WIN32_LEAN_AND_MEAN
#define _HAS_STD_BYTE 0

#include <windows.h> ... <mmsystem.h>
#include <BaseDefine.h>
#include <BaseRedefineDataType.h>
#include <BaseMacro.h>

#include <Util/WinCharsetConv.h>
#include <Util/EncodingConvert.h>
#include <Util/StringUtil.h>

#ifdef _DEBUG
#define _CONSOLE_LOG
#else
#define _FILE_LOG
#endif

#include <Util/Log.h>
#include <Util/ConsoleUtil.h>

#include <System/SystemBaseDefine.h>
#include <System/Wmi.h>
#include <System/OsInfo.h>
#include <System/SoftwareInfo.h>
#include <System/CpuInfo.h>
#include <System/HardwareInfo.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "version.lib")

#define  _DEBUGLOG
```

- **`WIN32_LEAN_AND_MEAN`**: `<windows.h>`가 포함하는 부가 기능(소켓, MFC 관련 헤더 등)을 빼서 컴파일 속도를 높이는 표준 관행입니다.
- **`_HAS_STD_BYTE 0`**: C++17의 `std::byte`가 이 프로젝트에서 쓰는 다른 `BYTE`/커스텀 바이트 타입과 이름 충돌을 일으키는 것을 막기 위해 비활성화합니다.
- **`_CONSOLE_LOG`/`_FILE_LOG` 빌드별 전환**: `Util/Log.h`(`CLog::Write()`)가 참조하는 로그 출력 대상 매크로를 `_DEBUG` 정의 여부로 자동 전환합니다 — 디버그는 콘솔에만, 릴리즈는 파일에만 출력합니다(3.8절/3.10절 참고). `Util/Log.h`를 include하기 직전에 정의해, 이 매크로가 필요한 헤더 바로 앞에서 설정 의도가 드러나도록 배치했습니다.
- **`Util/*`, `System/*` include 순서**: `System/Wmi.h` ~ `System/HardwareInfo.h`가 바로 `SystemInfoTool.cpp`에서 쓰는 `CWmi`, `COsInfo`, `CIeInfo`/`CDirectXInfo`/`CJavaVMInfo`/`CInstallSwInfo`(SoftwareInfo.h), `CCpuInfo`, 나머지 하드웨어 클래스(BIOS/MainBoard/Memory/HdDisk/Drive/SoundCard/VideoCard/NetworkCard/Cdrom/KeyBoard/Mouse/Monitor, HardwareInfo.h)들의 실제 출처입니다. `Util/Log.h`가 `LOG_WRITE`/`LOG_ERROR`/`CLogManager`/`ELOG_TYPE`의 출처로 추정되지만, 이 헤더 자체는 이번에 제공되지 않아 정확한 함수 시그니처까지는 확인할 수 없습니다.
- **`#pragma comment(lib, "winmm.lib")`**: `timeGetTime()`을 링크하기 위함입니다 — `CCpuInfo::CalculateCpuSpeedMethod2()`(CPU 클럭 속도 측정 폴백 경로)에서 사용됩니다.
- **`#pragma comment(lib, "version.lib")`**: `GetFileVersionInfoSize()`/`GetFileVersionInfo()`/`VerQueryValue()`를 링크하기 위함입니다 — `SoftwareInfo.cpp`의 `GetVersionLangOfFile()`(JVM 버전 조회 등에 사용)에서 사용됩니다.
- **`_DEBUGLOG`**: 주석상 "모니터링을 위한 로깅 활성화" 스위치로, `Util/Log.h` 내부에서 이 매크로 정의 여부로 로그 출력 동작을 켜고 끄는 것으로 추정됩니다.

### 4.2 `BaseDefine.h` — 전역 상수

이 헤더는 웹 서버, DB, HTTP 통신 등 훨씬 큰 범위의 프레임워크에서 공유하는 상수 모음이며, `SystemInfoTool.cpp`는 이 중 극히 일부만 사용합니다.

| 상수 | 값 | `SystemInfoTool.cpp`에서의 용도 |
|---|---|---|
| `NUMERIC_STRING_LEN` | `20` | `TCHAR tszFormat[NUMERIC_STRING_LEN];` — `ChangeDataFormat()`이 채우는 문자열 버퍼 크기(예: `"512.00 MB"` 같은 문자열이 20자 이내에 들어가야 함) |

나머지(`REGISTRY_*_STRLEN`, `HTTP_*` 계열, `DATABASE_*` 계열 등)는 이 파일에서 직접 쓰이지 않고, `HardwareInfo.cpp`/`SoftwareInfo.cpp` 등 하위 구현 파일에서 레지스트리 버퍼 크기 등으로 사용됩니다.

### 4.3 `BaseRedefineDataType.h` — 크로스플랫폼 타입 정의

- **`_tstring`**: `UNICODE`가 정의되어 있으면 `std::wstring`, 아니면 `std::string`으로 결정됩니다.
  ```cpp
  #ifdef UNICODE
      typedef std::wstring _tstring;
  #else
      typedef std::string  _tstring;
  #endif
  ```
  즉 `strCPUName`(3.9절 ② PROCESSOR INFORMATION)의 실제 타입은 빌드 설정(`UNICODE` 정의 여부)에 따라 `wstring` 또는 `string`으로 갈립니다.
- **`TCHAR`**: Windows 빌드에서는 `<tchar.h>`(pch.h가 직접 include)가 제공하는 타입을 그대로 쓰고, 비Windows 빌드에서는 `UNICODE` 여부에 따라 `wchar_t`/`char`로 이 헤더가 직접 정의합니다 — 크로스플랫폼 포팅을 염두에 둔 흔적입니다.
- **`int8`~`uint64`, `time32`, `time64`, `ulong`** 등: `<cstdint>` 표준 타입 기반의 프로젝트 전용 별칭. `SystemInfoTool.cpp`에서 직접 쓰이진 않지만, 하위 클래스들의 멤버 변수(예: `HWINFO_RAM::m_dwSpeed` 등)에서 쓰일 수 있는 공통 타입입니다.
- 이 헤더 안에 `using namespace std;`가 있어(주석으로 "추후 제거 권장"이라 명시됨), `SystemInfoTool.cpp`가 별도로 `using namespace std;`를 쓰지 않아도 `pch.h`를 통해 이미 전역에 영향을 미치고 있습니다.

### 4.4 `BaseMacro.h` — 매크로 모음

`SystemInfoTool.cpp`에서 실제로 쓰는 것은 없습니다(콘솔 유틸리티는 4.5절의 `Util/ConsoleUtil.h`에 있습니다). `NAMESPACE_BEGIN`/`SVR`/`SERVER_CONFIG`/`_STOMP` 등 `SystemInfoTool.cpp`와 무관한 서버 프레임워크용 매크로들도 함께 정의되어 있으나 이 표에서는 생략했습니다.

| 매크로 | 역할 |
|---|---|
| `SAFE_DELETE(p)` / `SAFE_DELETE_ARRAY(p)` | NULL 체크 후 `delete`/`delete[]`, 포인터를 `nullptr`로 초기화 |
| `CRASH` / `CRASH_CAUSE(cause)` / `ASSERT_CRASH(expr)` | 의도적으로 널 포인터 역참조를 일으켜 크래시 덤프를 발생시키는 디버깅용 매크로 |
| `_tmemset`/`_tmemcpy`/`__TFUNCTION__` | `UNICODE` 여부에 따라 `memset`/`wmemset` 등으로 분기 |
| `_GetTickCount` | Windows 버전(`_WIN32_WINNT >= 0x0600`, Vista 이상)에 따라 `GetTickCount64`/`GetTickCount`로 분기 |
| `LIB_NAME(LIB)` | 빌드 구성(x86/x64, Debug/Release)에 맞는 라이브러리 파일명 문자열 생성 (예: `"Foo64D.lib"`) |

### 4.5 `Util/ConsoleUtil.h` — 콘솔 유틸리티

`InitUtf8Console()`, `ClearConsoleScreen()`, `PauseConsole()` 세 함수를 한데 모은 헤더입니다. `pch.h`가 `Util/Log.h` 바로 다음 줄에 `#include <Util/ConsoleUtil.h>`를 두어, 이 헤더를 포함하는 모든 파일에서 세 함수를 즉시 쓸 수 있습니다.

```cpp
inline void InitUtf8Console() { ... }     // 3.2절 — UTF-8 콘솔 입출력 설정
inline void ClearConsoleScreen() { ... }  // 3.8절 — 콘솔 화면 지우기
inline void PauseConsole() { ... }        // 3.8절 — 키 입력 대기
```

**왜 매크로가 아니라 `inline` 함수로 만들었는가:**

1. **재사용성**
   1-1. 세 함수는 콘솔을 다루는 다른 도구에서도 똑같이 필요할 가능성이 높은 범용 유틸리티입니다.
   1-2. 파일마다 `static`으로 각자 정의하면 여러 `.cpp`에 같은 코드가 흩어져 중복이 발생합니다.
2. **매크로 대신 함수로 통일**
   2-1. `ClearConsoleScreen()`/`PauseConsole()`처럼 호출부가 많은(이 파일 기준 17회) 로직을 매크로로 두면, 전처리기가 호출부마다 본문을 그대로 복사해 바이너리 크기가 호출 횟수만큼 불어납니다. 함수로 두면 구현은 한 곳에만 존재하고 호출부는 `call` 하나씩만 남습니다.
   2-2. 매크로는 이름이 같은 실제 함수와 뒤섞이면 어느 쪽이 호출되는지 헷갈리는 문제가 있어(전처리기가 텍스트를 먼저 치환), 이름 충돌 여지를 없앱니다.
3. **헤더에 정의하면서도 여러 `.cpp`에서 문제없이 쓰기 위해**
   3-1. 모든 함수에 `inline`을 붙여, 이 헤더를 include하는 모든 번역 단위(`Wmi.cpp`, `HardwareInfo.cpp`, `SystemInfoTool.cpp` 등)에서 각자 정의를 갖더라도 링커 단계에서 중복 정의(ODR 위반) 오류가 나지 않도록 합니다.

`ClearConsoleScreen()`/`PauseConsole()`은 Windows 전용 구현(Win32 콘솔 API, `_getch()`)이며, `#if defined(_WIN32) || defined(_WIN64)` 분기 밖(비Windows)에서는 `ClearConsoleScreen()`은 아무 동작도 하지 않고, `PauseConsole()`은 표준 입력 `getchar()`로 대체됩니다.

## 5. 사용되는 매크로 · 유틸리티 (확정 정보)

이 파일에서 쓰이지만 정의는 다른 곳에 있는 심볼들을 정리합니다. 4절에서 확인한 헤더로 실제 정의를 찾은 것은 "확정", `Util/Log.h`처럼 내용이 제공되지 않아 여전히 추정에 머무는 것은 "추정"으로 표시합니다.

| 심볼 | 정의 위치 | 역할 |
|---|---|---|
| `LOG_WRITE(type, flag, fmt, ...)` | `Util/Log.h` (확정 — `Log.h` 확인 완료) | `CLogManager::Instance().Write(...)`를 호출하는 매크로 |
| `LOG_ERROR(fmt, ...)` | `Util/Log.h` (확정) | `ELOG_TYPE::LOG_TYPE_ERROR`로 고정된 로그 매크로 |
| `ELOG_TYPE::LOG_TYPE_INFO` | `Util/Log.h` (확정) | 로그 레벨 열거형 (`DEBUG`/`TRACE`/`INFO`/`WARNING`/`ERROR`) |
| `CLogManager::Instance()` | `Util/Log.h` (확정) | 싱글톤 로그 매니저. 로그 타입별 `CLog` 인스턴스 5개를 내부 배열로 관리 |
| `CWmi` | `System/Wmi.h` (확정) | WMI 연결/쿼리 래퍼 클래스 (`Connect()`, `ExecQuery()`, `GetProperties()` 등 보유) |
| `ChangeDataFormat(int64, TCHAR*)` | `System/HardwareInfo.h` (확정) | 바이트 수치를 KB/MB/GB 등 사람이 읽기 좋은 단위 문자열로 변환 |
| `_tstring` | `BaseRedefineDataType.h` (확정) | `UNICODE` 정의 시 `std::wstring`, 아니면 `std::string` |
| `NUMERIC_STRING_LEN` | `BaseDefine.h` (확정, 값 `20`) | `tszFormat` 버퍼 크기 상수 |
| `TCHAR` | `<tchar.h>`(Windows) / `BaseRedefineDataType.h`(비Windows) (확정) | 빌드 설정(`UNICODE`)에 따라 `wchar_t`/`char`로 갈리는 프로젝트 공통 문자 타입 |
| `InitUtf8Console()` / `ClearConsoleScreen()` / `PauseConsole()` | `Util/ConsoleUtil.h` (확정) | UTF-8 콘솔 설정 / 화면 지우기 / 키 입력 대기 (4.5절 참고) |

`Log.h`/`Log.cpp` 확인 결과 보충 설명:
- `CLog::Write()`는 `_FILE_LOG`/`_CONSOLE_LOG`/`_OUTPUT_LOG` 세 매크로로 파일 기록·콘솔 출력·`OutputDebugString` 출력을 각각 켜고 끕니다. 이 프로젝트에서는 `pch.h`가 `_DEBUG` 정의 여부에 따라 `_CONSOLE_LOG`(디버그) 또는 `_FILE_LOG`(릴리즈) 중 하나만 켜도록 조건부로 정의합니다(4.1절 참고) — 두 매크로가 동시에 켜지는 경우는 없습니다.
- `CLog::_tszDirectory`는 `TCHAR[DIRECTORY_STRLEN]`(256), `_tszFileNamePrefix`는 `TCHAR[FILENAME_STRLEN - DATETIME_STRLEN]`(약 227)로, 둘 다 `Init()` 내부에서 `_tcsncpy_s(..., _TRUNCATE)`로 안전하게 잘려 복사됩니다.
- 파일 쓰기용 `_mutex`(인스턴스별)와 콘솔 출력용 `s_consoleMutex`(전역 정적, 모든 `CLog` 인스턴스 공유)가 분리되어 있어, 여러 로그 레벨에서 동시에 파일을 써도 서로 막지 않으면서 콘솔 출력(색상 설정~출력~복원 구간)만 전역으로 직렬화됩니다.

## 6. 잠재적 개선 여지 (참고)

- `ChangeServiceConfig`, `CLogManager::Instance().Create()`의 반환값을 검사하지 않아, 실패해도 사용자에게 알리지 않고 넘어갑니다.
