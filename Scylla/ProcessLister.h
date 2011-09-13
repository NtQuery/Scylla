#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <Psapi.h>

#pragma comment(lib, "Psapi.lib")


typedef BOOL (WINAPI *def_IsWow64Process)(HANDLE hProcess,PBOOL Wow64Process);

class Process {
public:
	DWORD PID;
	DWORD_PTR imageBase;
	DWORD entryPoint; //without imagebase
	DWORD imageSize;
	WCHAR filename[MAX_PATH];
	WCHAR fullPath[MAX_PATH];

	Process()
	{
		PID = 0;
	}
};

class HardDisk {
public:
	WCHAR shortName[3];
	WCHAR longName[MAX_PATH];
	size_t longNameLength;
};

enum ProcessType {
	PROCESS_UNKNOWN,
	PROCESS_MISSING_RIGHTS,
	PROCESS_32,
	PROCESS_64
};

class ProcessLister {
public:

	static def_IsWow64Process _IsWow64Process;

	ProcessLister()
	{
		initDeviceNameList();
		_IsWow64Process = (def_IsWow64Process)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")),"IsWow64Process");
	}

	std::vector<Process>& getProcessList();
	static bool isWindows64();
	static DWORD setDebugPrivileges();
	std::vector<Process>& ProcessLister::getProcessListSnapshot();

private:
	std::vector<Process> processList;

	std::vector<HardDisk> deviceNameList;

	ProcessType checkIsProcess64(DWORD dwPID);

	void initDeviceNameList();


	bool getAbsoluteFilePath(Process * process);

	void getAllModuleInformation();
	void getModuleInformationByProcess(Process *process);
	bool resolveDeviceLongNameToShort( WCHAR * sourcePath, WCHAR * targetPath );
};