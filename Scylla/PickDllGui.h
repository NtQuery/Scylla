#pragma once

#include "MainGui.h"

static const enum ListColumns {
	COL_PATH,
	COL_NAME,
	COL_IMAGEBASE,
	COL_IMAGESIZE
};

class PickDllGui {
public:
	static HWND hWndDlg;

	static std::vector<ModuleInfo> * moduleList;

	static ModuleInfo * selectedModule;

	static LRESULT CALLBACK pickDllDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR initDialog(HINSTANCE hInstance, HWND hWndParent, std::vector<ModuleInfo> &moduleList);

	static void addColumnsToModuleList(HWND hList);

	static void getModuleListItem(int column, int iItem, WCHAR * buffer);

	static bool displayModuleList();
};