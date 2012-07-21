//#include <vld.h> // Visual Leak Detector

#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes

CAppModule _Module;

#include "MainGui.h"

MainGui* pMainGui = NULL; // for Logger

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	CoInitialize(NULL);

	AtlInitCommonControls(ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES);

	HRESULT hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;
	// BLOCK: Run application
	{
		MainGui dlgMain;
		pMainGui = &dlgMain; // o_O

		CMessageLoop loop;
		_Module.AddMessageLoop(&loop);

		dlgMain.Create(GetDesktopWindow());
		dlgMain.ShowWindow(SW_SHOW);

		loop.Run();
	}

	_Module.Term();
	CoUninitialize();

	return nRet;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	// Perform actions based on the reason for calling.
	switch(fdwReason) 
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
