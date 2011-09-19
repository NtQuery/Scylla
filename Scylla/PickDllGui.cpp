#include "PickDllGui.h"

#include "WindowDeferrer.h"

PickDllGui::PickDllGui(std::vector<ModuleInfo> &moduleList) : moduleList(moduleList)
{
	selectedModule = 0;
	hIcon.LoadIcon(IDI_ICON_SCYLLA);
}

BOOL PickDllGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	ListDLLSelect.Attach(GetDlgItem(IDC_LIST_DLLSELECT));

	addColumnsToModuleList(ListDLLSelect);
	displayModuleList(ListDLLSelect);

	CenterWindow();

	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	GetWindowRect(&MinSize);

	return TRUE;
}

void PickDllGui::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = MinSize.right - MinSize.left;
	lpMMI->ptMinTrackSize.y = MinSize.bottom - MinSize.top;
}

void PickDllGui::OnSizing(UINT fwSide, RECT* pRect)
{
	int toResize[] = {IDC_LIST_DLLSELECT};
	int toMove[] = {IDC_BTN_PICKDLL_OK, IDC_BTN_PICKDLL_CANCEL};

	WindowDeferrer::Deferrable controls[] =
	{
		{IDC_LIST_DLLSELECT, false, false, true, true},
		{IDC_BTN_PICKDLL_OK, true, true, false, false},
		{IDC_BTN_PICKDLL_CANCEL, true, true, false, false},
	};

	// Get size difference
	RECT rectOld;
	GetWindowRect(&rectOld);
	long deltaX = (pRect->right  - pRect->left) - (rectOld.right  - rectOld.left);
	long deltaY = (pRect->bottom - pRect->top)  - (rectOld.bottom - rectOld.top);

	WindowDeferrer deferrer(m_hWnd, controls, _countof(controls));
	deferrer.defer(deltaX, deltaY);
}

void PickDllGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int index = ListDLLSelect.GetSelectionMark();
	if (index != -1)
	{
		selectedModule = &moduleList[index];
		EndDialog(1);
	}
}

void PickDllGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void PickDllGui::addColumnsToModuleList(CListViewCtrl& list)
{
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	list.InsertColumn(COL_NAME, L"Name", LVCFMT_LEFT);
	list.InsertColumn(COL_IMAGEBASE, L"ImageBase", LVCFMT_CENTER);
	list.InsertColumn(COL_IMAGESIZE, L"ImageSize", LVCFMT_CENTER);
	list.InsertColumn(COL_PATH, L"Path", LVCFMT_LEFT);
}

void PickDllGui::displayModuleList(CListViewCtrl& list)
{
	WCHAR temp[20];

	list.DeleteAllItems();

	std::vector<ModuleInfo>::const_iterator iter;
	int count = 0;

	for( iter = moduleList.begin(); iter != moduleList.end(); iter++ , count++)
	{
		list.InsertItem(count, iter->getFilename());

#ifdef _WIN64
		swprintf_s(temp, _countof(temp), L"%016I64X", iter->modBaseAddr);
#else
		swprintf_s(temp, _countof(temp), L"%08X", iter->modBaseAddr);
#endif
		list.SetItemText(count, COL_IMAGEBASE, temp);

		swprintf_s(temp, _countof(temp),L"%08X",iter->modBaseSize);
		list.SetItemText(count, COL_IMAGESIZE, temp);

		list.SetItemText(count, COL_PATH, iter->fullPath);
	}

	list.SetColumnWidth(COL_NAME, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_IMAGEBASE, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_IMAGESIZE, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_PATH, LVSCW_AUTOSIZE_USEHEADER);
}
