#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlmisc.h>       // WTL utility classes like CString
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls

#include <vector>
#include "ProcessAccessHelp.h"

class PickDllGui : public CDialogImpl<PickDllGui>
{
public:
	enum { IDD = IDD_DLG_PICKDLL };

	BEGIN_MSG_MAP(PickDllGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		MSG_WM_SIZING(OnSizing)

		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKDLL_OK, OnOK)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKDLL_CANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

	PickDllGui(std::vector<ModuleInfo> &moduleList) : moduleList(moduleList), selectedModule(0) { }

	ModuleInfo* getSelectedModule() const { return selectedModule; }

protected:

	// Variables

	std::vector<ModuleInfo> &moduleList;
	ModuleInfo* selectedModule;

	// Controls

	CListViewCtrl ListDLLSelect;

	enum ListColumns {
		COL_PATH,
		COL_NAME,
		COL_IMAGEBASE,
		COL_IMAGESIZE
	};

	RECT MinSize;

	// Handles

	CIcon hIcon;

protected:

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	void OnSizing(UINT fwSide, RECT* pRect);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI functions

	void addColumnsToModuleList(CListViewCtrl& list);
	void displayModuleList(CListViewCtrl& list);
};
