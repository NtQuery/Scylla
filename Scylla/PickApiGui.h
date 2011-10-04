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

class PickApiGui : public CDialogImpl<PickApiGui>
{
public:
	enum { IDD = IDD_DLG_PICKAPI };

	BEGIN_MSG_MAP(PickDllGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)
		MSG_WM_SIZING(OnSizing)
		MSG_WM_SIZE(OnSize)

		COMMAND_HANDLER_EX(IDC_CBO_DLLSELECT, CBN_SELENDOK, OnDllListSelected)
		COMMAND_HANDLER_EX(IDC_EDIT_APIFILTER, EN_UPDATE, OnApiFilterUpdated)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKAPI_OK, OnOK)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKAPI_CANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

	PickApiGui(const std::vector<ModuleInfo> &moduleList);

	ApiInfo* getSelectedApi() const { return selectedApi; }

protected:

	// Variables

	const std::vector<ModuleInfo> &moduleList;
	std::vector<ApiInfo *> apiListTemp;
	ApiInfo* selectedApi;

	// Controls

	CComboBox ComboDllSelect;
	CListBox ListApiSelect;
	CEdit EditApiFilter;

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
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnDllListSelected(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnApiFilterUpdated(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI functions

	void fillDllComboBox(CComboBox& combo);
	void fillApiListBox(CListBox& list, const std::vector<ApiInfo *> &apis);

	//void displayModuleList(CListViewCtrl& list);
};
