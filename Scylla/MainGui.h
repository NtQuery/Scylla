#pragma once

//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>

#include <windows.h>
#include <Commctrl.h>
#include <stdio.h>
#include <map>
#include <Windowsx.h>

// WTL

#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlframe.h>      // WTL frame window classes
#include <atlmisc.h>       // WTL utility classes like CString
#include <atlcrack.h>      // WTL enhanced msg map macros

#include <atlctrls.h>

#include "resource.h"
#include "Logger.h"
#include "ProcessLister.h"
#include "IATSearch.h"
#include "PickDllGui.h"

#include "ImportsHandling.h"

//#pragma comment(lib, "comctl32.lib")

class ImportsHandling;

class MainGui : public CDialogImpl<MainGui>
{
public:
	enum { IDD = IDD_DLG_MAIN };

	BEGIN_MSG_MAP(MainGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CONTEXTMENU(OnContextMenu)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)

		NOTIFY_HANDLER_EX(IDC_TREE_IMPORTS, NM_CLICK, OnTreeImportsClick)
		NOTIFY_HANDLER_EX(IDC_TREE_IMPORTS, NM_DBLCLK, OnTreeImportsDoubleClick)
		NOTIFY_HANDLER_EX(IDC_TREE_IMPORTS, NM_RCLICK, OnTreeImportsRightClick)
		NOTIFY_HANDLER_EX(IDC_TREE_IMPORTS, NM_RDBLCLK, OnTreeImportsRightDoubleClick)

		COMMAND_HANDLER_EX(IDC_CBO_PROCESSLIST, CBN_DROPDOWN, OnProcessListDrop)
		COMMAND_HANDLER_EX(IDC_CBO_PROCESSLIST, CBN_SELENDOK, OnProcessListSelected)

		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKDLL, OnPickDLL)
		COMMAND_ID_HANDLER_EX(IDC_BTN_OPTIONS, OnOptions)
		COMMAND_ID_HANDLER_EX(IDC_BTN_DUMP, OnDump)
		COMMAND_ID_HANDLER_EX(IDC_BTN_FIXDUMP, OnFixDump)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PEREBUILD, OnPERebuild)
		COMMAND_ID_HANDLER_EX(IDC_BTN_DLLINJECT, OnDLLInject)
		COMMAND_ID_HANDLER_EX(IDC_BTN_IATAUTOSEARCH, OnIATAutoSearch)
		COMMAND_ID_HANDLER_EX(IDC_BTN_GETIMPORTS, OnGetImports)
		COMMAND_ID_HANDLER_EX(IDC_BTN_INVALIDIMPORTS, OnInvalidImports)
		COMMAND_ID_HANDLER_EX(IDC_BTN_SUSPECTIMPORTS, OnSuspectImports)
		COMMAND_ID_HANDLER_EX(IDC_BTN_CLEARIMPORTS, OnClearImports)
		COMMAND_ID_HANDLER_EX(IDC_BTN_CLEARLOG, OnClearLog)

		COMMAND_ID_HANDLER_EX(ID_FILE_EXIT, OnExit)
		COMMAND_ID_HANDLER_EX(ID_MISC_DLLINJECTION, OnDLLInject)
		COMMAND_ID_HANDLER_EX(ID_MISC_PREFERENCES, OnOptions)
		COMMAND_ID_HANDLER_EX(ID_HELP_ABOUT, OnAbout)

		COMMAND_ID_HANDLER_EX(IDCANCEL, OnExit)
	END_MSG_MAP()

	MainGui(HINSTANCE hInstance);

	Process * selectedProcess;

	//Output Window
	void addTextToOutputLog(const WCHAR * text);

	static DWORD_PTR stringToDwordPtr(WCHAR * hexString);

protected:
	HINSTANCE hInstance;

	// Controls
	CTreeViewCtrl TreeImports;
	CComboBox ComboProcessList;
	CEdit EditOEPAddress;
	CEdit EditIATAddress;
	CEdit EditIATSize;
	CListBox ListLog;

	ProcessLister processLister;
	WCHAR stringBuffer[600];

	ImportsHandling importsHandling;
	ProcessAccessHelp processAccessHelp;
	ApiReader apiReader;

private:

	//Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnLButtonDown(UINT nFlags, CPoint point);
	void OnContextMenu(CWindow wnd, CPoint point);

	LRESULT OnTreeImportsClick(const NMHDR* pnmh);
	LRESULT OnTreeImportsDoubleClick(const NMHDR* pnmh);
	LRESULT OnTreeImportsRightClick(const NMHDR* pnmh);
	LRESULT OnTreeImportsRightDoubleClick(const NMHDR* pnmh);

	void OnProcessListDrop(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnProcessListSelected(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnPickDLL(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOptions(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDump(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnFixDump(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnPERebuild(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDLLInject(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnIATAutoSearch(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnGetImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnInvalidImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnSuspectImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnClearImports(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnClearLog(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnExit(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnAbout(UINT uNotifyCode, int nID, CWindow wndCtl);

	// ---

	void setIconAndDialogCaption();

	void fillProcessListComboBox(CComboBox& hCombo);
	void getModuleListItem(int column, int iItem, char * buffer);

	void leftButtonDownActionHandler(WPARAM wParam, LPARAM lParam);
	void dialogInitActionHandler();
	void pickDllActionHandler();
	void processSelectedActionHandler(LRESULT index);

	//static bool displayModuleList(HWND hWndDlg, HWND hList, LRESULT index);

	// POPUP MENU Prototypes
	//bool OnContextMenu(int, int);
	void DisplayContextMenu(HWND, POINT);
	HWND mouseInDialogItem(int, POINT);
	void DisplayContextMenuImports(HWND, POINT);
	HMENU getCorrectSubMenu(int, int);

	
	void clearOutputLog();//Output Window
	void showInvalidImportsActionHandler();
	void showSuspectImportsActionHandler();
	void iatAutosearchActionHandler();
	void getImportsActionHandler();
	void appendPluginListToMenu( HMENU hMenuTrackPopup );
	void dumpActionHandler();
	DWORD_PTR getOEPFromGui();
	void peRebuildActionHandler();

	void startDisassemblerGui(HTREEITEM selectedTreeNode);
	void dumpFixActionHandler();
	void enableDialogButtons( BOOL value );
	void showAboutDialog();
	void dllInjectActionHandler();
	void optionsActionHandler();
	void pluginActionHandler( int menuItem );
};