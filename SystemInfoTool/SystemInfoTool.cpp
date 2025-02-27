//***************************************************************************
// SystemInfoTool.cpp : Defines the entry point for the console application.
//
//***************************************************************************

#include <pch.h>

#include <iostream>
#include <vector>
#include <bitset>
#include <array>
#include <string>
#include <intrin.h>

#if defined(_M_IX86)
	#include "CpuInfo86.h"
#else
	#include "CpuInfo64.h"
#endif

using namespace std;

#define VS_SEVICE_TITLE _T("winmgmt")

//***************************************************************************
//
void Print(const TCHAR *ptszBuffer)
{
#ifdef _DEBUG
	setlocale(LC_ALL, "korean");
	_tprintf(_T("%s\n"), ptszBuffer);
#endif
}

void getAMDFeatures() 
{
	unsigned long eax, ebx, ecx, edx;

	// CPUID 지원 여부 확인
	if( !cpu_id_supported() ) 
	{
		std::cout << "CPUID is not supported on this CPU." << std::endl;
		return;
	}

	// AMD 확장 기능 가져오기
	eax = AMD_EXTENDED_FEATURE;
	cpu_id(&eax, &ebx, &ecx, &edx);
	std::cout << "AMD Extended Features:" << std::endl;
	std::cout << "EAX: " << std::hex << eax << std::endl;
	std::cout << "EBX: " << std::hex << ebx << std::endl;
	std::cout << "ECX: " << std::hex << ecx << std::endl;
	std::cout << "EDX: " << std::hex << edx << std::endl;

	// 이름 문자열 가져오기
	eax = NAMESTRING_FEATURE;
	cpu_id(&eax, &ebx, &ecx, &edx);
	std::cout << "\nName String Features:" << std::endl;
	std::cout << "EAX: " << std::hex << eax << std::endl;
	std::cout << "EBX: " << std::hex << ebx << std::endl;
	std::cout << "ECX: " << std::hex << ecx << std::endl;
	std::cout << "EDX: " << std::hex << edx << std::endl;

	// L1 캐시 정보 가져오기
	eax = AMD_L1CACHE_FEATURE;
	cpu_id(&eax, &ebx, &ecx, &edx);
	std::cout << "\nL1 Cache Features:" << std::endl;
	std::cout << "EAX: " << std::hex << eax << std::endl;
	std::cout << "EBX: " << std::hex << ebx << std::endl;
	std::cout << "ECX: " << std::hex << ecx << std::endl;
	std::cout << "EDX: " << std::hex << edx << std::endl;

	// L2 캐시 정보 가져오기
	eax = AMD_L2CACHE_FEATURE;
	cpu_id(&eax, &ebx, &ecx, &edx);
	std::cout << "\nL2 Cache Features:" << std::endl;
	std::cout << "EAX: " << std::hex << eax << std::endl;
	std::cout << "EBX: " << std::hex << ebx << std::endl;
	std::cout << "ECX: " << std::hex << ecx << std::endl;
	std::cout << "EDX: " << std::hex << edx << std::endl;
}

//***************************************************************************
//
int main(int argc, char* argv[])
{
#ifdef	_MSC_VER
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

	//getAMDFeatures();

	char szSource[20] = "안녕123하세요";
	wchar_t wszSource[20] = L"안녕123하세요";

	int nLength = MultiByteToWideChar(CP_ACP, 0, (LPSTR)szSource, -1, NULL, 0);
	nLength = WideCharToMultiByte(CP_ACP, 0, wszSource, -1, NULL, 0, NULL, NULL);
	nLength = static_cast<int>(strlen(szSource));
	nLength = static_cast<int>(wcslen(wszSource));

	bool	fPause = true;
	int		i = 0;

	TCHAR	tszFormat[NUMERIC_STRING_LEN];
	TCHAR	tszBuffer[MAX_BUFFER_SIZE];

	CCpuInfo			CpuInfo;
	CBiosInfo			BiosInfo;
	CMainBoardInfo		MainBoardInfo;
	CMemoryInfo			MemoryInfo;
	CHdDiskInfo			HdDiskInfo;
	CDriveInfo			DriveInfo;
	CSoundCardInfo		SoundCardInfo;
	CVideoCardInfo		VideoCardInfo;
	CNetworkCardInfo	NetworkCardInfo;
	CCdromInfo			CdromInfo;
	CKeyBoardInfo		KeyBoardInfo;
	CMouseInfo			MouseInfo;
	CMonitorInfo		MonitorInfo;

	COsInfo			OsInfo;
	CIeInfo			IeInfo;
	CDirectXInfo	DirectXInfo;
	CJavaVMInfo		JavaVMInfo;
	CInstallSwInfo	InstallSwInfo;

	CMemBuffer<TCHAR> TCPUName;

	CWmi		Wmi;
	CEventLog	EvLog;

	HWINFO_RAM			*pRam = NULL;
	HWINFO_HDDISK		*pHdDisk = NULL;
	HWINFO_DRIVE		*pDrive = NULL;
	HWINFO_VIDEOCARD	*pVideoCard = NULL;
	HWINFO_NETWORKCARD	*pNetworkCard = NULL;
	HWINFO_CDROM		*pCdrom = NULL;
	HWINFO_MONITOR		*pMonitor = NULL;
	INSTALL_SWINFO		*pInstallSwInfo = NULL;

	CBaseLinkedList<HWINFO_RAM *> *psRamArray = NULL;
	CBaseLinkedList<HWINFO_HDDISK *> *psHdDiskArray = NULL;
	CBaseLinkedList<HWINFO_DRIVE *> *psDriveArray = NULL;
	CBaseLinkedList<HWINFO_VIDEOCARD *> *psVideoCardArray = NULL;
	CBaseLinkedList<HWINFO_NETWORKCARD *> *psNetworkCardArray = NULL;
	CBaseLinkedList<HWINFO_CDROM *> *psCdromArray = NULL;
	CBaseLinkedList<HWINFO_MONITOR *> *psMonitorArray = NULL;
	CBaseLinkedList<INSTALL_SWINFO *> *psInstallSwInfoArray = NULL;

	SC_HANDLE	hScm;
	SC_HANDLE	hService;

	hScm = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	hService = OpenService(hScm, VS_SEVICE_TITLE, SC_MANAGER_ALL_ACCESS);
	ChangeServiceConfig(hService, SERVICE_NO_CHANGE, SERVICE_AUTO_START, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	EvLog.InitForLogFile(_T("D:\\"), _T("SystemInfo"), EVENTLOG_FMT_DAILY, TEXT(","));

	HRESULT hr;

	// STEP 2. COM Security Level을 설정한다.
	hr = CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities 
		NULL                         // Reserved
	);
	if( FAILED(hr) )
	{
		CoUninitialize();

		return false;
	}

	Wmi.Connect();

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* BIOS INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	BiosInfo.GetInformation(Wmi);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("BIOS Manufacturer = %s"), BiosInfo.GetManufacturer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("BIOS SmVersion = %s"), BiosInfo.GetSmVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("BIOS Version = %s"), BiosInfo.GetVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("BIOS IdentificationCode = %s"), BiosInfo.GetIdentificationCode());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("BIOS SerialNumber = %s"), BiosInfo.GetSerialNumber());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("BIOS ReleaseDate = %s"), BiosInfo.GetReleaseDate());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* PROCESSOR INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	CpuInfo.GetInformation();

	TCHAR tszBuffer1[3];

	tszBuffer1[0] = ' ';
	tszBuffer1[1] = ' ';
	tszBuffer1[2] = '\0';

	StrReplace(TCPUName, CpuInfo.GetProcessorName(), tszBuffer1, _T(""));

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CPU Name = %s"), TCPUName.GetBuffer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CPU VendorName = %s"), CpuInfo.GetVendorName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CPU Speed = %d MHz"), CpuInfo.GetSpeedMHz());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Number Of Processes = %d"), CpuInfo.GetNumberOfProcessors());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CPU Family = %d"), CpuInfo.GetCPUFamily());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CPU Model = %d"), CpuInfo.GetCPUModel());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CPU Stepping = %d"), CpuInfo.GetCPUStepping());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* MAINBOARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MainBoardInfo.GetInformation(Wmi);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MAINBOARD Product = %s"), MainBoardInfo.GetProduct());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MAINBOARD SerialNumber = %s"), MainBoardInfo.GetSerialNumber());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MAINBOARD Manufacturer = %s"), MainBoardInfo.GetManufacturer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MAINBOARD Description = %s"), MainBoardInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* MEMORY INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MemoryInfo.GetInformation(Wmi);
	psRamArray = MemoryInfo.GetRamArray();

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("--------- RAM INFORMATION ---------"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	for( i = 0; i < psRamArray->GetCount(); i++ )
	{
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("RAM[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pRam = psRamArray->At(i);

		ChangeDataFormat(pRam->m_nCapacity, tszFormat);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Memory BankLabel = %s"), pRam->m_tszBankLabel);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Memory Name = %s"), pRam->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Memory DeviceLocator = %s"), pRam->m_tszDeviceLocator);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Memory Size = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Memory FormFactor = %d[ %s ]"), pRam->m_dwFormFactor, pRam->m_tszFormFactorDesc);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Memory MemoryType = %d[ %s ]"), pRam->m_dwMemoryType, pRam->m_tszMemoryTypeDesc);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Memory Speed = %d"), pRam->m_dwSpeed);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("-------- MEMORY INFORMATION --------"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Total RAM Count = %d"), MemoryInfo.GetRamCount());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetTotalMemSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Total Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetPhysicalMemSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Physical Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetUseMemSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Used Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetTotalVirtualMemSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Total Virtual Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetFreeVirtualMemSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Free Virtual Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetTotalPageFile(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Total PageFile Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetFreePageFile(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Free PageFile Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* DRIVES INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	HdDiskInfo.GetInformation(Wmi);
	psHdDiskArray = HdDiskInfo.GetHdDiskArray();

	for( i = 0; i < psHdDiskArray->GetCount(); i++ )
	{
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("HDDISK[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pHdDisk = psHdDiskArray->At(i);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Model = %s"), pHdDisk->m_tszModel);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Name = %s"), pHdDisk->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Manufacturer = %s"), pHdDisk->m_tszManufacturer);
		EvLog.EventLog(tszBuffer, false);
 		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Description = %s"), pHdDisk->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pHdDisk->m_nTotalSize, tszFormat);
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk TotalSize = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("****************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* LOGICAL DISK INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("****************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	DriveInfo.GetInformation(Wmi);
	psDriveArray = DriveInfo.GetDriveArray();

	for( i = 0; i < psDriveArray->GetCount(); i++ )
	{
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("DRIVE[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pDrive = psDriveArray->At(i);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Drive Name = %s"), pDrive->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Drive FileSystem = %s"), pDrive->m_tszFileSystem);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pDrive->m_nTotalSpace, tszFormat);
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Drive TotalSpace = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pDrive->m_nFreeSpace, tszFormat);
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Drive FreeSpace = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pDrive->m_nTotalSpace - pDrive->m_nFreeSpace, tszFormat);
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Drive UsedSpace = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("-------- DRIVE INFORMATION --------"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Total Drive Count = %d"), DriveInfo.GetDriveCount());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(DriveInfo.GetTotalSpaceSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Total Space Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(DriveInfo.GetFreeSpaceSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Free Space Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(DriveInfo.GetUsedSpaceSize(), tszFormat);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Used Space Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* SOUNDCARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	SoundCardInfo.GetInformation();

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("HasVolumeControl = %d"), SoundCardInfo.HasVolCtrl());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("HasSeparateRLVolCtrl = %d"), SoundCardInfo.HasSeparateLRVolCtrl());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("ProductName = %s"), SoundCardInfo.GetProductName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CompanyName = %s"), SoundCardInfo.GetCompanyName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* VIDEO INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	VideoCardInfo.GetInformation();
	psVideoCardArray = VideoCardInfo.GetVideoCardArray();

	for( i = 0; i < psVideoCardArray->GetCount(); i++ )
	{
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("VIDEO[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pVideoCard = psVideoCardArray->At(i);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Description = %s"), pVideoCard->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("AdapterString = %s"), pVideoCard->m_tszAdapterString);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("ChipType = %s"), pVideoCard->m_tszChipType);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("DacType = %s"), pVideoCard->m_tszDacType);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("DisplayDrivers = %s"), pVideoCard->m_tszDisplayDrivers);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MemorySize = %d"), pVideoCard->m_lMemorySize);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("***************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* NETWORKCARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("***************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	NetworkCardInfo.GetInformation(Wmi);
	psNetworkCardArray = NetworkCardInfo.GetNetworkCardArray();

	for( i = 0; i < psNetworkCardArray->GetCount(); i++ )
	{
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("NETWORKCARD[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pNetworkCard = psNetworkCardArray->At(i);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] NetworkCard Description = %s"), pNetworkCard->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* CDROM INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	CdromInfo.GetInformation(Wmi);
	psCdromArray = CdromInfo.GetCdromArray();

	for( i = 0; i < psCdromArray->GetCount(); i++ )
	{
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("CDROM[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pCdrom = psCdromArray->At(i);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Cdrom Name = %s"), pCdrom->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Cdrom Manufacturer = %s"), pCdrom->m_tszManufacturer);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Cdrom Description = %s"), pCdrom->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* KEYBOARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	KeyBoardInfo.GetInformation(Wmi);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("KeyBoard Description = %s"), KeyBoardInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("KeyBoard Type = %s"), KeyBoardInfo.GetType());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* MOUSE INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MouseInfo.GetInformation(Wmi);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Mouse Name = %s"), MouseInfo.GetName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Mouse Manufacturer = %s"), MouseInfo.GetManufacturer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Mouse Description = %s"), MouseInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* MONITOR INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MonitorInfo.GetInformation(Wmi);
	psMonitorArray = MonitorInfo.GetMonitorArray();

	for( i = 0; i < psMonitorArray->GetCount(); i++ )
	{
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MONITOR[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pMonitor = psMonitorArray->At(i);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Monitor Manufacturer = %s"), pMonitor->m_tszManufacturer);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[#] Monitor Description = %s"), pMonitor->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* OS INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Description = %s"), OsInfo.GetDescription());

	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	if( OsInfo.Is32bitPlatform() )
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Bit Platform = 32Bit Platform"));
	else if( OsInfo.Is64bitPlatform() )
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Bit Platform = 64Bit Platform"));

	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("BuildNumber = %d"), OsInfo.GetBuildNumber());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MajorVersion = %d"), OsInfo.GetMajorVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("MinorVersion = %d"), OsInfo.GetMinorVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("ServicePack = %s"), OsInfo.GetServicePack());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* IE INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	IeInfo.GetInformation();

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("IE Build = %s"), IeInfo.GetBuild());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("IE Version = %s"), IeInfo.GetVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* DIRECTX INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	DirectXInfo.GetInformation();

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("DirectX Version = %s"), DirectXInfo.GetVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("DirectX Install Version = %s"), DirectXInfo.GetInstallVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("DirectX Description = %s"), DirectXInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* JAVAVM INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	JavaVMInfo.GetInformation();

	if( JavaVMInfo.IsJVM() == 0 )
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Not Run Java Virtual Machine"));
	else if( JavaVMInfo.IsJVM() == 1 )
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Run MS Java Virtual Machine"));
	else if( JavaVMInfo.IsJVM() == 2 )
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Run SUN Java Virtual Machine"));
	else if( JavaVMInfo.IsJVM() == 3 )
		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("Run MS, SUN Java Virtual Machine"));

	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("********************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("* INSTALL SOFTWARE INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("********************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	InstallSwInfo.GetInformation();
	psInstallSwInfoArray = InstallSwInfo.GetInstallSwInfoArray();

	for( i = 0; i < psInstallSwInfoArray->GetCount(); i++ )
	{
		pInstallSwInfo = psInstallSwInfoArray->At(i);

		StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("[%d]. %s"), i + 1, pInstallSwInfo->m_tszDisplayName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);
	}

	EvLog.EventLog(_T(""), false);
	StringCchPrintf(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

#ifdef _DEBUG
	if( fPause )
	{
		system("pause");
		system("cls");
	}
#endif

	CloseServiceHandle(hService);
	CloseServiceHandle(hScm);

	return 0;
}

/*
int main(int argc, char* argv[])
{
	cout << "Program Running" << endl;

	DWORD iDevNum = 0;
	DISPLAY_DEVICE lpDisplayDevice;
	DWORD dwFlags = 0;

	lpDisplayDevice.cb = sizeof(lpDisplayDevice);

	while( EnumDisplayDevices( NULL, iDevNum, &lpDisplayDevice, dwFlags ) )
	{
		iDevNum++;

		if( lpDisplayDevice.StateFlags & DISPLAY_DEVICE_ACTIVE
			&& !(lpDisplayDevice.StateFlags & DISPLAY_DEVICE_VGA_COMPATIBLE ) )
		{
			cout << " Display Device : " << iDevNum << endl;
			cout << " Device ID : " << lpDisplayDevice.DeviceID << endl;
			cout << " Device Key : " << lpDisplayDevice.DeviceKey << endl;
			cout << " Device Name : " << lpDisplayDevice.DeviceName << endl;
			cout << " Device Descreption : " << lpDisplayDevice.DeviceString << endl;
			cout << " Device Status : " << lpDisplayDevice.StateFlags << endl << endl;
		}
	}

	if( cpu_id_supported() )
	{
		cout << "CPUID = yes" << endl;
		cout << "AVX = " << (cpu_avx() ? "yes" : "no") << endl;
		cout << "AVX2 = " << (cpu_avx2() ? "yes" : "no") << endl;
		cout << "MMX = " << (cpu_mmx() ? "yes" : "no") << endl;
		cout << "SSE = " << (cpu_sse() ? "yes" : "no") << endl;
		cout << "SSE2 = " << (cpu_sse2() ? "yes" : "no") << endl;
		cout << "SSE3 = " << (cpu_sse3() ? "yes" : "no") << endl;
		cout << "SSSE3 = " << (cpu_ssse3() ? "yes" : "no") << endl;
		cout << "SSE41 = " << (cpu_sse41() ? "yes" : "no") << endl;
		cout << "SSE42 = " << (cpu_sse42() ? "yes" : "no") << endl;
		cout << "HT = " << (cpu_hyperthreading() ? "yes" : "no") << endl;
		cout << "ThreadCount = " << cpu_logical_processor_count() << endl;

		// retrieve the processor brand
		char brand[48 + 1];
		brand[48] = 0;
		if( cpu_brand(brand) )
		{
			cout << "Brand = " << brand << endl;
		}
		cout << endl << "Ctrl+C to quit" << endl;
	}
	else
	{
		cout << "CPUID = no" << endl;
	}
	cin.get();
	return 0;

	if( cpu_id_supported() )
	{
		cout << "CPUID = yes" << endl;
		cout << "AVX = " << (cpu_avx() ? "yes" : "no") << endl;
		cout << "AVX2 = " << (cpu_avx2() ? "yes" : "no") << endl;
		cout << "MMX = " << (cpu_mmx() ? "yes" : "no") << endl;
		cout << "SSE = " << (cpu_sse() ? "yes" : "no") << endl;
		cout << "SSE2 = " << (cpu_sse2() ? "yes" : "no") << endl;
		cout << "SSE3 = " << (cpu_sse3() ? "yes" : "no") << endl;
		cout << "SSSE3 = " << (cpu_ssse3() ? "yes" : "no") << endl;
		cout << "SSE41 = " << (cpu_sse41() ? "yes" : "no") << endl;
		cout << "SSE42 = " << (cpu_sse42() ? "yes" : "no") << endl;
		cout << "HT = " << (cpu_hyperthreading() ? "yes" : "no") << endl;
		cout << "ThreadCount = " << cpu_logical_processor_count() << endl;

		// retrieve the processor brand
		union {
			char brand[48 + 1];
			long long buffer[6];
		};
		brand[48] = 0;
		buffer[0] = cpu_brand_part0();
		buffer[1] = cpu_brand_part1();
		buffer[2] = cpu_brand_part2();
		buffer[3] = cpu_brand_part3();
		buffer[4] = cpu_brand_part4();
		buffer[5] = cpu_brand_part5();
		cout << "Brand = " << brand << endl;
		cout << endl << "Ctrl+C to quit" << endl;
	}
	else
	{
		cout << "CPUID = no" << endl;
	}
	cin.get();
	return 0;
}
*/
