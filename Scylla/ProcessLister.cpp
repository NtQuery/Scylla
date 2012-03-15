#include "ProcessLister.h"

#include "SystemInformation.h"
#include "Logger.h"
#include "ProcessAccessHelp.h"

#include <algorithm>

//#define DEBUG_COMMENTS

def_IsWow64Process ProcessLister::_IsWow64Process = 0;

std::vector<Process>& ProcessLister::getProcessList()
{
	return processList;
}

bool ProcessLister::isWindows64()
{
#ifdef _WIN64
	//compiled 64bit application
	return true;
#else
	//32bit exe, check wow64
	BOOL bIsWow64 = FALSE;

	//not available in all windows operating systems
	//Minimum supported client: Windows Vista, Windows XP with SP2
	//Minimum supported server: Windows Server 2008, Windows Server 2003 with SP1

	if (_IsWow64Process)
	{
		_IsWow64Process(GetCurrentProcess(), &bIsWow64);
		if (bIsWow64 == TRUE)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
#endif	
}


void ProcessLister::initDeviceNameList()
{
	TCHAR shortName[3] = {0};
	TCHAR longName[MAX_PATH] = {0};
	HardDisk hardDisk;

	shortName[1] = L':';

	for ( WCHAR shortD = L'a'; shortD < L'z'; shortD++ )
	{
		shortName[0] = shortD;
		if (QueryDosDeviceW( shortName, longName, MAX_PATH ) > 0)
		{
			hardDisk.shortName[0] = towupper(shortD);
			hardDisk.shortName[1] = L':';
			hardDisk.shortName[2] = 0;

			hardDisk.longNameLength = wcslen(longName);

			wcscpy_s(hardDisk.longName, longName);
			deviceNameList.push_back(hardDisk);
		}
	}
}

//only needed in windows xp
DWORD ProcessLister::setDebugPrivileges()
{
	DWORD err = 0;
	HANDLE hToken = 0;
	TOKEN_PRIVILEGES Debug_Privileges = {0};

	if(!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Debug_Privileges.Privileges[0].Luid))
	{
		return GetLastError();
	}

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		err = GetLastError();  
		if(hToken) CloseHandle(hToken);
		return err;
	}

	Debug_Privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	Debug_Privileges.PrivilegeCount = 1;

	AdjustTokenPrivileges(hToken, false, &Debug_Privileges, 0, NULL, NULL);

	CloseHandle(hToken);
	return GetLastError();
}


/************************************************************************/
/* Check if a process is 32 or 64bit                                    */
/************************************************************************/
ProcessType ProcessLister::checkIsProcess64(DWORD dwPID)
{
	HANDLE hProcess;
	BOOL bIsWow64 = FALSE;

	if (dwPID == 0)
	{
		//unknown
		return PROCESS_UNKNOWN;
	}

	//hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, NULL, dwPID);

	hProcess = ProcessAccessHelp::NativeOpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, dwPID);

	if(!hProcess)
	{
		//missing rights
		return PROCESS_MISSING_RIGHTS;
	}

	if (!isWindows64())
	{
		//32bit win can only run 32bit process
		CloseHandle(hProcess);
		return PROCESS_32;
	}

	_IsWow64Process(hProcess, &bIsWow64);
	CloseHandle(hProcess);

	if (bIsWow64 == FALSE)
	{
		//process not running under wow
		return PROCESS_64;
	} 
	else
	{
		//process running under wow -> 32bit
		return PROCESS_32;
	}
}

bool ProcessLister::getAbsoluteFilePath(Process * process)
{
	WCHAR processPath[MAX_PATH];
	HANDLE hProcess;

	//hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, NULL, process->PID);
	hProcess = ProcessAccessHelp::NativeOpenProcess(PROCESS_QUERY_INFORMATION, process->PID);

	if(!hProcess)
	{
		//missing rights
		return false;
	}

	if (GetProcessImageFileName(hProcess, processPath, _countof(processPath)) > 0)
	{
		CloseHandle(hProcess);

		if (!resolveDeviceLongNameToShort(processPath, process->fullPath))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getAbsoluteFilePath :: resolveDeviceLongNameToShort failed with path %s", processPath);
#endif
		}
		return true;
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"getAbsoluteFilePath :: GetProcessImageFileName failed %u", GetLastError());
#endif
		CloseHandle(hProcess);
		return false;
	}

}

std::vector<Process>& ProcessLister::getProcessListSnapshot()
{
	HANDLE hProcessSnap;
	ProcessType processType;
	PROCESSENTRY32 pe32;
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32 = {0};
	Process process;


	if (!processList.empty())
	{
		//clear elements, but keep reversed memory
		processList.clear();
	}
	else
	{
		//first time, reserve memory
		processList.reserve(34);
	}

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return processList;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hProcessSnap, &pe32))
	{
		CloseHandle(hProcessSnap);
		return processList;
	}

	do
	{
		//filter process list
		if (pe32.th32ProcessID > 4)
		{

			processType = checkIsProcess64(pe32.th32ProcessID);

			if (processType != PROCESS_MISSING_RIGHTS)
			{
				

#ifdef _WIN64
				if (processType == PROCESS_64)
#else
				if (processType == PROCESS_32)
#endif
				{
					process.PID = pe32.th32ProcessID;
					

					hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process.PID);
					if(hModuleSnap != INVALID_HANDLE_VALUE)
					{
						me32.dwSize = sizeof(MODULEENTRY32);

						Module32First(hModuleSnap, &me32);
						process.imageBase = (DWORD_PTR)me32.hModule;
						process.imageSize = me32.modBaseSize;
						CloseHandle(hModuleSnap);
					}

					wcscpy_s(process.filename, pe32.szExeFile);

					getAbsoluteFilePath(&process);

					processList.push_back(process);
				}
			}
		}
	} while(Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);


	//reverse process list
	std::reverse(processList.begin(), processList.end());

	return processList;
}

void ProcessLister::getAllModuleInformation()
{
	/*for (std::size_t i = 0; i < processList.size(); i++)
	{
		getModuleInformationByProcess(&processList[i]);
	}*/
}

void ProcessLister::getModuleInformationByProcess(Process *process)
{
/*	MODULEENTRY32 me32 = {0};
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	char temp[111];


	if (process->PID == 0)
	{
		MessageBox(0, "PID == NULL","ProcessLister::getModuleInformationByProcess", MB_OK|MB_ICONWARNING);
		return;
	}

#ifdef _WIN64
	if (!process->is64BitProcess)
	{
		//MessageBox(hWndDlg, "I'm a x64 process and you're trying to access a 32-bit process!","displayModuleList", MB_OK);
		return;
	}
#else
	if (process->is64BitProcess)
	{
		//MessageBox(hWndDlg, "I'm a 32-bit process and you're trying to access a x64 process!","displayModuleList", MB_OK);
		return;
	}
#endif

	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, process->PID);

	if(hModuleSnap == INVALID_HANDLE_VALUE)
	{
		sprintf_s(temp, "GetLastError %d",GetLastError());
		MessageBox(0, temp,"ProcessLister::getModuleInformationByProcess", MB_OK|MB_ICONWARNING);
		return;
	}

	me32.dwSize = sizeof(MODULEENTRY32);

	if(!Module32First(hModuleSnap, &me32))
	{
		MessageBox(0, "Module32First error","ProcessLister::getModuleInformationByProcess", MB_OK|MB_ICONWARNING);
		CloseHandle(hModuleSnap);
		return;
	}

	do {

		ModuleInfo moduleInfo;

		if (!_strnicmp(me32.szExePath,"\\Systemroot",11))
		{
			char * path = (char *)malloc(MAX_PATH);
			sprintf_s(path"%s\\%s",getenv("SystemRoot"),(me32.szExePath + 12));
			strcpy_s(moduleInfo.fullPath,MAX_PATH, path);
			free(path);
		}
		else if(!_strnicmp(me32.szExePath,"\\??\\",4))
		{
			strcpy_s(moduleInfo.fullPath,MAX_PATH, (me32.szExePath + 4));
		}
		else
		{
			strcpy_s(moduleInfo.fullPath,MAX_PATH,me32.szExePath);
		}

		moduleInfo.hModule = (DWORD_PTR)me32.hModule;
		moduleInfo.modBaseSize = me32.modBaseSize;
		moduleInfo.modBaseAddr = (DWORD_PTR)me32.modBaseAddr;

		process->moduleList[moduleInfo.hModule] = moduleInfo;

	} while(Module32Next(hModuleSnap, &me32));

	CloseHandle(hModuleSnap);*/

}

bool ProcessLister::resolveDeviceLongNameToShort( WCHAR * sourcePath, WCHAR * targetPath )
{
	for (unsigned int i = 0; i < deviceNameList.size(); i++)
	{
		if (!_wcsnicmp(deviceNameList[i].longName, sourcePath, deviceNameList[i].longNameLength))
		{
			wcscpy_s(targetPath, MAX_PATH,deviceNameList[i].shortName);
			wcscat_s(targetPath, MAX_PATH, sourcePath + deviceNameList[i].longNameLength);
			return true;
		}
	}

	return false;
}
