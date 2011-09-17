#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes

CAppModule _Module;

#include "MainGui.h"

MainGui* pMainGui = NULL; // for Logger

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	HRESULT hRes = CoInitialize(NULL);
	// If you are running on NT 4.0 or higher you can use the following call instead to 
	// make the EXE free threaded. This means that calls come in on a random RPC thread.
	//HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES); // add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = 0;
	// BLOCK: Run application
	{
		MainGui dlgMain;

		pMainGui = &dlgMain; // o_O

		nRet = dlgMain.DoModal();
	}

	_Module.Term();
	CoUninitialize();

	return nRet;
}
