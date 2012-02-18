#pragma once

#include <windows.h>

class DllInjection
{
public:

	HMODULE dllInjection(HANDLE hProcess, const WCHAR * filename);
	bool unloadDllInProcess(HANDLE hProcess, HMODULE hModule);
	HANDLE startRemoteThread(HANDLE hProcess, LPVOID lpStartAddress, LPVOID lpParameter);

private:

	HANDLE customCreateRemoteThread(HANDLE hProcess, LPVOID lpStartAddress, LPVOID lpParameter);
	void specialThreadSettings(HANDLE hThread);
	HMODULE getModuleHandleByFilename(HANDLE hProcess, const WCHAR * filename);
};
