#include "ScyllaPlugin.h"

//remove c runtime library
//#pragma comment(linker, "/ENTRY:DllMain")

//typedef DWORD (__stdcall * def_ImpREC_TraceSTD)(DWORD hFileMap, DWORD dwSizeMap, DWORD dwTimeOut, DWORD dwToTrace, DWORD dwExactCall);
//typedef DWORD (__cdecl * def_ImpREC_TraceCDE)(DWORD hFileMap, DWORD dwSizeMap, DWORD dwTimeOut, DWORD dwToTrace, DWORD dwExactCall);
typedef DWORD (* def_voidFunction)();

#define PLUGIN_IMPREC_EXCHANGE_DLL_PATH "ScyllaImprecPluginExchangePath"
#define PLUGIN_MAPPING_NAME "Imprec_plugin_exchanging"

const char logFileName[] = "logfile_scylla_plugin.txt";
BOOL writeToLogFile(const char * text);
BOOL getLogFilePath();

HMODULE hImprecPlugin = 0;
HANDLE hMapFile = 0;
LPVOID lpViewOfFile = 0;
WCHAR imprecPluginPath[260];
char textBuffer[200];
char logFilePath[260];

def_voidFunction voidFunction = 0;

BOOL stdcallPlugin = 0;

BOOL getMappedView();
void cleanUp();

void resolveImports();
BOOL getImprecPlugin();

DWORD callImprecTraceFunction(DWORD hFileMap, DWORD dwSizeMap, DWORD dwTimeOut, DWORD dwToTrace, DWORD dwExactCall);


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason,LPVOID lpvReserved)
{
	switch(fdwReason) 
	{ 
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.

		getLogFilePath();

		writeToLogFile("DLL attached - Injection successful\r\n");

		if (getImprecPlugin())
		{
			writeToLogFile("Loading ImpREC Plugin successful\r\n");

			if (getMappedView()) //open file mapping
			{
				writeToLogFile("Open mapping successful\r\n");
				resolveImports(); //resolve imports
				writeToLogFile("Resolving Imports successful\r\n");
				cleanUp(); //clean up handles
				writeToLogFile("Cleanup successful\r\n");
			}
		}
		else
		{
			writeToLogFile("Failed to get ImpREC Plugin\r\n");
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

/*void checkCallingConvention() 
{
	__asm {
		push eax
		push 0
		push 0
		push 0
		push 0
		push 0x1337
	}

	voidFunction();

	__asm {
		cmp dword ptr ss:[esp],0x1337
		je cdecl_call
		mov stdcallPlugin, 1
		jmp finished
cdecl_call:
		add esp, 0x14
		mov stdcallPlugin, 0
finished:
		pop eax
	}
}*/

BOOL getImprecPlugin() 
{
	HANDLE hImprecExchange = OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, PLUGIN_IMPREC_EXCHANGE_DLL_PATH); //open named file mapping object
	if (hImprecExchange == 0)
	{
		writeToLogFile("getImprecPlugin() -> OpenFileMappingA failed\r\n");
		return FALSE;
	}

	LPVOID lpImprecViewOfFile = MapViewOfFile(hImprecExchange, FILE_MAP_READ, 0, 0, 0); //map the view with full access

	if (lpImprecViewOfFile == 0)
	{
		CloseHandle(hImprecExchange); //close mapping handle
		writeToLogFile("getImprecPlugin() -> MapViewOfFile failed\r\n");
		return FALSE;
	}

	lstrcpyW(imprecPluginPath, (LPWSTR)lpImprecViewOfFile);

	UnmapViewOfFile(lpImprecViewOfFile);
	CloseHandle(hImprecExchange);

	wsprintfA(textBuffer, "- ImpREC Plugin ->  %S\r\n", imprecPluginPath);
	writeToLogFile(textBuffer);

	hImprecPlugin = LoadLibraryW(imprecPluginPath);

	if (hImprecPlugin)
	{
		voidFunction = (def_voidFunction)GetProcAddress(hImprecPlugin, "Trace");
		if (voidFunction)
		{
			return TRUE;
		}
		else
		{
			writeToLogFile("getImprecPlugin() -> Cannot find Trace method\r\n");
			return FALSE;
		}
	}
	else
	{
		wsprintfA(textBuffer, "getImprecPlugin() -> LoadLibraryW failed 0x%X\r\n", GetLastError());
		writeToLogFile(textBuffer);
		return FALSE;
	}
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
		lpViewOfFile = 0;
	}
	if (hMapFile != 0)
	{
		CloseHandle(hMapFile); //close mapping handle
		hMapFile = 0;
	}
	if (hImprecPlugin != 0)
	{
		FreeLibrary(hImprecPlugin);
		hImprecPlugin = 0;
	}
}

DWORD callImprecTraceFunction(DWORD hFileMap, DWORD dwSizeMap, DWORD dwTimeOut, DWORD dwToTrace, DWORD dwExactCall)
{
	DWORD retValue = 0;

	//some ImpREC Plugins use __cdecl and some other use __stdcall

	__asm {
		push eax
		xor eax, eax
		push eax
		push dwExactCall
		push dwToTrace
		push dwTimeOut
		push dwSizeMap
		push hFileMap
	}

	retValue = voidFunction();

	__asm {
		cmp dword ptr ss:[esp],0
		jne cdecl_call
		jmp finished
cdecl_call:
		add esp, 0x14
finished:
		pop eax
		pop eax
	}

	return retValue;
}

void resolveImports()
{
	PSCYLLA_EXCHANGE scyllaExchange = 0;
	PUNRESOLVED_IMPORT unresolvedImport = 0;
	DWORD pluginRet = 0;
	DWORD * pluginOutput = 0;
	LPVOID lpMapped = 0;
	HANDLE hPluginMap = 0, hPluginMapDup = 0;

	scyllaExchange = (PSCYLLA_EXCHANGE)lpViewOfFile;
	unresolvedImport = (PUNRESOLVED_IMPORT)((DWORD_PTR)scyllaExchange + scyllaExchange->offsetUnresolvedImportsArray);

	scyllaExchange->status = SCYLLA_STATUS_SUCCESS;

	hPluginMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE|SEC_COMMIT, 0, sizeof(DWORD), PLUGIN_MAPPING_NAME);

	if (hPluginMap == 0)
	{
		scyllaExchange->status = SCYLLA_STATUS_MAPPING_FAILED;
		writeToLogFile("resolveImports :: CreateFileMappingA hPluginMap failed\r\n");
		return;
	}

	while (unresolvedImport->ImportTableAddressPointer != 0) //last element is a nulled struct
	{

		if (!DuplicateHandle(GetCurrentProcess(), hPluginMap, GetCurrentProcess(), &hPluginMapDup, 0, FALSE, DUPLICATE_SAME_ACCESS))
		{
			wsprintfA(textBuffer,"resolveImports :: DuplicateHandle failed error code %d\r\n", GetLastError());
			writeToLogFile(textBuffer);
		}

		/*
		- hFileMap      : HANDLE of the file mapped by ImportREC
		- dwSizeMap     : Size of the mapped file
		- dwTimeOut     : TimeOut in ImportREC Options
		- dwToTrace     : The pointer to trace (in VA)
		- dwExactCall   : The EIP of the 'Exact Call' (in VA)
							(this value is 0 when it is not an 'Exact Call')
		*/

		pluginRet = callImprecTraceFunction((DWORD)hPluginMapDup, sizeof(DWORD), 200, unresolvedImport->InvalidApiAddress, 0);

		lpMapped = MapViewOfFile(hPluginMap, FILE_MAP_WRITE|FILE_MAP_READ, 0, 0, 0);

		if (lpMapped != 0)
		{
			pluginOutput = (DWORD *)lpMapped;

			wsprintfA(textBuffer, "- Plugin return value %d resolved import %X\r\n", pluginRet, *pluginOutput);
			writeToLogFile(textBuffer);

			if (*pluginOutput != 0)
			{
				unresolvedImport->InvalidApiAddress = *pluginOutput;
				*pluginOutput = 0;
			}
			else
			{
				scyllaExchange->status = SCYLLA_STATUS_IMPORT_RESOLVING_FAILED;
			}

			if (!UnmapViewOfFile(lpMapped))
			{
				wsprintfA(textBuffer,"resolveImports :: UnmapViewOfFile failed error code %d\r\n", GetLastError());
				writeToLogFile(textBuffer);
			}


			lpMapped = 0;
		}
		else
		{
			scyllaExchange->status = SCYLLA_STATUS_MAPPING_FAILED;
			wsprintfA(textBuffer,"resolveImports :: MapViewOfFile failed error code %d\r\n", GetLastError());
			writeToLogFile(textBuffer);
		}

		unresolvedImport++; //next pointer to struct
	}

	CloseHandle(hPluginMap);
}

BOOL getLogFilePath()
{
	size_t i = 0;

	if (!GetModuleFileNameA(0, logFilePath, sizeof(logFilePath))) //get full path of exe
	{
		return FALSE;
	}

	for (i = (lstrlenA(logFilePath) - 1); i >= 0; i--) //remove the exe file name from full path
	{
		if (logFilePath[i] == '\\')
		{
			logFilePath[i+1] = 0x00;
			break;
		}
	}

	if (lstrcatA(logFilePath,logFileName) == 0) //append log file name to path
	{
		return FALSE;
	}

	return TRUE;
}

BOOL writeToLogFile(const char * text)
{
	DWORD lpNumberOfBytesWritten = 0;
	BOOL wfRet = 0;
	HANDLE hFile = 0;

	hFile = CreateFileA(logFilePath, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); //open log file for writing

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