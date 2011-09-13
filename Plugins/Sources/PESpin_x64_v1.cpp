#include "ScyllaPlugin.h"

const char logFileName[] = "logfile_scylla_plugin.txt";
BOOL writeToLogFile(const char * text);


HANDLE hMapFile = 0;
LPVOID lpViewOfFile = 0;

BOOL getMappedView();
void cleanUp();

void resolveImports();
DWORD getApiHash(const char * apiName);
DWORD_PTR findPattern(DWORD_PTR startOffset, DWORD size, const BYTE * pattern, const char * mask);
DWORD_PTR getApiAddress(DWORD_PTR dllBaseArray, DWORD_PTR apiHash);
BOOL searchMagicConstant(DWORD_PTR startAddress);
DWORD_PTR findJumpDestination(DWORD_PTR startAddress);
DWORD_PTR findApiDefinition(DWORD_PTR startAddress);
DWORD_PTR findDllBaseArrayAddress(DWORD_PTR startAddress);

#define PLUGIN_NAME "PESpin x64 v1.x"

char textBuffer[200];
DWORD magicConstant = 0;
BOOL executeOnce = 0;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,LPVOID lpvReserved)
{
	switch(fdwReason) 
	{ 
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.

		DisableThreadLibraryCalls(hinstDLL);

		writeToLogFile("DLL attached - Injection successful\r\n");
		if (getMappedView()) //open file mapping
		{
			writeToLogFile("Open mapping successful\r\n");

			resolveImports(); //resolve imports
			writeToLogFile("resolveImports done\r\n");

			cleanUp(); //clean up handles
			writeToLogFile("All Plugin stuff done\r\n");
		}
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		writeToLogFile("DLL successfully detached\r\n");
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

BOOL getMappedView()
{
	hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, FILE_MAPPING_NAME); //open named file mapping object

	if (hMapFile == 0)
	{
		writeToLogFile("OpenFileMappingA failed\r\n");
		return FALSE;
	}

	lpViewOfFile = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS,	0, 0, 0); //map the view with full access

	if (lpViewOfFile == 0)
	{
		CloseHandle(hMapFile); //close mapping handle
		hMapFile = 0;
		writeToLogFile("MapViewOfFile failed\r\n");
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

void cleanUp()
{
	if (lpViewOfFile != 0)
	{
		UnmapViewOfFile(lpViewOfFile); //close map view
	}
	if (hMapFile != 0)
	{
		CloseHandle(hMapFile); //close mapping handle
	}
}


void resolveImports()
{
	DWORD_PTR invalidApiAddress = 0;
	DWORD_PTR apiHash = 0;
	DWORD_PTR destination = 0;
	DWORD_PTR dllBaseArray = 0;
	PSCYLLA_EXCHANGE scyllaExchange = 0;
	PUNRESOLVED_IMPORT unresolvedImport = 0;

	scyllaExchange = (PSCYLLA_EXCHANGE)lpViewOfFile;
	unresolvedImport = (PUNRESOLVED_IMPORT)((DWORD_PTR)lpViewOfFile + scyllaExchange->offsetUnresolvedImportsArray);

	scyllaExchange->status = SCYLLA_STATUS_SUCCESS;

	while (unresolvedImport->ImportTableAddressPointer != 0) //last element is a nulled struct
	{
		//get real WINAPI address

		//overwrite the existing wrong value with the good api address
		invalidApiAddress = unresolvedImport->InvalidApiAddress;


		//000000014000F676			50								push rax
		//000000014000F677			48B800657410C2584800			mov rax,004858C210746500
		//000000014000F681			E928030000			jmp 000000014000F9AE

		if (*((BYTE *)invalidApiAddress) == 0x50) //is it pespin, push rax
		{
			apiHash = findApiDefinition(invalidApiAddress); //read 00657410C2584800

			if (!apiHash)
			{
				writeToLogFile("API Definition not found\r\n");
				scyllaExchange->status = SCYLLA_STATUS_UNSUPPORTED_PROTECTION;
				break;
			}

			destination = findJumpDestination(invalidApiAddress); //jmp 000000014000F9AE

			if (!destination)
			{
				writeToLogFile("JMP Destination not found\r\n");
				scyllaExchange->status = SCYLLA_STATUS_UNSUPPORTED_PROTECTION;
				break;
			}

			if (!executeOnce)
			{
				if (!searchMagicConstant(destination))
				{
					writeToLogFile("Magic Constant not found\r\n");
					scyllaExchange->status = SCYLLA_STATUS_UNSUPPORTED_PROTECTION;
					break;
				}
			}

			dllBaseArray = findDllBaseArrayAddress(destination);

			if (!dllBaseArray)
			{
				writeToLogFile("DLL Base Array not found\r\n");
				scyllaExchange->status = SCYLLA_STATUS_UNSUPPORTED_PROTECTION;
				break;
			}

			unresolvedImport->InvalidApiAddress = getApiAddress(dllBaseArray, apiHash);

			if (!unresolvedImport->InvalidApiAddress)
			{
				writeToLogFile("API not found\r\n");
				scyllaExchange->status = SCYLLA_STATUS_IMPORT_RESOLVING_FAILED;
				break;
			}

		}
		else
		{
			writeToLogFile("Unsupported opcode found\r\n");
			scyllaExchange->status = SCYLLA_STATUS_UNSUPPORTED_PROTECTION;
			break;
		}

		unresolvedImport++; //next pointer to struct
	}
}


			/*
			000000014000F9E1 4C 8D 1D AE FF FF FF          lea     r11, off_14000F996 ;dll base
			000000014000F9E8 4C 8B D0                      mov     r10, rax
			000000014000F9EB 49 81 E2 FF 00 00 00          and     r10, 0FFh
			000000014000F9F2 49 C1 E2 03                   shl     r10, 3
			000000014000F9F6 4F 8B 14 1A                   mov     r10, [r10+r11]
			*/
DWORD_PTR findDllBaseArrayAddress(DWORD_PTR startAddress)
{
	__int32 relAddress = 0;

	startAddress = findPattern(startAddress, 300, (BYTE *)"\x4C\x8D\xFF\xFF\xFF\xFF\xFF\x4C\x8B\xD0\x49\x81\xE2\xFF\x00\x00","xx?????xxxxxxxxx");

	if (startAddress)
	{
		startAddress += 3;
		relAddress = *((__int32 *)startAddress);

		wsprintfA(textBuffer, "- Found DLL Base array ->  %016I64X\r\n", (relAddress + startAddress + sizeof(DWORD)));
		writeToLogFile(textBuffer);

		return (relAddress + startAddress + sizeof(DWORD));
	}
	else
	{
		writeToLogFile("findDllBaseArrayAddress failed\r\n");
		return 0;
	}
}

DWORD_PTR findApiDefinition(DWORD_PTR startAddress)
{
	startAddress += 3;  //increase 3 bytes

	wsprintfA(textBuffer, "- Found API Definition ->  %016I64X\r\n", *((DWORD_PTR *)startAddress));
	writeToLogFile(textBuffer);

	return *((DWORD_PTR *)startAddress); //read 00657410C2584800
}

DWORD_PTR findJumpDestination(DWORD_PTR startAddress)
{
	__int32 destination = 0;

	startAddress += sizeof(DWORD_PTR) + 4;  //increase 8 + 4 bytes

	destination = *((__int32 *)startAddress); //get jmp relative 4 bytes

	wsprintfA(textBuffer, "- Found JMP Destination ->  %016I64X\r\n", destination + startAddress + sizeof(DWORD));
	writeToLogFile(textBuffer);

	return destination + startAddress + sizeof(DWORD); //increase 4 + 1 bytes
}

BOOL searchMagicConstant(DWORD_PTR startAddress)
{

	executeOnce = 1;
	/*

	000000014000FBF9 32 D0                         xor     dl, al
	000000014000FBFB B0 08                         mov     al, 8
	000000014000FBFD
	000000014000FBFD                               loc_14000FBFD:
	000000014000FBFD D1 EA                         shr     edx, 1
	000000014000FBFF 73 06                         jnb     short loc_14000FC07
	000000014000FC01 81 F2 1F AF 81 73             xor     edx, 7381AF1F       ; constant
	000000014000FC07
	000000014000FC07                               loc_14000FC07:
	000000014000FC07 FE C8                         dec     al
	000000014000FC09 75 F2                         jnz     short loc_14000FBFD
	000000014000FC0B EB E3                         jmp     short loc_14000FBF0
	000000014000FC0D
	000000014000FC0D                               loc_14000FC0D: 
	000000014000FC0D 48 92                         xchg    rax, rdx

	*/

	DWORD_PTR address = findPattern(startAddress, 1000, (BYTE *)"\x32\xD0\xB0\x08\xD1\xEA\x73\x06\x81\xF2\xFF\xFF\xFF\xFF\xFE\xC8\x75","xxxxxxxxxx????xxx");
	if (address)
	{
		address += 10;
		magicConstant = *((DWORD *)address);

		wsprintfA(textBuffer, "- Found magic constant ->  %08X\r\n", magicConstant);
		writeToLogFile(textBuffer);

		return 1;
	}
	else
	{
		magicConstant = 0;
		writeToLogFile("Magic Constant not found\r\n");
		return 0;
	}
}

/*
 * Resolve PESpin Api String to VA
 */
DWORD_PTR getApiAddress(DWORD_PTR dllBaseArray, DWORD_PTR apiHash)
{
	PIMAGE_DOS_HEADER pDosHeader = 0;
	PIMAGE_NT_HEADERS pNtHeader = 0;
	PIMAGE_EXPORT_DIRECTORY pExportHeader = 0;
	DWORD_PTR dllBase = 0;
	int dllBaseIndex = 0;
	DWORD *addressOfFunctionsArray = 0,*addressOfNamesArray = 0;
	WORD *addressOfNameOrdinalsArray = 0;
	char *functionName = 0;
	DWORD_PTR RVA = 0, VA = 0, deltaAddress = 0;
	DWORD apiNameHash = 0;
	char apiNameCmp[3] = {0};

	dllBaseIndex = apiHash & 0xFF;
	dllBase = *((DWORD_PTR *)dllBaseArray + dllBaseIndex);

	//e.g. 0x0021AF0957746500
	apiNameHash = (DWORD)(apiHash >> 24); //21AF0957
	apiNameCmp[0] = (char)(apiHash >> 8); //65
	apiNameCmp[1] = (char)(apiHash >> 16); //74

	pDosHeader = (PIMAGE_DOS_HEADER)dllBase;

	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		writeToLogFile("Wrong dll base, IMAGE_DOS_SIGNATURE doesn't match\r\n");
		return 0;
	}

	pNtHeader = (PIMAGE_NT_HEADERS)(dllBase + pDosHeader->e_lfanew);

	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		writeToLogFile("Wrong dll base, IMAGE_NT_SIGNATURE doesn't match\r\n");
		return 0;
	}

	if (pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0)
	{
		writeToLogFile("No export table found\r\n");
		return 0;
	}

	pExportHeader = (PIMAGE_EXPORT_DIRECTORY)(dllBase + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	deltaAddress = (DWORD_PTR)pExportHeader - pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	addressOfFunctionsArray = (DWORD *)((DWORD_PTR)pExportHeader->AddressOfFunctions + deltaAddress);
	addressOfNamesArray = (DWORD *)((DWORD_PTR)pExportHeader->AddressOfNames + deltaAddress);
	addressOfNameOrdinalsArray = (WORD *)((DWORD_PTR)pExportHeader->AddressOfNameOrdinals + deltaAddress);

	for (DWORD i = 0; i < pExportHeader->NumberOfNames; i++)
	{
		functionName = (char*)(addressOfNamesArray[i] + deltaAddress);

		if (functionName[1] == apiNameCmp[0] && functionName[2] == apiNameCmp[1])
		{

			if (getApiHash(functionName) == apiNameHash)
			{
				//VA = addressOfFunctionsArray[addressOfNameOrdinalsArray[i]] + dllBase;

				//avoid forward api handling:
				VA = (DWORD_PTR)GetProcAddress((HMODULE)dllBase, functionName);

				wsprintfA(textBuffer, "- Found %s %016I64X\r\n",functionName, VA);
				writeToLogFile(textBuffer);
				
				return VA;
			}
		}
	}

	wsprintfA(textBuffer, "- Cannot find apiHash %016I64X dllBaseArray address %016I64X\r\n",apiHash, dllBaseArray);
	writeToLogFile(textBuffer);

	return 0;
}


/*
 * Api name hash function at VA 14000FBEC
 *
 * Start: 000000014000FBEC 52                            push    rdx
 * End:   000000014000FC10 C3                            retn
 */
DWORD getApiHash(const char * apiName)
{
	DWORD dwCheck = 0xFFFFFFFF;
	char key = 0;

	for (int i = 0; i < lstrlenA(apiName); i++)
	{
		key = apiName[i];

		dwCheck = (dwCheck & 0xFFFFFF00) + ((dwCheck & 0xFF) ^ key);
		key = 0x8;

		do 
		{
			if (dwCheck % 2)
			{
				dwCheck >>= 1;
				dwCheck ^= magicConstant;
			}
			else
			{
				dwCheck >>= 1;
			}

			key--;
		} while (key);
	}

	return dwCheck;
}

/*
 * Search a memory region for a byte pattern and return the address
 */
DWORD_PTR findPattern(DWORD_PTR startOffset, DWORD size, const BYTE * pattern, const char * mask)
{
	DWORD pos = 0;
	int searchLen = lstrlenA(mask) - 1;

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

/*
 * Write text to a logfile, the logfile is in the same folder as the target exe
 */
BOOL writeToLogFile(const char * text)
{
	DWORD lpNumberOfBytesWritten = 0;
	size_t i = 0;
	BOOL wfRet = 0;
	HANDLE hFile = 0;
	char buffer[260];

	if (!GetModuleFileNameA(0, buffer, sizeof(buffer))) //get full path of exe
	{
		return FALSE;
	}

	for (i = (lstrlenA(buffer) - 1); i >= 0; i--) //remove the exe file name from full path
	{
		if (buffer[i] == '\\')
		{
			buffer[i+1] = 0x00;
			break;
		}
	}

	if (lstrcatA(buffer,logFileName) == 0) //append log file name to path
	{
		return FALSE;
	}

	hFile = CreateFileA(buffer, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); //open log file for writing

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}

	SetFilePointer(hFile, 0, 0, FILE_END); //set file pointer to the end of file

	if (WriteFile(hFile, text, (DWORD)lstrlenA(text), &lpNumberOfBytesWritten, 0)) //write message to logfile
	{
		wfRet = TRUE;
	}
	else
	{
		wfRet = FALSE;
	}

	CloseHandle(hFile);
	return wfRet;
}


#ifdef UNICODE
DllExport wchar_t * __cdecl ScyllaPluginNameW()
{
	return TEXT(PLUGIN_NAME);
}
#else
DllExport char * __cdecl ScyllaPluginNameA()
{
	return PLUGIN_NAME;
}
#endif