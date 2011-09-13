#pragma once

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>
#include <map>
#include <Windowsx.h>

#include "resource.h"
#include "Logger.h"
#include "ProcessLister.h"
#include "IATSearch.h"
#include "PickDllGui.h"


#pragma comment(lib, "comctl32.lib")


#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

class ImportsHandling;

class MainGui
{
public:
	static HWND hWndMainDlg;
	static Process * selectedProcess;

	static void initDialog(HINSTANCE hInstance);

	//Output Window
	static void addTextToOutputLog(const WCHAR * text);

	static DWORD_PTR stringToDwordPtr(WCHAR * hexString);

protected:
	static HWND hWndParent;

	static HINSTANCE hInstance;
	static ProcessLister processLister;
	static WCHAR stringBuffer[300];

	static ImportsHandling importsHandling;
	static ProcessAccessHelp processAccessHelp;
	static ApiReader apiReader;

private:

	static LRESULT CALLBACK mainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static void setIconAndDialogCaption();
	

	static void fillProcessListComboBox(HWND hCombo);
	static void getModuleListItem(int column, int iItem, char * buffer);


	static void leftButtonDownActionHandler(WPARAM wParam, LPARAM lParam);
	static void dialogInitActionHandler();
	static void pickDllActionHandler();
	static void processSelectedActionHandler(LRESULT index);

	//static bool displayModuleList(HWND hWndDlg, HWND hList, LRESULT index);


	// POPUP MENU Prototypes
	static bool OnContextMenu(int, int);
	static void DisplayContextMenu(HWND, POINT);
	static HWND mouseInDialogItem(int, POINT);
	static void DisplayContextMenuImports(HWND, POINT);
	static HMENU getCorrectSubMenu(int, int);

	
	static void clearOutputLog();//Output Window
	static void showInvalidImportsActionHandler();
	static void showSuspectImportsActionHandler();
	static void iatAutosearchActionHandler();
	static void getImportsActionHandler();
	static void appendPluginListToMenu( HMENU hMenuTrackPopup );
	static void dumpActionHandler();
	static DWORD_PTR getOEPFromGui();
	static void peRebuildActionHandler();

	static void startDisassemblerGui(HTREEITEM selectedTreeNode);
	static void dumpFixActionHandler();
	static void enableDialogButtons( BOOL value );
	static void showAboutDialog();
	static void dllInjectActionHandler();
	static void optionsActionHandler();
};