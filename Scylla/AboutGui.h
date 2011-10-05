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

class AboutGui : public CDialogImpl<AboutGui>, public CWinDataExchange<AboutGui>
{
public:
	enum { IDD = IDD_DLG_ABOUT };

	BEGIN_DDX_MAP(AboutGui)
		DDX_CONTROL_HANDLE(IDC_STATIC_ABOUT_TITLE, StaticTitle)
		DDX_CONTROL_HANDLE(IDC_STATIC_DEVELOPED, StaticDeveloped)
		DDX_CONTROL_HANDLE(IDC_STATIC_GREETINGS, StaticGreetings)
		DDX_CONTROL_HANDLE(IDC_STATIC_YODA, StaticYoda)
		DDX_CONTROL_HANDLE(IDC_SYSLINK_VISIT, LinkVisit)
		DDX_CONTROL_HANDLE(IDC_SYSLINK_DISTORM, LinkDistorm)
		DDX_CONTROL_HANDLE(IDC_SYSLINK_WTL, LinkWTL)
		DDX_CONTROL_HANDLE(IDC_SYSLINK_SILK, LinkSilk)
		DDX_CONTROL_HANDLE(IDC_SYSLINK_LICENSE, LinkLicense)
	END_DDX_MAP()

	BEGIN_MSG_MAP(AboutGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CLOSE(OnClose)

		NOTIFY_HANDLER_EX(IDC_SYSLINK_DISTORM, NM_CLICK, OnLink)
		NOTIFY_HANDLER_EX(IDC_SYSLINK_DISTORM, NM_RETURN, OnLink)
		NOTIFY_HANDLER_EX(IDC_SYSLINK_WTL, NM_CLICK, OnLink)
		NOTIFY_HANDLER_EX(IDC_SYSLINK_WTL, NM_RETURN, OnLink)
		NOTIFY_HANDLER_EX(IDC_SYSLINK_SILK, NM_CLICK, OnLink)
		NOTIFY_HANDLER_EX(IDC_SYSLINK_SILK, NM_RETURN, OnLink)
		NOTIFY_HANDLER_EX(IDC_SYSLINK_VISIT, NM_CLICK, OnLink)
		NOTIFY_HANDLER_EX(IDC_SYSLINK_VISIT, NM_RETURN, OnLink)

		COMMAND_ID_HANDLER_EX(IDOK, OnExit)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnExit)
	END_MSG_MAP()

protected:

	// Controls

	CStatic StaticTitle;
	CStatic StaticDeveloped;
	CStatic StaticGreetings;
	CStatic StaticYoda;

	CLinkCtrl LinkVisit;
	CLinkCtrl LinkDistorm;
	CLinkCtrl LinkWTL;
	CLinkCtrl LinkSilk;
	CLinkCtrl LinkLicense;

	CToolTipCtrl TooltipDistorm;
	CToolTipCtrl TooltipWTL;
	CToolTipCtrl TooltipSilk;
	CToolTipCtrl TooltipLicense;

	// Handles

	CFontHandle FontBold;

	// Texts

	static const WCHAR TEXT_VISIT[];
	static const WCHAR TEXT_DEVELOPED[];
	static const WCHAR TEXT_CREDIT_DISTORM[];
	static const WCHAR TEXT_CREDIT_YODA[];
	static const WCHAR TEXT_CREDIT_WTL[];
	static const WCHAR TEXT_CREDIT_SILK[];
	static const WCHAR TEXT_GREETINGS[];
	static const WCHAR TEXT_LICENSE[];

	// URLs

	static const WCHAR URL_VISIT1[];
	static const WCHAR URL_VISIT2[];
	static const WCHAR URL_DISTORM[];
	static const WCHAR URL_WTL[];
	static const WCHAR URL_SILK[];
	static const WCHAR URL_LICENSE[];

protected:

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnClose();
	LRESULT OnLink(NMHDR* pnmh);
	void OnExit(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI helpers

	void setupLinks();
	void setLinkURL(CLinkCtrl& link, const WCHAR* url, int index = 0);
	void setupTooltip(CToolTipCtrl tooltip, CWindow window, const WCHAR* text);
};
