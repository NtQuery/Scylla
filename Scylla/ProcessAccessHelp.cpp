
#include "ProcessAccessHelp.h"

#include "Scylla.h"
#include "NativeWinApi.h"
#include "PeParser.h"

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
			Scylla::debugLog.log(L"openProcessHandle :: There is already a process handle, HANDLE %X", hProcess);
#endif
			return false;
		}
		else
		{
			//hProcess = OpenProcess(PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE, 0, dwPID);
			//if (!NT_SUCCESS(NativeWinApi::NtOpenProcess(&hProcess,PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE,&ObjectAttributes, &cid)))

			hProcess = NativeOpenProcess(PROCESS_CREATE_THREAD|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_SUSPEND_RESUME|PROCESS_TERMINATE, dwPID);

			if (hProcess)
			{
				return true;
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"openProcessHandle :: Failed to open handle, PID %X", dwPID);
#endif
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"openProcessHandle :: Wrong PID, PID %X", dwPID);
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
		Scylla::debugLog.log(L"NativeOpenProcess :: Failed to open handle, PID %X Error 0x%X", dwProcessId, NativeWinApi::RtlNtStatusToDosError(ntStatus));
#endif
		return 0;
	}
}

void ProcessAccessHelp::closeProcessHandle()
{
	if (hProcess)
	{
		CloseHandle(hProcess);
		hProcess = 0;
	}

	moduleList.clear();
	targetImageBase = 0;
	selectedModule = 0;
}

bool ProcessAccessHelp::readMemoryPartlyFromProcess(DWORD_PTR address, SIZE_T size, LPVOID dataBuffer)
{
	DWORD_PTR addressPart = 0;
	DWORD_PTR readBytes = 0;
	DWORD_PTR bytesToRead = 0;
	MEMORY_BASIC_INFORMATION memBasic = {0};
	bool returnValue = false;

	if (!hProcess)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"readMemoryPartlyFromProcess :: hProcess == NULL");
#endif
		return returnValue;
	}

	if (!readMemoryFromProcess(address, size, dataBuffer))
	{
		addressPart = address;

		do 
		{
			if (!VirtualQueryEx(ProcessAccessHelp::hProcess,(LPCVOID)addressPart,&memBasic,sizeof(memBasic)))
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"readMemoryPartlyFromProcess :: Error VirtualQueryEx %X %X err: %u", addressPart,size, GetLastError());
#endif
				break;
			}

			bytesToRead = memBasic.RegionSize;

			if ( (readBytes+bytesToRead) > size)
			{
				bytesToRead = size - readBytes;
			}

			if (memBasic.State == MEM_COMMIT)
			{
				if (!readMemoryFromProcess(addressPart, bytesToRead, (LPVOID)((DWORD_PTR)dataBuffer + readBytes)))
				{
					break;
				}
			}
			else
			{
				ZeroMemory((LPVOID)((DWORD_PTR)dataBuffer + readBytes),bytesToRead);
			}


			readBytes += bytesToRead;

			addressPart += memBasic.RegionSize;

		} while (readBytes < size);

		if (readBytes == size)
		{
			returnValue = true;
		}
		
	}
	else
	{
		returnValue = true;
	}

	return returnValue;
}

bool ProcessAccessHelp::writeMemoryToProcess(DWORD_PTR address, SIZE_T size, LPVOID dataBuffer)
{
	SIZE_T lpNumberOfBytesWritten = 0;
	if (!hProcess)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"readMemoryFromProcess :: hProcess == NULL");
#endif
		return false;
	}


	return (WriteProcessMemory(hProcess,(LPVOID)address, dataBuffer, size,&lpNumberOfBytesWritten) != FALSE);
}

bool ProcessAccessHelp::readMemoryFromProcess(DWORD_PTR address, SIZE_T size, LPVOID dataBuffer)
{
	SIZE_T lpNumberOfBytesRead = 0;
	DWORD dwProtect = 0;
	bool returnValue = false;

	if (!hProcess)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"readMemoryFromProcess :: hProcess == NULL");
#endif
		return returnValue;
	}

	if (!ReadProcessMemory(hProcess, (LPVOID)address, dataBuffer, size, &lpNumberOfBytesRead))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"readMemoryFromProcess :: Error ReadProcessMemory %X %X err: %u", address, size, GetLastError());
#endif
		if (!VirtualProtectEx(hProcess, (LPVOID)address, size, PAGE_READONLY, &dwProtect)) // Just try to get Read access, write combination cannot be obtained under certain circumstances and this is abused by some packers
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"readMemoryFromProcess :: Error VirtualProtectEx %X %X err: %u", address,size, GetLastError());
#endif
			returnValue = false;
		}
		else
		{
			if (!ReadProcessMemory(hProcess, (LPVOID)address, dataBuffer, size, &lpNumberOfBytesRead))
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"readMemoryFromProcess :: Error ReadProcessMemory %X %X err: %u", address, size, GetLastError());
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
			Scylla::debugLog.log(L"readMemoryFromProcess :: Error ReadProcessMemory read %d bytes requested %d bytes", lpNumberOfBytesRead, size);
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
		Scylla::debugLog.log(L"decomposeMemory :: distorm_decompose == DECRES_INPUTERR");
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
		Scylla::debugLog.log(L"disassembleMemory :: res == DECRES_INPUTERR");
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
		Scylla::debugLog.log(L"disassembleMemory :: res == %d", res);
#endif
		return true; //not all instructions fit in buffer
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
			Scylla::debugLog.log(L"ProcessAccessHelp::getFileSize :: GetFileSizeEx failed %u", GetLastError());
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
		Scylla::debugLog.log(L"ProcessAccessHelp::getFileSize hFile invalid");
#endif
		return 0;
	}
}


bool ProcessAccessHelp::readMemoryFromFile(HANDLE hFile, LONG offset, DWORD size, LPVOID dataBuffer)
{
	DWORD lpNumberOfBytesRead = 0;
	DWORD retValue = 0;
	DWORD dwError = 0;

	if (hFile != INVALID_HANDLE_VALUE)
	{
		retValue = SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
		dwError = GetLastError();

		if ((retValue == INVALID_SET_FILE_POINTER) && (dwError != NO_ERROR))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"readMemoryFromFile :: SetFilePointer failed error %u", dwError);
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
				Scylla::debugLog.log(L"readMemoryFromFile :: ReadFile failed - size %d - error %u", size, GetLastError());
#endif
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"readMemoryFromFile :: hFile invalid");
#endif
		return false;
	}
}

bool ProcessAccessHelp::writeMemoryToNewFile(const WCHAR * file,DWORD size, LPCVOID dataBuffer)
{
	HANDLE hFile = CreateFile(file, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		bool resultValue = writeMemoryToFile(hFile,0,size,dataBuffer);
		CloseHandle(hFile);
		return resultValue;
	}
	else
	{
		return false;
	}
}

bool ProcessAccessHelp::writeMemoryToFile(HANDLE hFile, LONG offset, DWORD size, LPCVOID dataBuffer)
{
	DWORD lpNumberOfBytesWritten = 0;
	DWORD retValue = 0;
	DWORD dwError = 0;

	if ((hFile != INVALID_HANDLE_VALUE) && dataBuffer)
	{
		retValue = SetFilePointer(hFile, offset, NULL, FILE_BEGIN);
		dwError = GetLastError();

		if ((retValue == INVALID_SET_FILE_POINTER) && (dwError != NO_ERROR))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"writeMemoryToFile :: SetFilePointer failed error %u", dwError);
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
				Scylla::debugLog.log(L"writeMemoryToFile :: WriteFile failed - size %d - error %u", size, GetLastError());
#endif
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"writeMemoryToFile :: hFile invalid");
#endif
		return false;
	}
}

bool ProcessAccessHelp::writeMemoryToFileEnd(HANDLE hFile, DWORD size, LPCVOID dataBuffer)
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
			Scylla::debugLog.log(L"writeMemoryToFileEnd :: WriteFile failed - size %d - error %u", size, GetLastError());
#endif
			return false;
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"writeMemoryToFileEnd :: hFile invalid");
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
		Scylla::debugLog.log(L"readHeaderFromFile :: INVALID_HANDLE_VALUE %u", GetLastError());
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
		Scylla::debugLog.log(L"createFileMappingView :: INVALID_HANDLE_VALUE %u", GetLastError());
#endif
		return NULL;
	}

	HANDLE hMappedFile = CreateFileMapping(hFile, NULL, flProtect, 0, 0, NULL);
	CloseHandle(hFile);

	if( hMappedFile == NULL )
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"createFileMappingView :: hMappedFile == NULL");
#endif
		return NULL;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"createFileMappingView :: GetLastError() == ERROR_ALREADY_EXISTS");
#endif
		CloseHandle(hMappedFile);
		return NULL;
	}

	LPVOID addrMappedDll = MapViewOfFile(hMappedFile, accessMap, 0, 0, 0);

	if( addrMappedDll == NULL )
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"createFileMappingView :: addrMappedDll == NULL");
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
		Scylla::debugLog.log(L"getProcessByName :: Error getting first Process");
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

bool ProcessAccessHelp::getProcessModules(HANDLE hProcess, std::vector<ModuleInfo> &moduleList)
{
    ModuleInfo module;
    WCHAR filename[MAX_PATH*2] = {0};
    DWORD cbNeeded = 0;
    bool retVal = false;
    DeviceNameResolver deviceNameResolver;

    moduleList.reserve(20);

    EnumProcessModules(hProcess, 0, 0, &cbNeeded);

    HMODULE* hMods=(HMODULE*)malloc(cbNeeded*sizeof(HMODULE));

    if (hMods)
    {
        if(EnumProcessModules(hProcess, hMods, cbNeeded, &cbNeeded))
        {
            for(unsigned int i = 1; i < (cbNeeded/sizeof(HMODULE)); i++) //skip first module!
            {
                module.modBaseAddr = (DWORD_PTR)hMods[i];
                module.modBaseSize = (DWORD)getSizeOfImageProcess(hProcess, module.modBaseAddr);
                module.isAlreadyParsed = false;
                module.parsing = false;

                filename[0] = 0;
                module.fullPath[0] = 0;

                if (GetMappedFileNameW(hProcess, (LPVOID)module.modBaseAddr, filename, _countof(filename)) > 0)
                {
                    if (!deviceNameResolver.resolveDeviceLongNameToShort(filename, module.fullPath))
                    {
                        if (!GetModuleFileNameExW(hProcess, (HMODULE)module.modBaseAddr, module.fullPath, _countof(module.fullPath)))
                        {
                            wcscpy_s(module.fullPath, filename);
                        }
                    }
                }
                else
                {
                    GetModuleFileNameExW(hProcess, (HMODULE)module.modBaseAddr, module.fullPath, _countof(module.fullPath));
                }

                moduleList.push_back(module);
            }

            retVal = true;
        }

        free(hMods);
    }

	return retVal;
}

bool ProcessAccessHelp::getMemoryRegionFromAddress(DWORD_PTR address, DWORD_PTR * memoryRegionBase, SIZE_T * memoryRegionSize)
{
	MEMORY_BASIC_INFORMATION memBasic;

	if (VirtualQueryEx(hProcess,(LPCVOID)address,&memBasic,sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"getMemoryRegionFromAddress :: VirtualQueryEx error %u", GetLastError());
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
	SIZE_T sizeOfImage = 0, sizeOfImageNative = 0;
	MEMORY_BASIC_INFORMATION lpBuffer = {0};

    sizeOfImageNative = getSizeOfImageProcessNative(processHandle, moduleBase);

    if (sizeOfImageNative)
    {
        return sizeOfImageNative;
    }

    WCHAR filenameOriginal[MAX_PATH*2] = {0};
    WCHAR filenameTest[MAX_PATH*2] = {0};

    GetMappedFileNameW(processHandle, (LPVOID)moduleBase, filenameOriginal, _countof(filenameOriginal));

	do
	{
		moduleBase = (DWORD_PTR)((SIZE_T)moduleBase + lpBuffer.RegionSize);
		sizeOfImage += lpBuffer.RegionSize;


		if (!VirtualQueryEx(processHandle, (LPCVOID)moduleBase, &lpBuffer, sizeof(MEMORY_BASIC_INFORMATION)))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getSizeOfImageProcess :: VirtualQuery failed %X", GetLastError());
#endif
			lpBuffer.Type = 0;
			sizeOfImage = 0;
		}

        GetMappedFileNameW(processHandle, (LPVOID)moduleBase, filenameTest, _countof(filenameTest));

        if (_wcsicmp(filenameOriginal,filenameTest) != 0)//problem: 2 modules without free space
        {
            break; 
        }

	} while (lpBuffer.Type == MEM_IMAGE);


    //if (sizeOfImage != sizeOfImageNative)
    //{
    //    WCHAR temp[1000] = {0};
    //    wsprintfW(temp, L"0x%X sizeofimage\n0x%X sizeOfImageNative", sizeOfImage, sizeOfImageNative);
    //    MessageBoxW(0, temp, L"Test", 0);
    //}

	return sizeOfImage;
}

DWORD ProcessAccessHelp::getEntryPointFromFile(const WCHAR * filePath)
{
	PeParser peFile(filePath, false);

	return peFile.getEntryPoint();
}

bool ProcessAccessHelp::createBackupFile(const WCHAR * filePath)
{
	size_t fileNameLength = wcslen(filePath) + 5; //.bak + null
	BOOL retValue = 0;

	WCHAR * backupFile = new WCHAR[fileNameLength];

	wcscpy_s(backupFile, fileNameLength, filePath);
	wcscat_s(backupFile, fileNameLength, L".bak");
	retValue = CopyFile(filePath, backupFile, FALSE);

	if (!retValue)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"createBackupFile :: CopyFile failed with error 0x%X", GetLastError());
#endif
	}

	delete [] backupFile;

	return retValue != 0;
}

DWORD ProcessAccessHelp::getModuleHandlesFromProcess(const HANDLE hProcess, HMODULE ** hMods)
{
	DWORD count = 30;
	DWORD cbNeeded = 0;
	bool notEnough = true;

	*hMods = new HMODULE[count];

	do 
	{
		if (!EnumProcessModules(hProcess, *hMods, count * sizeof(HMODULE), &cbNeeded))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getModuleHandlesFromProcess :: EnumProcessModules failed count %d", count);
#endif
			delete [] *hMods;
			return 0;
		}

		if ((count * sizeof(HMODULE)) < cbNeeded)
		{
			delete [] *hMods;
			count = cbNeeded / sizeof(HMODULE);
			*hMods = new HMODULE[count];
		}
		else
		{
			notEnough = false;
		}
	} while (notEnough);

	return cbNeeded / sizeof(HMODULE);
}

void ProcessAccessHelp::setCurrentProcessAsTarget()
{
	ProcessAccessHelp::hProcess = GetCurrentProcess();
}

bool ProcessAccessHelp::suspendProcess()
{
	if (NativeWinApi::NtSuspendProcess)
	{
		if (NT_SUCCESS( NativeWinApi::NtSuspendProcess(ProcessAccessHelp::hProcess) ))
		{
			return true;
		}
	}

	return false;
}

bool ProcessAccessHelp::resumeProcess()
{
	if (NativeWinApi::NtResumeProcess)
	{
		if (NT_SUCCESS( NativeWinApi::NtResumeProcess(ProcessAccessHelp::hProcess) ))
		{
			return true;
		}
	}

	return false;
}

bool ProcessAccessHelp::terminateProcess()
{
	if (NativeWinApi::NtTerminateProcess)
	{
		if (NT_SUCCESS( NativeWinApi::NtTerminateProcess(ProcessAccessHelp::hProcess, 0) ))
		{
			return true;
		}
	}

	return false;
}

bool ProcessAccessHelp::isPageAccessable( DWORD Protect )
{
	if (Protect & PAGE_NOCACHE) Protect ^= PAGE_NOCACHE;
	if (Protect & PAGE_GUARD) Protect ^= PAGE_GUARD;
	if (Protect & PAGE_WRITECOMBINE) Protect ^= PAGE_WRITECOMBINE;

	if (Protect != PAGE_NOACCESS)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ProcessAccessHelp::isPageExecutable( DWORD Protect )
{
    if (Protect & PAGE_NOCACHE) Protect ^= PAGE_NOCACHE;
    if (Protect & PAGE_GUARD) Protect ^= PAGE_GUARD;
    if (Protect & PAGE_WRITECOMBINE) Protect ^= PAGE_WRITECOMBINE;

    switch(Protect)
    {
    case PAGE_EXECUTE:
        {
            return true;
        }
    case PAGE_EXECUTE_READ:
        {
            return true;
        }
    case PAGE_EXECUTE_READWRITE:
        {
            return true;
        }
    case PAGE_EXECUTE_WRITECOPY:
        {
            return true;
        }
    default:
        return false;
    }

}

SIZE_T ProcessAccessHelp::getSizeOfImageProcessNative( HANDLE processHandle, DWORD_PTR moduleBase )
{
    MEMORY_REGION_INFORMATION memRegion = {0};
    SIZE_T retLen = 0;
    if (NativeWinApi::NtQueryVirtualMemory(processHandle, (PVOID)moduleBase, MemoryRegionInformation, &memRegion, sizeof(MEMORY_REGION_INFORMATION), &retLen) == STATUS_SUCCESS)
    {
        return memRegion.RegionSize;
    }

    return 0;
}
