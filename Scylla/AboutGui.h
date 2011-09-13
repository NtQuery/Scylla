#pragma once

#include "MainGui.h"


class AboutGui {
public:
	
	static INT_PTR initDialog(HINSTANCE hInstance, HWND hWndParent);

private:
	static LRESULT CALLBACK aboutDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};