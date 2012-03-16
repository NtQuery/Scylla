#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <psapi.h>

#include "DeviceNameResolver.h"

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
	std::vector<Process>& ProcessLister::getProcessListSnapshot();

private:
	std::vector<Process> processList;

	DeviceNameResolver * deviceNameResolver;

	ProcessType checkIsProcess64(DWORD dwPID);

	bool getAbsoluteFilePath(Process * process);

	void getAllModuleInformation();
	void getModuleInformationByProcess(Process *process);
};