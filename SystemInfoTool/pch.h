
//***************************************************************************
// pch.h : include file for standard system include files
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN
#define _HAS_STD_BYTE 0			// c++17 옵션을 활성화 시 std::byte 를 비활성 하는 옵션

#include <windows.h>
#include <atlbase.h>
#include <comdef.h>
#include <tchar.h>
#include <crtdbg.h>
#include <time.h>
#include <typeinfo>
#include <malloc.h>
#include <locale.h> 
#include <strsafe.h>
#include <mmsystem.h>

#pragma warning(disable : 4251 4800)

#include <BaseDefine.h>
#include <BaseRedefineDataType.h>
#include <BaseMacro.h>

#include <Util/WinCharsetConv.h>
#include <Util/EncodingConvert.h>
#include <Util/StringUtil.h>

// 로그 출력 대상을 빌드 구성에 따라 자동으로 전환합니다(Util/Log.h가 참조하는
// 매크로이므로 include 바로 앞에 정의합니다).
// - _DEBUG 빌드: 콘솔에만 출력(파일 미생성) — 개발 중 즉시 확인 용도
// - Release 빌드: 파일에만 출력(콘솔 미출력) — 배포 환경에서는 결과 파일 경로만
//   별도로 안내하고, 상세 내용은 파일로 남깁니다(SystemInfoTool.cpp 참고)
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
#include <System/HwInfoStructs.h>
#include <System/WmiHardwareInfo.h>
#include <System/DeviceInfo.h>

#pragma comment(lib, "winmm.lib")			// timeGetTime 함수를 사용하려면 winmm.lib 필요
#pragma comment(lib, "version.lib")			// GetFileVersionInfoSize 함수를 사용하려면 version.lib 필요

#define  _DEBUGLOG				  // Enable Logging For Monitoring System