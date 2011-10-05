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
#include <atlddx.h>        // WTL dialog data exchange

#include <vector>
#include "ProcessAccessHelp.h"

class PickDllGui : public CDialogImpl<PickDllGui>, public CWinDataExchange<PickDllGui>
{
public:
	enum { IDD = IDD_DLG_PICKDLL };

	BEGIN_DDX_MAP(PickDllGui)
		DDX_CONTROL_HANDLE(IDC_LIST_DLLSELECT, ListDLLSelect)
	END_DDX_MAP()

	BEGIN_MSG_MAP(PickDllGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		MSG_WM_SIZING(OnSizing)
		MSG_WM_SIZE(OnSize)

		NOTIFY_HANDLER_EX(IDC_LIST_DLLSELECT, LVN_COLUMNCLICK, OnListDllColumnClicked)
		NOTIFY_HANDLER_EX(IDC_LIST_DLLSELECT, NM_DBLCLK, OnListDllDoubleClick)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKDLL_OK, OnOK)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKDLL_CANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

	PickDllGui(std::vector<ModuleInfo> &moduleList);

	ModuleInfo* getSelectedModule() const { return selectedModule; }

protected:

	// Variables

	std::vector<ModuleInfo> &moduleList;
	ModuleInfo* selectedModule;

	// Controls

	CListViewCtrl ListDLLSelect;

	enum ListColumns {
		COL_NAME = 0,
		COL_IMAGEBASE,
		COL_IMAGESIZE,
		COL_PATH
	};

	int prevColumn;
	bool ascending;

	CRect minDlgSize;
	CSize sizeOffset;

	// Handles

	CIcon hIcon;

protected:

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	void OnSizing(UINT fwSide, RECT* pRect);
	void OnSize(UINT nType, CSize size);

	LRESULT OnListDllColumnClicked(NMHDR* pnmh);
	LRESULT OnListDllDoubleClick(NMHDR* pnmh);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI functions

	void addColumnsToModuleList(CListViewCtrl& list);
	void displayModuleList(CListViewCtrl& list);

	static int CALLBACK listviewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
};
