#pragma once

#include <windows.h>

typedef struct _GUI_DLL_PARAMETER {
	DWORD dwProcessId;
	HINSTANCE mod;
} GUI_DLL_PARAMETER, *PGUI_DLL_PARAMETER;

int InitializeGui(HINSTANCE hInstance, LPARAM param);


//function to export in DLL

BOOL DumpProcessW(const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult);

BOOL WINAPI ScyllaDumpCurrentProcessW(const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult);
BOOL WINAPI ScyllaDumpCurrentProcessA(const char * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const char * fileResult);

BOOL WINAPI ScyllaDumpProcessW(DWORD_PTR pid, const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult);
BOOL WINAPI ScyllaDumpProcessA(DWORD_PTR pid, const char * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const char * fileResult);

BOOL WINAPI ScyllaRebuildFileW(const WCHAR * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum, BOOL createBackup);
BOOL WINAPI ScyllaRebuildFileA(const char * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum, BOOL createBackup);

WCHAR * WINAPI ScyllaVersionInformationW();
char * WINAPI ScyllaVersionInformationA();
DWORD WINAPI ScyllaVersionInformationDword();

INT WINAPI ScyllaStartGui(DWORD dwProcessId, HINSTANCE mod);