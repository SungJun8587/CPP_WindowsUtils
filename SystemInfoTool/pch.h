
//***************************************************************************
// pch.h : include file for standard system include files
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN
#define _HAS_STD_BYTE 0			// c++17 �ɼ��� Ȱ��ȭ �� std::byte �� ��Ȱ�� �ϴ� �ɼ�

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

#include <Memory/MemBuffer.h>
#include <Util/ConvertCharset.h>
#include <Util/StringUtil.h>
#include <Util/Regular.h>
#include <Util/BaseFile.h>
#include <Util/EventLog.h>
#include <BaseLinkedList.h>

#include <SystemBaseDefine.h>
#include <OsInfo.h>
#include <SoftwareInfo.h>
#include <CpuInfo.h>
#include <HardwareInfo.h>

#pragma comment(lib, "winmm.lib")			// timeGetTime �Լ��� ����Ϸ��� winmm.lib �ʿ�
#pragma comment(lib, "version.lib")			// GetFileVersionInfoSize �Լ��� ����Ϸ��� version.lib �ʿ�

#define  _DEBUGLOG				  // Enable Logging For Monitoring System