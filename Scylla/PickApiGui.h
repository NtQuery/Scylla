#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlframe.h>      // WTL window frame helpers
#include <atlmisc.h>       // WTL utility classes
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls
#include <atlddx.h>        // WTL dialog data exchange

#include <vector>
#include "ProcessAccessHelp.h"

class PickApiGui : public CDialogImpl<PickApiGui>, public CWinDataExchange<PickApiGui>, public CDialogResize<PickApiGui>
{
public:
	enum { IDD = IDD_DLG_PICKAPI };

	BEGIN_DDX_MAP(PickApiGui)
		DDX_CONTROL_HANDLE(IDC_CBO_DLLSELECT, ComboDllSelect)
		DDX_CONTROL_HANDLE(IDC_LIST_APISELECT, ListApiSelect)
		DDX_CONTROL_HANDLE(IDC_EDIT_APIFILTER, EditApiFilter)
	END_DDX_MAP()

	BEGIN_MSG_MAP(PickDllGui)
		MSG_WM_INITDIALOG(OnInitDialog)

		COMMAND_HANDLER_EX(IDC_CBO_DLLSELECT, CBN_SELENDOK, OnDllListSelected)
		COMMAND_HANDLER_EX(IDC_LIST_APISELECT, LBN_DBLCLK, OnApiListDoubleClick)
		COMMAND_HANDLER_EX(IDC_EDIT_APIFILTER, EN_UPDATE, OnApiFilterUpdated)

		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKAPI_OK, OnOK)
		COMMAND_ID_HANDLER_EX(IDC_BTN_PICKAPI_CANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)

		CHAIN_MSG_MAP(CDialogResize<PickApiGui>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(PickApiGui)
		DLGRESIZE_CONTROL(IDC_GROUP_DLL,     DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_CBO_DLLSELECT, DLSZ_SIZE_X)

		DLGRESIZE_CONTROL(IDC_GROUP_APIS,       DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_LIST_APISELECT,   DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_APIFILTER, DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EDIT_APIFILTER,   DLSZ_MOVE_Y | DLSZ_SIZE_X)

		DLGRESIZE_CONTROL(IDC_BTN_PICKAPI_OK,     DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_PICKAPI_CANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	PickApiGui(const std::vector<ModuleInfo> &moduleList);

	ApiInfo* getSelectedApi() const { return selectedApi; }

protected:

	// Variables

	const std::vector<ModuleInfo> &moduleList;
	ApiInfo* selectedApi;

	// Controls

	CComboBox ComboDllSelect;
	CListBox ListApiSelect;
	CEdit EditApiFilter;

protected:

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	void OnDllListSelected(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnApiListDoubleClick(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnApiFilterUpdated(UINT uNotifyCode, int nID, CWindow wndCtl);

	// Actions

	void actionApiSelected();

	// GUI functions

	void fillDllComboBox(CComboBox& combo);
	void fillApiListBox(CListBox& list, const std::vector<ApiInfo *> &apis);
};
