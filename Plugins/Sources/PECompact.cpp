
#include "ScyllaPlugin.h"

//remove c runtime library
//#pragma comment(linker, "/ENTRY:DllMain")


const char logFileName[] = "logfile_scylla_plugin.txt";
BOOL writeToLogFile(const char * text);


HANDLE hMapFile = 0;
LPVOID lpViewOfFile = 0;

BOOL getMappedView();
void cleanUp();

void resolveImports();


#define PLUGIN_NAME "PECompact v2.x"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,LPVOID lpvReserved)
{
	switch(fdwReason) 
	{ 
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.

		writeToLogFile("DLL attached - Injection successful\r\n");
		if (getMappedView()) //open file mapping
		{
			writeToLogFile("Open mapping successful\r\n");
			resolveImports(); //resolve imports
			cleanUp(); //clean up handles
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
	PSCYLLA_EXCHANGE scyllaExchange = 0;
	PUNRESOLVED_IMPORT unresolvedImport = 0;

	scyllaExchange = (PSCYLLA_EXCHANGE)lpViewOfFile;
	unresolvedImport = (PUNRESOLVED_IMPORT)((DWORD_PTR)scyllaExchange + scyllaExchange->offsetUnresolvedImportsArray);

	scyllaExchange->status = SCYLLA_STATUS_SUCCESS;

	while (unresolvedImport->ImportTableAddressPointer != 0) //last element is a nulled struct
	{
		//get real WINAPI address

		//overwrite the existing wrong value with the good api address
		invalidApiAddress = unresolvedImport->InvalidApiAddress;

		//PECompact example
		//01BF01FF     B8 45128275         MOV EAX,kernel32.GetModuleHandleA
		//01BF0204   - FFE0                JMP EAX

		if (*((BYTE *)invalidApiAddress) == 0xB8) //is it pe compact?
		{
			invalidApiAddress++;  //increase 1 opcode

			//write right value to struct
			unresolvedImport->InvalidApiAddress = *((DWORD_PTR *)invalidApiAddress);
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