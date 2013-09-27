#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls
#include <atlddx.h>        // WTL dialog data exchange

class DonateGui : public CDialogImpl<DonateGui>, public CWinDataExchange<DonateGui>
{
public:
	enum { IDD = IDD_DLG_DONATE };

	BEGIN_DDX_MAP(DonateGui)
		DDX_CONTROL_HANDLE(IDC_STATIC_DONATEINFO, DonateInfo)
	END_DDX_MAP()

	BEGIN_MSG_MAP(DonateGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CLOSE(OnClose)
		COMMAND_ID_HANDLER_EX(IDC_BUTTON_COPYBTC, CopyBtcAddress)

		COMMAND_ID_HANDLER_EX(IDOK, OnExit)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnExit)
	END_MSG_MAP()

protected:

	// Controls

	CStatic DonateInfo;

	// Texts
	static const WCHAR TEXT_DONATE[];

protected:

	// Message handlers
	void CopyBtcAddress(UINT uNotifyCode, int nID, CWindow wndCtl);
	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnClose();
	void OnExit(UINT uNotifyCode, int nID, CWindow wndCtl);

};