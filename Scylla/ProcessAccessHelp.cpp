
#include "ProcessAccessHelp.h"

#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atldlgs.h>       // WTL common dialogs

#include "Logger.h"
#include "NativeWinApi.h"

HANDLE ProcessAccessHelp::hProcess = 0;

ModuleInfo * ProcessAccessHelp::selectedModule;
DWORD_PTR ProcessAccessHelp::targetImageBase = 0;
DWORD_PTR ProcessAccessHelp::targetSizeOfImage = 0;
DWORD_PTR ProcessAccessHelp::maxValidAddress = 0;

std::vector<ModuleInfo> ProcessAccessHelp::moduleList; //target process module list
std::vector<ModuleInfo> ProcessAccessHelp::ownModuleList; //own module list


_DInst ProcessAccessHelp::decomposerResult[MAX_INSTRUCTIONS];
unsigned int ProcessAccessHelp::decomposerInstructionsCount = 0;
_CodeInfo ProcessAccessHelp::decomposerCi = {0};

_DecodedInst  ProcessAccessHelp::decodedInstructions[MAX_INSTRUCTIONS];
unsigned int  ProcessAccessHelp::decodedInstructionsCount = 0;

BYTE ProcessAccessHelp::fileHeaderFromDisk[PE_HEADER_BYTES_COUNT];

//#define DEBUG_COMMENTS

bool ProcessAccessHelp::openProcessHandle(DWORD dwPID)
{
	if (dwPID > 0)
	{
		if (hProcess)
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("openProcessHandle :: There is already a process handle, HANDLE %X\r\n"),hProcess);
#endif
			return false;
		}
		else
		{
			//hProcess = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE, 0, dwPID);
			//if (!NT_SUCCESS(NativeWinApi::NtOpenProcess(&hProcess,PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE,&ObjectAttributes, &cid)))

			hProcess = NativeOpenProcess(PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE, dwPID);

			if (hProcess)
			{
				return true;
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog(TEXT("openProcessHandle :: Failed to open handle, PID %X\r\n"),dwPID);
#endif
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("openProcessHandle :: Wrong PID, PID %X \r\n"),dwPID);
#endif
		return false;
	}
	
}

HANDLE ProcessAccessHelp::NativeOpenProcess(DWORD dwDesiredAccess, DWORD dwProcessId)
{
	HANDLE hProcess = 0;
	CLIENT_ID cid = {0};
	OBJECT_ATTRIBUTES ObjectAttributes;
	NTSTATUS ntStatus = 0;

	InitializeObjectAttributes(&ObjectAttributes, 0, 0, 0, 0);
	cid.UniqueProcess = (HANDLE)dwProcessId;

	ntStatus = NativeWinApi::NtOpenProcess(&hProcess,dwDesiredAccess,&ObjectAttributes, &cid);

	if (NT_SUCCESS(ntStatus))
	{
		return hProcess;
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("NativeOpenProcess :: Failed to open handle, PID %X Error 0x%X\r\n"),dwProcessId, NativeWinApi::RtlNtStatusToDosError(ntStatus));
#endif
		return 0;
	}
}

void ProcessAccessHelp::closeProcessHandle()
{
	CloseHandle(hProcess);
	hProcess = 0;
	moduleList.clear();
	targetImageBase = 0;
	selectedModule = 0;
}

bool ProcessAccessHelp::readMemoryFromProcess(DWORD_PTR address, SIZE_T size, LPVOID dataBuffer)
{
	SIZE_T lpNumberOfBytesRead = 0;
	DWORD dwProtect = 0;
	bool returnValue = false;

	if (!hProcess)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("readMemoryFromProcess :: hProcess == NULL\r\n"));
#endif
		return returnValue;
	}

	if (!ReadProcessMemory(hProcess, (LPVOID)address, dataBuffer, size, &lpNumberOfBytesRead))
	{
		if (!VirtualProtectEx(hProcess, (LPVOID)address, size, PAGE_READWRITE, &dwProtect))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("readMemoryFromProcess :: Error VirtualProtectEx %X %X err: %u\r\n"),address,size,GetLastError());
#endif
			returnValue = false;
		}
		else
		{
			if (!ReadProcessMemory(hProcess, (LPVOID)address, dataBuffer, size, &lpNumberOfBytesRead))
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog(TEXT("readMemoryFromProcess :: Error ReadProcessMemory %X %X err: %u\r\n"),address,size,GetLastError());
#endif
				returnValue = false;
			}
			else
			{
				returnValue = true;
			}
			VirtualProtectEx(hProcess, (LPVOID)address, size, dwProtect, &dwProtect);
		}
	}
	else
	{
		returnValue = true;
	}

	if (returnValue)
	{
		if (size != lpNumberOfBytesRead)
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("readMemoryFromProcess :: Error ReadProcessMemory read %d bytes requested %d bytes\r\n"), lpNumberOfBytesRead, size);
#endif
			returnValue = false;
		}
		else
		{
			returnValue = true;
		}
	}
	
	return returnValue;
}

bool ProcessAccessHelp::decomposeMemory(BYTE * dataBuffer, SIZE_T bufferSize, DWORD_PTR startAddress)
{

	ZeroMemory(&decomposerCi, sizeof(_CodeInfo));
	decomposerCi.code = dataBuffer;
	decomposerCi.codeLen = (int)bufferSize;
	decomposerCi.dt = dt;
	decomposerCi.codeOffset = startAddress;

	decomposerInstructionsCount = 0;

	if (distorm_decompose(&decomposerCi, decomposerResult, sizeof(decomposerResult)/sizeof(decomposerResult[0]), &decomposerInstructionsCount) == DECRES_INPUTERR)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("decomposeMemory :: distorm_decompose == DECRES_INPUTERR\r\n"));
#endif
		return false;
	}
	else
	{
		return true;
	}
}

bool ProcessAccessHelp::disassembleMemory(BYTE * dataBuffer, SIZE_T bufferSize, DWORD_PTR startOffset)
{
	// Holds the result of the decoding.
	_DecodeResult res;

	// next is used for instruction's offset synchronization.
	// decodedInstructionsCount holds the count of filled instructions' array by the decoder.

	decodedInstructionsCount = 0;

	_OffsetType offset = startOffset;

	res = distorm_decode(offset, dataBuffer, (int)bufferSize, dt, decodedInstructions, MAX_INSTRUCTIONS, &decodedInstructionsCount);

/*	for (unsigned int i = 0; i < decodedInstructionsCount; i++) {
#ifdef SUPPORT_64BIT_OFFSET
		printf("%0*I64x (%02d) %-24s %s%s%s\n", dt != Decode64Bits ? 8 : 16, decodedInstructions[i].offset, decodedInstructions[i].size, (char*)decodedInstructions[i].instructionHex.p, (char*)decodedInstructions[i].mnemonic.p, decodedInstructions[i].operands.length != 0 ? " " : "", (char*)decodedInstructions[i].operands.p);
#else
		printf("%08x (%02d) %-24s %s%s%s\n", decodedInstructions[i].offset, decodedInstructions[i].size, (char*)decodedInstructions[i].instructionHex.p, (char*)decodedInstructions[i].mnemonic.p, decodedInstructions[i].operands.length != 0 ? " " : "", (char*)decodedInstructions[i].operands.p);
#endif

	}*/

	if (res == DECRES_INPUTERR)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("disassembleMemory :: res == DECRES_INPUTERR\r\n"));
#endif
		return false;
	}
	else if (res == DECRES_SUCCESS)
	{
		//printf("disassembleMemory :: res == DECRES_SUCCESS\n");
		return true;
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("disassembleMemory :: res == %d\r\n"),res);
#endif
		return false;
	}
}

DWORD_PTR ProcessAccessHelp::findPattern(DWORD_PTR startOffset, DWORD size, BYTE * pattern, const char * mask)
{
	DWORD pos = 0;
	size_t searchLen = strlen(mask) - 1;

	for(DWORD_PTR retAddress = startOffset; retAddress < startOffset + size; retAddress++)
	{
		if( *(BYTE*)retAddress == pattern[pos] || mask[pos] == '?' )
		{
			if(mask[pos+1] == 0x00)
			{
				return (retAddress - searchLen);
			}
			pos++;
		} else {
			pos = 0;
		}
	}
	return 0;
}

bool ProcessAccessHelp::readHeaderFromCurrentFile(const WCHAR * filePath)
{
	return readHeaderFromFile(fileHeaderFromDisk, sizeof(fileHeaderFromDisk), filePath);
}

LONGLONG ProcessAccessHelp::getFileSize(const WCHAR * filePath)
{
	LONGLONG fileSize = 0;

	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		fileSize = getFileSize(hFile);
		CloseHandle(hFile);
		hFile = 0;
	}
	
	return fileSize;
}

LONGLONG ProcessAccessHelp::getFileSize(HANDLE hFile)
{
	LARGE_INTEGER lpFileSize = {0};

	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != 0))
	{
		if (!GetFileSizeEx(hFile, &lpFileSize))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("ProcessAccessHelp::getFileSize :: GetFileSizeEx failed %u\r\n"),GetLastError());
#endif
			return 0;
		}
		else
		{
			return lpFileSize.QuadPart;
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("ProcessAccessHelp::getFileSize hFile invalid\r\n"));
#endif
		return 0;
	}
}


bool ProcessAccessHelp::readMemoryFromFile(HANDLE hFile, LONG offset, DWORD size, LPVOID dataBuffer)
{
	DWORD lpNumberOfBytesRead = 0;
	DWORD retValue = 0;
	DWORD dwError = 0;

	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != 0))
	{
		retValue = SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
		dwError = GetLastError();

		if ((retValue == INVALID_SET_FILE_POINTER) && (dwError != NO_ERROR))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("readMemoryFromFile :: SetFilePointer failed error %u\r\n"),dwError);
#endif
			return false;
		}
		else
		{
			if (ReadFile(hFile, dataBuffer, size, &lpNumberOfBytesRead, 0))
			{
				return true;
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog(TEXT("readMemoryFromFile :: ReadFile failed - size %d - error %u\r\n"),size,GetLastError());
#endif
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("readMemoryFromFile :: hFile invalid\r\n"));
#endif
		return false;
	}
}

bool ProcessAccessHelp::writeMemoryToFile(HANDLE hFile, LONG offset, DWORD size, LPVOID dataBuffer)
{
	DWORD lpNumberOfBytesWritten = 0;
	DWORD retValue = 0;
	DWORD dwError = 0;

	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != 0))
	{
		retValue = SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
		dwError = GetLastError();

		if ((retValue == INVALID_SET_FILE_POINTER) && (dwError != NO_ERROR))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("writeMemoryToFile :: SetFilePointer failed error %u\r\n"),dwError);
#endif
			return false;
		}
		else
		{
			if (WriteFile(hFile, dataBuffer, size, &lpNumberOfBytesWritten, 0))
			{
				return true;
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog(TEXT("writeMemoryToFile :: WriteFile failed - size %d - error %u\r\n"),size,GetLastError());
#endif
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("writeMemoryToFile :: hFile invalid\r\n"));
#endif
		return false;
	}
}

bool ProcessAccessHelp::writeMemoryToFileEnd(HANDLE hFile, DWORD size, LPVOID dataBuffer)
{
	DWORD lpNumberOfBytesWritten = 0;
	DWORD retValue = 0;

	if ((hFile != INVALID_HANDLE_VALUE) && (hFile != 0))
	{
		SetFilePointer(hFile, 0, 0, FILE_END);

		if (WriteFile(hFile, dataBuffer, size, &lpNumberOfBytesWritten, 0))
		{
			return true;
		}
		else
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("writeMemoryToFileEnd :: WriteFile failed - size %d - error %u\r\n"),size,GetLastError());
#endif
			return false;
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("writeMemoryToFileEnd :: hFile invalid\r\n"));
#endif
		return false;
	}
}

bool ProcessAccessHelp::readHeaderFromFile(BYTE * buffer, DWORD bufferSize, const WCHAR * filePath)
{
	DWORD lpNumberOfBytesRead = 0;
	LONGLONG fileSize = 0;
	DWORD dwSize = 0;
	bool returnValue = 0;

	HANDLE hFile = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if( hFile == INVALID_HANDLE_VALUE )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("readHeaderFromFile :: INVALID_HANDLE_VALUE %u\r\n"),GetLastError());
#endif
		returnValue = false;
	}
	else
	{
		fileSize = getFileSize(hFile);

		if (fileSize > 0)
		{
			if (fileSize > bufferSize)
			{
				dwSize = bufferSize;
			}
			else
			{
				dwSize = (DWORD)(fileSize - 1);
			}

			returnValue = readMemoryFromFile(hFile, 0, dwSize, buffer);
		}

		CloseHandle(hFile);
	}

	return returnValue;
}

LPVOID ProcessAccessHelp::createFileMappingViewRead(const WCHAR * filePath)
{
	return createFileMappingView(filePath, GENERIC_READ, PAGE_READONLY | SEC_IMAGE, FILE_MAP_READ);
}

LPVOID ProcessAccessHelp::createFileMappingViewFull(const WCHAR * filePath)
{
	return createFileMappingView(filePath, GENERIC_ALL, PAGE_EXECUTE_READWRITE, FILE_MAP_ALL_ACCESS);
}

LPVOID ProcessAccessHelp::createFileMappingView(const WCHAR * filePath, DWORD accessFile, DWORD flProtect, DWORD accessMap)
{
	HANDLE hFile = CreateFile(filePath, accessFile, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if( hFile == INVALID_HANDLE_VALUE )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("createFileMappingView :: INVALID_HANDLE_VALUE %u\r\n"),GetLastError());
#endif
		return NULL;
	}

	HANDLE hMappedFile = CreateFileMapping(hFile, NULL, flProtect, 0, 0, NULL);
	CloseHandle(hFile);

	if( hMappedFile == NULL )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("createFileMappingView :: hMappedFile == NULL\r\n"));
#endif
		return NULL;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("createFileMappingView :: GetLastError() == ERROR_ALREADY_EXISTS\r\n"));
#endif
		return NULL;
	}

	LPVOID addrMappedDll = MapViewOfFile(hMappedFile, accessMap, 0, 0, 0);

	if( addrMappedDll == NULL )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("createFileMappingView :: addrMappedDll == NULL\r\n"));
#endif
		CloseHandle(hMappedFile);
		return NULL;
	}

	CloseHandle(hMappedFile);

	return addrMappedDll;
}

DWORD ProcessAccessHelp::getProcessByName(const WCHAR * processName)
{
	DWORD dwPID = 0;
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32W pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if( !Process32FirstW( hProcessSnap, &pe32 ) )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("getProcessByName :: Error getting first Process\r\n"));
#endif
		CloseHandle( hProcessSnap );
		return 0;
	}

	do
	{
		if(!_wcsicmp(pe32.szExeFile, processName)) 
		{
			dwPID = pe32.th32ProcessID;
			break;
		}
	} while(Process32NextW(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);

	return dwPID;
}

bool ProcessAccessHelp::getProcessModules(DWORD dwPID, std::vector<ModuleInfo> &moduleList)
{
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	MODULEENTRY32 me32;
	ModuleInfo module;

	// Take a snapshot of all modules in the specified process.
	hModuleSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, dwPID );
	if( hModuleSnap == INVALID_HANDLE_VALUE )
	{
		return false;
	}

	// Set the size of the structure before using it.
	me32.dwSize = sizeof( MODULEENTRY32 );

	// Retrieve information about the first module,
	// and exit if unsuccessful
	if( !Module32First( hModuleSnap, &me32 ) )
	{
		CloseHandle( hModuleSnap );
		return false;
	}

	// Now walk the module list of the process,
	// and display information about each module

	//the first is always the .exe
	if (!Module32Next(hModuleSnap, &me32))
	{
		CloseHandle( hModuleSnap );
		return false;
	}

	moduleList.reserve(20);

	do
	{
		//printf(TEXT("\n     MODULE NAME:     %s"),   me32.szModule);
		module.modBaseAddr = (DWORD_PTR)me32.modBaseAddr;
		module.modBaseSize = me32.modBaseSize;
		module.isAlreadyParsed = false;
		module.parsing = false;
		wcscpy_s(module.fullPath, MAX_PATH, me32.szExePath);

		moduleList.push_back(module);

	} while(Module32Next(hModuleSnap, &me32));

	CloseHandle( hModuleSnap );
	return true;
}

bool ProcessAccessHelp::getMemoryRegionFromAddress(DWORD_PTR address, DWORD_PTR * memoryRegionBase, SIZE_T * memoryRegionSize)
{
	MEMORY_BASIC_INFORMATION memBasic;

	if (VirtualQueryEx(hProcess,(LPCVOID)address,&memBasic,sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("getMemoryRegionFromAddress :: VirtualQueryEx error %u\r\n"), GetLastError());
#endif
		return false;
	}
	else
	{
		*memoryRegionBase = (DWORD_PTR)memBasic.BaseAddress;
		*memoryRegionSize = memBasic.RegionSize;
		return true;
	}
}

bool ProcessAccessHelp::getSizeOfImageCurrentProcess()
{
	DWORD_PTR newSizeOfImage = getSizeOfImageProcess(ProcessAccessHelp::hProcess, ProcessAccessHelp::targetImageBase);

	if (newSizeOfImage != 0)
	{
		ProcessAccessHelp::targetSizeOfImage = newSizeOfImage;
		return true;
	}
	else
	{
		return false;
	}
}

SIZE_T ProcessAccessHelp::getSizeOfImageProcess(HANDLE processHandle, DWORD_PTR moduleBase)
{
	SIZE_T sizeOfImage = 0;
	MEMORY_BASIC_INFORMATION lpBuffer = {0};
	SIZE_T dwLength = sizeof(MEMORY_BASIC_INFORMATION);

	do
	{
		moduleBase = (DWORD_PTR)((SIZE_T)moduleBase + lpBuffer.RegionSize);
		sizeOfImage += lpBuffer.RegionSize;

		//printf("Query 0x"PRINTF_DWORD_PTR_FULL" size 0x%08X\n",moduleBase,sizeOfImage);

		if (!VirtualQueryEx(processHandle, (LPCVOID)moduleBase, &lpBuffer, dwLength))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("getSizeOfImageProcess :: VirtualQuery failed %X\r\n"),GetLastError());
#endif
			lpBuffer.Type = 0;
			sizeOfImage = 0;
		}
		/*else
		{
			printf("\nAllocationBase %X\n",lpBuffer.AllocationBase);
			printf("AllocationProtect %X\n",lpBuffer.AllocationProtect);
			printf("BaseAddress %X\n",lpBuffer.BaseAddress);
			printf("Protect %X\n",lpBuffer.Protect);
			printf("RegionSize %X\n",lpBuffer.RegionSize);
			printf("State %X\n",lpBuffer.State);
			printf("Type %X\n",lpBuffer.Type);
		}*/
	} while (lpBuffer.Type == MEM_IMAGE);

	//printf("Real sizeOfImage %X\n",sizeOfImage);

	return sizeOfImage;
}

//OFN_FILEMUSTEXIST
WCHAR * ProcessAccessHelp::selectFile(fileFilter type, BOOL save, DWORD flags, HWND parent)
{
	DWORD dwFlags = flags | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING;
	if(!save)
		dwFlags |= OFN_FILEMUSTEXIST;

	const WCHAR* filter;
	const WCHAR* defExt;

	switch (type)
	{
	case fileDll:
		filter = TEXT("Dynamic Link Library (*.dll)\0*.dll\00");
		defExt = TEXT(".dll");
		break;
	case fileExe:
		filter	= TEXT("Executable (*.exe)\0*.exe\00");
		defExt	= TEXT(".exe");
		break;
	case fileExeDll:
		filter = TEXT("Executable (*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\00");
		defExt = 0;
		break;
	default:
		filter = 0;
		defExt = 0;
		break;
	}

	CFileDialog dlgFile(!save, defExt, NULL, dwFlags, filter, parent);
	if(dlgFile.DoModal() == IDOK)
	{
		Logger::printfDialog(TEXT("Selected %s"), dlgFile.m_szFileName);
		WCHAR * targetFile = new WCHAR[MAX_PATH];
		wcscpy_s(targetFile, MAX_PATH, dlgFile.m_szFileName);
		return targetFile;
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("selectFileToSave :: CommDlgExtendedError 0x%X\r\n"), CommDlgExtendedError());
#endif
		return 0;
	}

	/*
	OPENFILENAME ofn = {0};
	WCHAR * targetFile = new WCHAR[MAX_PATH];
	targetFile[0] = 0;

	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= 0;

	ofn.lpstrCustomFilter	= 0;
	ofn.nFilterIndex		= 1;
	ofn.lpstrFile			= targetFile;
	ofn.nMaxFile			= MAX_PATH;
	ofn.lpstrFileTitle		= 0;
	ofn.nMaxFileTitle		= 0;
	ofn.lpstrInitialDir		= 0;
	ofn.lpstrTitle			= TEXT("Select a file");
	ofn.Flags				= OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLESIZING | flags;

	if(GetOpenFileName(&ofn)) {
		Logger::printfDialog(TEXT("Selected %s"),targetFile);
		return targetFile;
	} else {
		delete [] targetFile;

#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("selectFileToSave :: CommDlgExtendedError 0x%X\r\n"), CommDlgExtendedError());
#endif
		return 0;
	}
	*/
}

DWORD ProcessAccessHelp::getEntryPointFromFile(const WCHAR * filePath)
{
	PIMAGE_NT_HEADERS pNtHeader = 0;
	PIMAGE_DOS_HEADER pDosHeader = 0;

	readHeaderFromCurrentFile(filePath);

	pDosHeader = (PIMAGE_DOS_HEADER)fileHeaderFromDisk;

	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		return 0;
	}

	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)fileHeaderFromDisk + (DWORD_PTR)(pDosHeader->e_lfanew));

	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		return 0;
	}

	return pNtHeader->OptionalHeader.AddressOfEntryPoint;
}

bool ProcessAccessHelp::createBackupFile(const WCHAR * filePath)
{
	size_t fileNameLength = wcslen(filePath) + 5; //.bak + null
	BOOL retValue = 0;

	WCHAR * backupFile = new WCHAR[fileNameLength];

	wcscpy_s(backupFile, fileNameLength, filePath);
	wcscat_s(backupFile, fileNameLength, TEXT(".bak"));
	retValue = CopyFile(filePath, backupFile, FALSE);

	if (!retValue)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("createBackupFile :: CopyFile failed with error 0x%X\r\n"), GetLastError());
#endif
	}

	delete [] backupFile;

	return retValue != 0;
}
