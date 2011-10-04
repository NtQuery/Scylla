#include "AboutGui.h"

#include "definitions.h"

BOOL AboutGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	StaticTitle.Attach(GetDlgItem(IDC_STATIC_ABOUT_TITLE));
	StaticAbout.Attach(GetDlgItem(IDC_STATIC_ABOUT));

	LOGFONT lf;
	CFontHandle font = StaticTitle.GetFont();
	font.GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	FontBold.CreateFontIndirect(&lf);

	StaticTitle.SetFont(FontBold, FALSE);

	StaticTitle.SetWindowText(TEXT(APPNAME)TEXT(" ")TEXT(ARCHITECTURE)TEXT(" ")TEXT(APPVERSION));

	StaticAbout.SetWindowText(TEXT(DEVELOPED)TEXT("\n\n\n")TEXT(CREDIT_DISTORM)TEXT("\n")TEXT(CREDIT_YODA)TEXT("\n")TEXT(CREDIT_WTL)TEXT("\n")TEXT(CREDIT_SILK)TEXT("\n\n")TEXT(GREETINGS)TEXT("\n\n")TEXT(VISIT));
	CenterWindow();

	return TRUE;
}

void AboutGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	FontBold.DeleteObject();
	EndDialog(0);
}
