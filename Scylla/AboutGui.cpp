#include "AboutGui.h"

#include "definitions.h"

const WCHAR AboutGui::TEXT_VISIT[]          = L"Visit <a>http://kickme.to/grn</a> and <a>http://forum.tuts4you.com</a>";
const WCHAR AboutGui::TEXT_DEVELOPED[]      = L"Developed with Microsoft Visual Studio, written in pure C/C++";
const WCHAR AboutGui::TEXT_CREDIT_DISTORM[] = L"This tool uses the <a>diStorm disassembler library</a> v3";
const WCHAR AboutGui::TEXT_CREDIT_YODA[]    = L"The PE Rebuilder engine is based on Realign DLL v1.5 by yoda";
const WCHAR AboutGui::TEXT_CREDIT_SILK[]    = L"The small icons are taken from the <a>Silk icon package</a>";
const WCHAR AboutGui::TEXT_CREDIT_WTL[]     = L"<a>Windows Template Library</a> v8 is used for the GUI";
const WCHAR AboutGui::TEXT_GREETINGS[]      = L"Greetz: metr0, G36KV and all from the gRn Team";
const WCHAR AboutGui::TEXT_LICENSE[]        = L"Scylla is licensed under the <a>GNU General Public License v3</a>";

const WCHAR AboutGui::URL_VISIT1[]  = L"http://kickme.to/grn";
const WCHAR AboutGui::URL_VISIT2[]  = L"http://forum.tuts4you.com";
const WCHAR AboutGui::URL_DISTORM[] = L"http://code.google.com/p/distorm/";
const WCHAR AboutGui::URL_WTL[]     = L"http://wtl.sourceforge.net";
const WCHAR AboutGui::URL_SILK[]    = L"http://www.famfamfam.com";
const WCHAR AboutGui::URL_LICENSE[] = L"http://www.gnu.org/licenses/gpl-3.0.html";

BOOL AboutGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	StaticTitle.Attach(GetDlgItem(IDC_STATIC_ABOUT_TITLE));
	StaticDeveloped.Attach(GetDlgItem(IDC_STATIC_DEVELOPED));
	StaticGreetings.Attach(GetDlgItem(IDC_STATIC_GREETINGS));
	StaticYoda.Attach(GetDlgItem(IDC_STATIC_YODA));

	LinkVisit.Attach(GetDlgItem(IDC_SYSLINK_VISIT));
	LinkDistorm.Attach(GetDlgItem(IDC_SYSLINK_DISTORM));
	LinkWTL.Attach(GetDlgItem(IDC_SYSLINK_WTL));
	LinkSilk.Attach(GetDlgItem(IDC_SYSLINK_SILK));
	LinkLicense.Attach(GetDlgItem(IDC_SYSLINK_LICENSE));

	LOGFONT lf;
	CFontHandle font = StaticTitle.GetFont();
	font.GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	FontBold.CreateFontIndirect(&lf);

	StaticTitle.SetFont(FontBold, FALSE);

	StaticTitle.SetWindowText(TEXT(APPNAME)TEXT(" ")TEXT(ARCHITECTURE)TEXT(" ")TEXT(APPVERSION));
	StaticDeveloped.SetWindowText(TEXT_DEVELOPED);
	StaticGreetings.SetWindowText(TEXT_GREETINGS);
	StaticYoda.SetWindowText(TEXT_CREDIT_YODA);

	setupLinks();

	CenterWindow();

	GotoDlgCtrl(GetDlgItem(IDOK));
	return FALSE;
}

void AboutGui::OnClose()
{
	TooltipDistorm.DestroyWindow();
	TooltipWTL.DestroyWindow();
	TooltipSilk.DestroyWindow();
	TooltipLicense.DestroyWindow();
	FontBold.DeleteObject();
	EndDialog(0);
}

LRESULT AboutGui::OnLink(NMHDR* pnmh)
{
	const NMLINK* link = (NMLINK*)pnmh;
	ShellExecute(NULL, L"open", link->item.szUrl, NULL, NULL, SW_SHOW);
	return 0;
}

void AboutGui::OnExit(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	SendMessage(WM_CLOSE);
}

void AboutGui::setupLinks()
{
	LITEM item;
	item.mask = LIF_ITEMINDEX | LIF_URL;
	item.iLink = 0;

	LinkDistorm.SetWindowText(TEXT_CREDIT_DISTORM);
	wcscpy_s(item.szUrl, _countof(item.szUrl), URL_DISTORM);
	LinkDistorm.SetItem(&item);

	LinkWTL.SetWindowText(TEXT_CREDIT_WTL);
	wcscpy_s(item.szUrl, _countof(item.szUrl), URL_WTL);
	LinkWTL.SetItem(&item);

	LinkSilk.SetWindowText(TEXT_CREDIT_SILK);
	wcscpy_s(item.szUrl, _countof(item.szUrl), URL_SILK);
	LinkSilk.SetItem(&item);

	LinkLicense.SetWindowText(TEXT_LICENSE);
	wcscpy_s(item.szUrl, _countof(item.szUrl), URL_LICENSE);
	LinkLicense.SetItem(&item);

	LinkVisit.SetWindowText(TEXT_VISIT);
	wcscpy_s(item.szUrl, _countof(item.szUrl), URL_VISIT1);
	LinkVisit.SetItem(&item);
	item.iLink = 1;
	wcscpy_s(item.szUrl, _countof(item.szUrl), URL_VISIT2);
	LinkVisit.SetItem(&item);

	TooltipDistorm.Create(m_hWnd, NULL, NULL, TTS_NOPREFIX, WS_EX_TOPMOST);
	TooltipWTL.Create(m_hWnd, NULL, NULL, TTS_NOPREFIX, WS_EX_TOPMOST);
	TooltipSilk.Create(m_hWnd, NULL, NULL, TTS_NOPREFIX, WS_EX_TOPMOST);
	TooltipLicense.Create(m_hWnd, NULL, NULL, TTS_NOPREFIX, WS_EX_TOPMOST);

	setupTooltip(TooltipDistorm, LinkDistorm, URL_DISTORM);
	setupTooltip(TooltipWTL, LinkWTL, URL_WTL);
	setupTooltip(TooltipSilk, LinkSilk, URL_SILK);
	setupTooltip(TooltipLicense, LinkLicense, URL_LICENSE);
}

void AboutGui::setupTooltip(CToolTipCtrl tooltip, CWindow window, const WCHAR* text)
{
	CToolInfo ti(TTF_SUBCLASS, window);

	window.GetClientRect(&ti.rect);
	ti.lpszText = const_cast<WCHAR *>(text);
	tooltip.AddTool(ti);
}
