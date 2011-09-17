#include "AboutGui.h"

#include "definitions.h"

BOOL AboutGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	StaticAbout.Attach(GetDlgItem(IDC_STATIC_ABOUT));

	StaticAbout.SetWindowText(TEXT(APPNAME)TEXT(" ")TEXT(ARCHITECTURE)TEXT(" ")TEXT(APPVERSION)TEXT("\n\n")TEXT(DEVELOPED)TEXT("\n\n\n")TEXT(CREDIT_DISTORM)TEXT("\n")TEXT(CREDIT_YODA)TEXT("\n\n")TEXT(GREETINGS)TEXT("\n\n\n")TEXT(VISIT));
	CenterWindow();

	return TRUE;
}

void AboutGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void AboutGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}
