#include "PickDllGui.h"


HWND PickDllGui::hWndDlg;
std::vector<ModuleInfo> * PickDllGui::moduleList = 0;
ModuleInfo * PickDllGui::selectedModule = 0;

INT_PTR PickDllGui::initDialog(HINSTANCE hInstance, HWND hWndParent, std::vector<ModuleInfo> &moduleListNew)
{
	moduleList = &moduleListNew;
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_PICKDLL),hWndParent, (DLGPROC)pickDllDlgProc);
}

LRESULT CALLBACK PickDllGui::pickDllDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int index;
	hWndDlg = hWnd;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		addColumnsToModuleList(GetDlgItem(hWnd, IDC_LIST_DLLSELECT));
		displayModuleList();
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BTN_PICKDLL_OK:
			index = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_LIST_DLLSELECT));
			if (index != -1)
			{
				selectedModule = &(*moduleList).at(index);
				EndDialog(hWnd, 1);
			}

			return TRUE;
		case IDC_BTN_PICKDLL_CANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		}
	}
	return FALSE;
}

void PickDllGui::addColumnsToModuleList(HWND hList)
{
	if (hList)
	{
		LVCOLUMN * lvc = (LVCOLUMN*)malloc(sizeof(LVCOLUMN));

		ListView_SetExtendedListViewStyleEx(hList,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);

		lvc->mask = LVCF_TEXT | LVCF_WIDTH;
		lvc->cx = 210;
		lvc->pszText = L"Path";
		ListView_InsertColumn(hList, COL_PATH, lvc);

		lvc->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvc->fmt = LVCFMT_CENTER;
		lvc->cx = 130;
		lvc->pszText = L"Name";
		ListView_InsertColumn(hList, COL_NAME, lvc);

		lvc->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvc->fmt = LVCFMT_CENTER;
		lvc->cx = 70;
		lvc->pszText = L"ImageBase";
		ListView_InsertColumn(hList, COL_IMAGEBASE, lvc);

		lvc->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvc->fmt = LVCFMT_CENTER;
		lvc->cx = 70;
		lvc->pszText = L"ImageSize";
		ListView_InsertColumn(hList, COL_IMAGESIZE, lvc);

		free(lvc);
	}
}

void PickDllGui::getModuleListItem(int column, int iItem, WCHAR * buffer)
{
	LVITEM pitem = {0};
	pitem.iItem = iItem;
	pitem.iSubItem = column;
	pitem.mask = LVIF_TEXT;
	pitem.cchTextMax = 256;
	pitem.pszText = buffer;
	ListView_GetItem(GetDlgItem(hWndDlg, IDC_LIST_DLLSELECT),&pitem);
}

bool PickDllGui::displayModuleList()
{
	LVITEM item;
	WCHAR temp[20];
	HWND hList = GetDlgItem(hWndDlg, IDC_LIST_DLLSELECT);

	ListView_DeleteAllItems(hList);

	item.mask = LVIF_TEXT;



	std::vector<ModuleInfo>::iterator iter;
	int count = 0;

	for( iter = (*moduleList).begin(); iter != (*moduleList).end(); iter++ , count++) {
		item.iItem = count;
		item.iSubItem = COL_PATH;
		item.pszText = iter->fullPath;
		item.iItem = ListView_InsertItem(hList, &item);

		item.iSubItem = COL_NAME;
		item.pszText = iter->getFilename();
		ListView_SetItem(hList, &item);

		item.iSubItem = COL_IMAGEBASE;
		swprintf_s(temp,_countof(temp),L"%08X",iter->modBaseAddr);
		item.pszText = temp;
		ListView_SetItem(hList, &item);

		item.iSubItem = COL_IMAGESIZE;
		swprintf_s(temp,_countof(temp),L"%08X",iter->modBaseSize);
		item.pszText = temp;
		ListView_SetItem(hList, &item);		
	}

	return true;
}