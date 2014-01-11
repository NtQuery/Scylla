//#include <vld.h> // Visual Leak Detector

#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include "Architecture.h"

CAppModule _Module;

#include "MainGui.h"
#include "Scylla.h"

MainGui* pMainGui = NULL; // for Logger
HINSTANCE hDllModule = 0;
bool IsDllMode = false;

LONG WINAPI HandleUnknownException(struct _EXCEPTION_POINTERS *ExceptionInfo);
void AddExceptionHandler();
void RemoveExceptionHandler();
int InitializeGui(HINSTANCE hInstance, LPARAM param);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	AddExceptionHandler();

	return InitializeGui(hInstance, (LPARAM)0);
}

int InitializeGui(HINSTANCE hInstance, LPARAM param)
{
	CoInitialize(NULL);

	AtlInitCommonControls(ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES);

	Scylla::initAsGuiApp();

	IsDllMode = false;

	HRESULT hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	

	int nRet = 0;
	// BLOCK: Run application
	{
		MainGui dlgMain;
		pMainGui = &dlgMain; // o_O

		CMessageLoop loop;
		_Module.AddMessageLoop(&loop);

		dlgMain.Create(GetDesktopWindow(), param);

		dlgMain.ShowWindow(SW_SHOW);

		loop.Run();
	}

	_Module.Term();
	CoUninitialize();

	return nRet;
}

void InitializeDll(HINSTANCE hinstDLL)
{
	hDllModule = hinstDLL;
	IsDllMode = true;
	Scylla::initAsDll();
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	// Perform actions based on the reason for calling.
	switch(fdwReason) 
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		AddExceptionHandler();
		InitializeDll(hinstDLL);
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		RemoveExceptionHandler();
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

LPTOP_LEVEL_EXCEPTION_FILTER oldFilter;

void AddExceptionHandler()
{
	oldFilter = SetUnhandledExceptionFilter(HandleUnknownException);
}
void RemoveExceptionHandler()
{
	SetUnhandledExceptionFilter(oldFilter);
}

LONG WINAPI HandleUnknownException(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	WCHAR registerInfo[220];
	WCHAR message[159 + _countof(registerInfo)];
	DWORD_PTR address = (DWORD_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;
	
	swprintf_s(message, _countof(message), TEXT("ExceptionCode %08X\r\nExceptionFlags %08X\r\nNumberParameters %08X\r\nExceptionAddress VA ")TEXT(PRINTF_DWORD_PTR_FULL_S)TEXT("\r\nExceptionAddress RVA ")TEXT(PRINTF_DWORD_PTR_FULL_S)TEXT("\r\n\r\n"), 
	ExceptionInfo->ExceptionRecord->ExceptionCode,
	ExceptionInfo->ExceptionRecord->ExceptionFlags, 
	ExceptionInfo->ExceptionRecord->NumberParameters, 
	address, 
	address - (DWORD_PTR)GetModuleHandle(NULL));

#ifdef _WIN64
	swprintf_s(registerInfo, _countof(registerInfo),TEXT("rax=0x%p, rbx=0x%p, rdx=0x%p, rcx=0x%p, rsi=0x%p, rdi=0x%p, rbp=0x%p, rsp=0x%p, rip=0x%p"),
		ExceptionInfo->ContextRecord->Rax,
		ExceptionInfo->ContextRecord->Rbx,
		ExceptionInfo->ContextRecord->Rdx,
		ExceptionInfo->ContextRecord->Rcx,
		ExceptionInfo->ContextRecord->Rsi,
		ExceptionInfo->ContextRecord->Rdi,
		ExceptionInfo->ContextRecord->Rbp,
		ExceptionInfo->ContextRecord->Rsp,
		ExceptionInfo->ContextRecord->Rip
		);
#else
	swprintf_s(registerInfo, _countof(registerInfo),TEXT("eax=0x%p, ebx=0x%p, edx=0x%p, ecx=0x%p, esi=0x%p, edi=0x%p, ebp=0x%p, esp=0x%p, eip=0x%p"),
		ExceptionInfo->ContextRecord->Eax,
		ExceptionInfo->ContextRecord->Ebx,
		ExceptionInfo->ContextRecord->Edx,
		ExceptionInfo->ContextRecord->Ecx,
		ExceptionInfo->ContextRecord->Esi,
		ExceptionInfo->ContextRecord->Edi,
		ExceptionInfo->ContextRecord->Ebp,
		ExceptionInfo->ContextRecord->Esp,
		ExceptionInfo->ContextRecord->Eip
		);
#endif

	wcscat_s(message, _countof(message), registerInfo);

	MessageBox(0, message, TEXT("Exception! Please report it!"), MB_ICONERROR);

	return EXCEPTION_EXECUTE_HANDLER;
}
