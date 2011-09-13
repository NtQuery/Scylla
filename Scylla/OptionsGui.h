#pragma once

#include "MainGui.h"

class ConfigObject;

class OptionsGui {
public:
	static HWND hWndDlg;
	static INT_PTR initOptionsDialog(HINSTANCE hInstance, HWND hWndParent);
private:
	static LRESULT CALLBACK optionsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static void saveOptions();
	static void loadOptions();
	static void setCheckBox( int nIDDlgItem, bool bValue );
	static void displayConfigInDlg( ConfigObject & config );
	static void setEditControl( int nIDDlgItem, const WCHAR * valueString );
	static void getConfigOptionsFromDlg( ConfigObject & config );

	static bool getEditControl( int nIDDlgItem, WCHAR * valueString );
	static void getCheckBox( int dialogItemValue, DWORD_PTR * valueNumeric );
	static void getEditControlNumeric( int nIDDlgItem, DWORD_PTR * valueNumeric, int nBase );
};