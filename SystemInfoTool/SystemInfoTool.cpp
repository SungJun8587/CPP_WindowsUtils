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
	_tprintf(_T("%s\n"), ptszBuffer);
#endif
}

//***************************************************************************
//
int main(int argc, char* argv[])
{
#ifdef	_MSC_VER
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif

	setlocale(LC_ALL, "korean");

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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* BIOS INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	BiosInfo.GetInformation(Wmi);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("BIOS Manufacturer = %s"), BiosInfo.GetManufacturer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("BIOS SmVersion = %s"), BiosInfo.GetSmVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("BIOS Version = %s"), BiosInfo.GetVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("BIOS IdentificationCode = %s"), BiosInfo.GetIdentificationCode());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("BIOS SerialNumber = %s"), BiosInfo.GetSerialNumber());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("BIOS ReleaseDate = %s"), BiosInfo.GetReleaseDate());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* PROCESSOR INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*************************"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CPU Name = %s"), TCPUName.GetBuffer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CPU VendorName = %s"), CpuInfo.GetVendorName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CPU Speed = %d MHz"), CpuInfo.GetSpeedMHz());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Number Of Processes = %d"), CpuInfo.GetNumberOfProcessors());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CPU Family = %d"), CpuInfo.GetCPUFamily());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CPU Model = %d"), CpuInfo.GetCPUModel());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CPU Stepping = %d"), CpuInfo.GetCPUStepping());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* MAINBOARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MainBoardInfo.GetInformation(Wmi);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MAINBOARD Product = %s"), MainBoardInfo.GetProduct());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MAINBOARD SerialNumber = %s"), MainBoardInfo.GetSerialNumber());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MAINBOARD Manufacturer = %s"), MainBoardInfo.GetManufacturer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MAINBOARD Description = %s"), MainBoardInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* MEMORY INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MemoryInfo.GetInformation(Wmi);
	psRamArray = MemoryInfo.GetRamArray();

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("--------- RAM INFORMATION ---------"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	for( i = 0; i < psRamArray->GetCount(); i++ )
	{
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("RAM[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pRam = psRamArray->At(i);

		ChangeDataFormat(pRam->m_nCapacity, tszFormat);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Memory BankLabel = %s"), pRam->m_tszBankLabel);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Memory Name = %s"), pRam->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Memory DeviceLocator = %s"), pRam->m_tszDeviceLocator);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Memory Size = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Memory FormFactor = %d[ %s ]"), pRam->m_dwFormFactor, pRam->m_tszFormFactorDesc);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Memory MemoryType = %d[ %s ]"), pRam->m_dwMemoryType, pRam->m_tszMemoryTypeDesc);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Memory Speed = %d"), pRam->m_dwSpeed);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("-------- MEMORY INFORMATION --------"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Total RAM Count = %d"), MemoryInfo.GetRamCount());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetTotalMemSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Total Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetPhysicalMemSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Physical Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetUseMemSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Used Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetTotalVirtualMemSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Total Virtual Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetFreeVirtualMemSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Free Virtual Memory Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetTotalPageFile(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Total PageFile Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(MemoryInfo.GetFreePageFile(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Free PageFile Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* DRIVES INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	HdDiskInfo.GetInformation(Wmi);
	psHdDiskArray = HdDiskInfo.GetHdDiskArray();

	for( i = 0; i < psHdDiskArray->GetCount(); i++ )
	{
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("HDDISK[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pHdDisk = psHdDiskArray->At(i);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Model = %s"), pHdDisk->m_tszModel);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Name = %s"), pHdDisk->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Manufacturer = %s"), pHdDisk->m_tszManufacturer);
		EvLog.EventLog(tszBuffer, false);
 		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk Description = %s"), pHdDisk->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pHdDisk->m_nTotalSize, tszFormat);
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] HdDisk TotalSize = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("****************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* LOGICAL DISK INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("****************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	DriveInfo.GetInformation(Wmi);
	psDriveArray = DriveInfo.GetDriveArray();

	for( i = 0; i < psDriveArray->GetCount(); i++ )
	{
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("DRIVE[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pDrive = psDriveArray->At(i);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Drive Name = %s"), pDrive->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Drive FileSystem = %s"), pDrive->m_tszFileSystem);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pDrive->m_nTotalSpace, tszFormat);
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Drive TotalSpace = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pDrive->m_nFreeSpace, tszFormat);
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Drive FreeSpace = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		ChangeDataFormat(pDrive->m_nTotalSpace - pDrive->m_nFreeSpace, tszFormat);
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Drive UsedSpace = %s"), tszFormat);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("-------- DRIVE INFORMATION --------"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Total Drive Count = %d"), DriveInfo.GetDriveCount());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(DriveInfo.GetTotalSpaceSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Total Space Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(DriveInfo.GetFreeSpaceSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Free Space Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	ChangeDataFormat(DriveInfo.GetUsedSpaceSize(), tszFormat);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Used Space Size = %s"), tszFormat);
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* SOUNDCARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	SoundCardInfo.GetInformation();

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("HasVolumeControl = %d"), SoundCardInfo.HasVolCtrl());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("HasSeparateRLVolCtrl = %d"), SoundCardInfo.HasSeparateLRVolCtrl());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("ProductName = %s"), SoundCardInfo.GetProductName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CompanyName = %s"), SoundCardInfo.GetCompanyName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* VIDEO INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	VideoCardInfo.GetInformation();
	psVideoCardArray = VideoCardInfo.GetVideoCardArray();

	for( i = 0; i < psVideoCardArray->GetCount(); i++ )
	{
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("VIDEO[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pVideoCard = psVideoCardArray->At(i);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Description = %s"), pVideoCard->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("AdapterString = %s"), pVideoCard->m_tszAdapterString);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("ChipType = %s"), pVideoCard->m_tszChipType);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("DacType = %s"), pVideoCard->m_tszDacType);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("DisplayDrivers = %s"), pVideoCard->m_tszDisplayDrivers);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MemorySize = %d"), pVideoCard->m_lMemorySize);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("***************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* NETWORKCARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("***************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	NetworkCardInfo.GetInformation(Wmi);
	psNetworkCardArray = NetworkCardInfo.GetNetworkCardArray();

	for( i = 0; i < psNetworkCardArray->GetCount(); i++ )
	{
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("NETWORKCARD[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pNetworkCard = psNetworkCardArray->At(i);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] NetworkCard Description = %s"), pNetworkCard->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* CDROM INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	CdromInfo.GetInformation(Wmi);
	psCdromArray = CdromInfo.GetCdromArray();

	for( i = 0; i < psCdromArray->GetCount(); i++ )
	{
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("CDROM[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pCdrom = psCdromArray->At(i);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Cdrom Name = %s"), pCdrom->m_tszName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Cdrom Manufacturer = %s"), pCdrom->m_tszManufacturer);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Cdrom Description = %s"), pCdrom->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* KEYBOARD INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	KeyBoardInfo.GetInformation(Wmi);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("KeyBoard Description = %s"), KeyBoardInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("KeyBoard Type = %s"), KeyBoardInfo.GetType());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* MOUSE INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("*********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MouseInfo.GetInformation(Wmi);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Mouse Name = %s"), MouseInfo.GetName());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Mouse Manufacturer = %s"), MouseInfo.GetManufacturer());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Mouse Description = %s"), MouseInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* MONITOR INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	MonitorInfo.GetInformation(Wmi);
	psMonitorArray = MonitorInfo.GetMonitorArray();

	for( i = 0; i < psMonitorArray->GetCount(); i++ )
	{
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MONITOR[%d]"), i + 1);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		pMonitor = psMonitorArray->At(i);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Monitor Manufacturer = %s"), pMonitor->m_tszManufacturer);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[#] Monitor Description = %s"), pMonitor->m_tszDescription);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);

		EvLog.EventLog(_T(""), false);

		Print(_T(""));
	}

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* OS INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Description = %s"), OsInfo.GetDescription());

	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	if( OsInfo.Is32bitPlatform() )
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Bit Platform = 32Bit Platform"));
	else if( OsInfo.Is64bitPlatform() )
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Bit Platform = 64Bit Platform"));

	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("BuildNumber = %d"), OsInfo.GetBuildNumber());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MajorVersion = %d"), OsInfo.GetMajorVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("MinorVersion = %d"), OsInfo.GetMinorVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("ServicePack = %s"), OsInfo.GetServicePack());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* IE INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("******************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	IeInfo.GetInformation();

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("IE Build = %s"), IeInfo.GetBuild());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("IE Version = %s"), IeInfo.GetVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* DIRECTX INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("***********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	DirectXInfo.GetInformation();

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("DirectX Version = %s"), DirectXInfo.GetVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("DirectX Install Version = %s"), DirectXInfo.GetInstallVersion());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("DirectX Description = %s"), DirectXInfo.GetDescription());
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* JAVAVM INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("**********************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	JavaVMInfo.GetInformation();

	if( JavaVMInfo.IsJVM() == 0 )
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Not Run Java Virtual Machine"));
	else if( JavaVMInfo.IsJVM() == 1 )
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Run MS Java Virtual Machine"));
	else if( JavaVMInfo.IsJVM() == 2 )
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Run SUN Java Virtual Machine"));
	else if( JavaVMInfo.IsJVM() == 3 )
		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("Run MS, SUN Java Virtual Machine"));

	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(_T(""));
	Print(tszBuffer);
	Print(_T(""));

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("********************************"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("* INSTALL SOFTWARE INFORMATION *"));
	EvLog.EventLog(tszBuffer, false);
	Print(tszBuffer);

	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("********************************"));
	EvLog.EventLog(tszBuffer, false);
	EvLog.EventLog(_T(""), false);
	Print(tszBuffer);
	Print(_T(""));

	InstallSwInfo.GetInformation();
	psInstallSwInfoArray = InstallSwInfo.GetInstallSwInfoArray();

	for( i = 0; i < psInstallSwInfoArray->GetCount(); i++ )
	{
		pInstallSwInfo = psInstallSwInfoArray->At(i);

		_stprintf_s(tszBuffer, _countof(tszBuffer), _T("[%d]. %s"), i + 1, pInstallSwInfo->m_tszDisplayName);
		EvLog.EventLog(tszBuffer, false);
		Print(tszBuffer);
	}

	EvLog.EventLog(_T(""), false);
	_stprintf_s(tszBuffer, _countof(tszBuffer), _T("---------------------------------------------------------"));
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