#include "AboutGui.h"
#include "definitions.h"

INT_PTR AboutGui::initDialog(HINSTANCE hInstance, HWND hWndParent)
{
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_ABOUT),hWndParent, (DLGPROC)aboutDlgProc);
}

LRESULT CALLBACK AboutGui::aboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText(hWnd,IDC_STATIC_ABOUT, TEXT(APPNAME)TEXT(" ")TEXT(ARCHITECTURE)TEXT(" ")TEXT(APPVERSION)TEXT("\n\n")TEXT(DEVELOPED)TEXT("\n\n\n")TEXT(CREDIT_DISTORM)TEXT("\n")TEXT(CREDIT_YODA)TEXT("\n\n")TEXT(GREETINGS)TEXT("\n\n\n")TEXT(VISIT));
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BTN_ABOUT_OK:
			EndDialog(hWnd, 0);
			return TRUE;
		}
	}
	return FALSE;
}