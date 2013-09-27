#include "DonateGui.h"

#include "Scylla.h"
#include "Architecture.h"

const WCHAR DonateGui::TEXT_DONATE[] = L"If you like this tool, please feel free to donate some Bitcoins to support this project.\n\n\nBTC Address:\n\n" TEXT(DONATE_BTC_ADDRESS);


BOOL DonateGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange(); // attach controls

	DonateInfo.SetWindowText(TEXT_DONATE);

	CenterWindow();

	// Set focus to button
	GotoDlgCtrl(GetDlgItem(IDC_BUTTON_COPYBTC));
	return FALSE;
}

void DonateGui::OnClose()
{
	EndDialog(0);
}

void DonateGui::OnExit(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	SendMessage(WM_CLOSE);
}

void DonateGui::CopyBtcAddress(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if(OpenClipboard())
	{
		EmptyClipboard();
		size_t len = strlen(DONATE_BTC_ADDRESS);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(CHAR));
		if(hMem)
		{
			strcpy_s(static_cast<CHAR *>(GlobalLock(hMem)), len + 1, DONATE_BTC_ADDRESS);
			GlobalUnlock(hMem);
			if(!SetClipboardData(CF_TEXT, hMem))
			{
				GlobalFree(hMem);
			}
		}
		CloseClipboard();
	}
}