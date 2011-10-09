#pragma once

#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <hash_map>
#include <map>

/************************************************************************/
/* distorm                                                              */
/************************************************************************/
#include "distorm.h"

// The number of the array of instructions the decoder function will use to return the disassembled instructions.
// Play with this value for performance...
#define MAX_INSTRUCTIONS (200)

/************************************************************************/

class ApiInfo;

class ModuleInfo {
public:
	WCHAR fullPath[MAX_PATH];
	DWORD_PTR modBaseAddr;
	DWORD modBaseSize;

	bool isAlreadyParsed;
	bool parsing;

	/*
	  for iat rebuilding with duplicate entries:

	  ntdll = low priority
	  kernelbase = low priority
	  SHLWAPI = low priority

	  kernel32 = high priority
	  
	  priority = 1 -> normal/high priority
	  priority = 0 -> low priority
	*/
	int priority;

	std::vector<ApiInfo *> apiList;

	ModuleInfo()
	{
		modBaseAddr = 0;
		modBaseSize = 0;
		priority = 1;
		isAlreadyParsed = false;
		parsing = false;
	}

	const WCHAR * getFilename() const
	{
		const WCHAR* slash = wcsrchr(fullPath, L'\\');
		if(slash)
		{
			return slash+1;
		}
		return fullPath;
	}
};

class ApiInfo {
	public:
		char name[MAX_PATH];
		WORD hint;
		DWORD_PTR va;
		DWORD_PTR rva;
		WORD ordinal;
		bool isForwarded;
		ModuleInfo * module;
};

class ProcessAccessHelp {
public:
	static HANDLE hProcess; //OpenProcess handle to target process

	static DWORD_PTR targetImageBase;
	static DWORD_PTR targetSizeOfImage;
	static DWORD_PTR maxValidAddress;

	static ModuleInfo * selectedModule;

	static std::vector<ModuleInfo> moduleList; //target process module list
	static std::vector<ModuleInfo> ownModuleList; //own module list

	static const int PE_HEADER_BYTES_COUNT = 2000;

	static BYTE fileHeaderFromDisk[PE_HEADER_BYTES_COUNT];


	//for decomposer
	static _DInst decomposerResult[MAX_INSTRUCTIONS];
	static unsigned int decomposerInstructionsCount;
	static _CodeInfo decomposerCi;

	//distorm :: Decoded instruction information.
	static _DecodedInst decodedInstructions[MAX_INSTRUCTIONS];
	static unsigned int decodedInstructionsCount;
#ifdef _WIN64
	static const _DecodeType dt = Decode64Bits;
#else
	static const _DecodeType dt = Decode32Bits;
#endif

	/*
	 * Open a new process handle
	 */
	static bool openProcessHandle(DWORD dwPID);

	static HANDLE NativeOpenProcess(DWORD dwDesiredAccess, DWORD dwProcessId);

	static void closeProcessHandle();

	/*
	 * Get all modules from a process
	 */
	static bool getProcessModules(DWORD dwPID, std::vector<ModuleInfo> &moduleList);


	/*
	 * file mapping view with different access level
	 */
	static LPVOID createFileMappingViewRead(const WCHAR * filePath);
	static LPVOID createFileMappingViewFull(const WCHAR * filePath);

	/*
	 * Create a file mapping view of a file 
	 */
	static LPVOID createFileMappingView(const WCHAR * filePath, DWORD accessFile, DWORD flProtect, DWORD accessMap);

	/*
	 * Read memory from target process
	 */
	static bool readMemoryFromProcess(DWORD_PTR address, SIZE_T size, LPVOID dataBuffer);

	/*
	 * Read memory from file
	 */
	static bool readMemoryFromFile(HANDLE hFile, LONG offset, DWORD size, LPVOID dataBuffer);

	/*
	 * Write memory to file
	 */
	static bool writeMemoryToFile(HANDLE hFile, LONG offset, DWORD size, LPCVOID dataBuffer);

	/*
	 * Write memory to file end
	 */
	static bool writeMemoryToFileEnd(HANDLE hFile, DWORD size, LPCVOID dataBuffer);

	/*
	 * Disassemble Memory
	 */
	static bool disassembleMemory(BYTE * dataBuffer, SIZE_T bufferSize, DWORD_PTR startOffset);

	static bool decomposeMemory(BYTE * dataBuffer, SIZE_T bufferSize, DWORD_PTR startAddress);

	/*
	 * Search for pattern
	 */
	static DWORD_PTR findPattern(DWORD_PTR startOffset, DWORD size, BYTE * pattern, const char * mask);

	/*
	 * Get process ID by process name
	 */
	static DWORD getProcessByName(const WCHAR * processName);

	/*
	 * Get memory region from address
	 */
	bool getMemoryRegionFromAddress(DWORD_PTR address, DWORD_PTR * memoryRegionBase, SIZE_T * memoryRegionSize);


	/*
	 * Read PE Header from file
	 */
	static bool readHeaderFromFile(BYTE * buffer, DWORD bufferSize, const WCHAR * filePath);

	static bool readHeaderFromCurrentFile(const WCHAR * filePath);

	/*
	 * Get real sizeOfImage value
	 */
	static SIZE_T getSizeOfImageProcess(HANDLE processHandle, DWORD_PTR moduleBase);

	/*
	 * Get real sizeOfImage value current process
	 */
	static bool getSizeOfImageCurrentProcess();

	static LONGLONG getFileSize(HANDLE hFile);
	static LONGLONG getFileSize(const WCHAR * filePath);

	static DWORD getEntryPointFromFile(const WCHAR * filePath);

	static bool createBackupFile(const WCHAR * filePath);
};
