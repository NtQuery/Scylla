#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <psapi.h>

#include "NativeWinApi.h"
#include "DeviceNameResolver.h"

typedef BOOL (WINAPI *def_IsWow64Process)(HANDLE hProcess,PBOOL Wow64Process);

class Process {
public:
	DWORD PID;
    DWORD sessionId;
	DWORD_PTR imageBase;
    DWORD_PTR pebAddress;
	DWORD entryPoint; //RVA without imagebase
	DWORD imageSize;
	WCHAR filename[MAX_PATH];
	WCHAR fullPath[MAX_PATH];

	Process()
	{
		PID = 0;
	}
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
		deviceNameResolver = new DeviceNameResolver();
		_IsWow64Process = (def_IsWow64Process)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "IsWow64Process");
	}
	~ProcessLister()
	{
		delete deviceNameResolver;
	}

	std::vector<Process>& getProcessList();
	static bool isWindows64();
	static DWORD setDebugPrivileges();
    std::vector<Process>& getProcessListSnapshotNative();
private:
	std::vector<Process> processList;

	DeviceNameResolver * deviceNameResolver;

	ProcessType checkIsProcess64(HANDLE hProcess);

	bool getAbsoluteFilePath(HANDLE hProcess, Process * process);


    void handleProcessInformationAndAddToList( PSYSTEM_PROCESS_INFORMATION pProcess );
    void getProcessImageInformation( HANDLE hProcess, Process* process );
    DWORD_PTR getPebAddressFromProcess( HANDLE hProcess );
};