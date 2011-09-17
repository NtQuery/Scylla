#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls

class ConfigObject;

class OptionsGui : public CDialogImpl<OptionsGui>
{
public:
	enum { IDD = IDD_DLG_OPTIONS };

	BEGIN_MSG_MAP(OptionsGui)
		MSG_WM_INITDIALOG(OnInitDialog)

		COMMAND_ID_HANDLER_EX(IDC_BTN_OPTIONS_OK, OnOK)
		COMMAND_ID_HANDLER_EX(IDC_BTN_OPTIONS_CANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

private:

	CEdit EditSectionName;

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);

	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	void saveOptions();
	void loadOptions();
	void setCheckBox( int nIDDlgItem, bool bValue );
	void displayConfigInDlg( ConfigObject & config );
	void setEditControl( int nIDDlgItem, const WCHAR * valueString );
	void getConfigOptionsFromDlg( ConfigObject & config );

	bool getEditControl( int nIDDlgItem, WCHAR * valueString );
	void getCheckBox( int dialogItemValue, DWORD_PTR * valueNumeric );
	void getEditControlNumeric( int nIDDlgItem, DWORD_PTR * valueNumeric, int nBase );
};
