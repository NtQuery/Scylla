#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls

class AboutGui : public CDialogImpl<AboutGui>
{
public:
	enum { IDD = IDD_DLG_ABOUT };

	BEGIN_MSG_MAP(AboutGui)
		MSG_WM_INITDIALOG(OnInitDialog)

		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

protected:

	// Controls

	CStatic StaticTitle;
	CStatic StaticAbout;

	// Handles

	CFontHandle FontBold;

protected:

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);
};
