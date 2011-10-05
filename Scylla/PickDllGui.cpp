#include "PickDllGui.h"

#include "WindowDeferrer.h"

PickDllGui::PickDllGui(std::vector<ModuleInfo> &moduleList) : moduleList(moduleList)
{
	prevColumn = -1;
	ascending = true;

	selectedModule = 0;
	hIcon.LoadIcon(IDI_ICON_SCYLLA);
}

BOOL PickDllGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange(); // attach controls

	addColumnsToModuleList(ListDLLSelect);
	displayModuleList(ListDLLSelect);

	CenterWindow();

	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	GetWindowRect(&minDlgSize);

	return TRUE;
}

void PickDllGui::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize = CPoint(minDlgSize.Size());
}

void PickDllGui::OnSizing(UINT fwSide, RECT* pRect)
{
	// Get size difference
	CRect rectOld;
	GetWindowRect(&rectOld);
	CRect rectNew = *pRect;

	sizeOffset = rectNew.Size() - rectOld.Size();
}

void PickDllGui::OnSize(UINT nType, CSize size)
{
	const WindowDeferrer::Deferrable controls[] =
	{
		{IDC_LIST_DLLSELECT, false, false, true, true},
		{IDC_BTN_PICKDLL_OK, true, true, false, false},
		{IDC_BTN_PICKDLL_CANCEL, true, true, false, false},
	};

	if(nType == SIZE_RESTORED)
	{
		WindowDeferrer deferrer(m_hWnd, controls, _countof(controls));
		deferrer.defer(sizeOffset.cx, sizeOffset.cy);
		sizeOffset.SetSize(0, 0);
	}
}

LRESULT PickDllGui::OnListDllColumnClicked(NMHDR* pnmh)
{
	NMLISTVIEW* list = (NMLISTVIEW*)pnmh;
	int column = list->iSubItem;

	if(column == prevColumn)
	{
		ascending = !ascending;
	}
	else
	{
		prevColumn = column;
		ascending = true;
	}

	// lo-byte: column, hi-byte: sort-order
	ListDLLSelect.SortItems(&listviewCompareFunc, MAKEWORD(column, ascending));

	return 0;
}

LRESULT PickDllGui::OnListDllDoubleClick(NMHDR* pnmh)
{
	NMITEMACTIVATE* ia = (NMITEMACTIVATE*)pnmh;
	LVHITTESTINFO hti;
	hti.pt = ia->ptAction;
	int clicked = ListDLLSelect.HitTest(&hti);
	if(clicked != -1)
	{
		selectedModule = (ModuleInfo *)ListDLLSelect.GetItemData(clicked);
		EndDialog(1);
	}
	return 0;
}

void PickDllGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int index = ListDLLSelect.GetSelectionMark();
	if (index != -1)
	{
		selectedModule = (ModuleInfo *)ListDLLSelect.GetItemData(index);
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

		list.SetItemData(count, (DWORD_PTR)&(*iter));
	}

	list.SetColumnWidth(COL_NAME, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_IMAGEBASE, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_IMAGESIZE, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_PATH, LVSCW_AUTOSIZE_USEHEADER);
}

// lParamSort - lo-byte: column, hi-byte: sort-order
int PickDllGui::listviewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const ModuleInfo * module1 = (ModuleInfo *)lParam1;
	const ModuleInfo * module2 = (ModuleInfo *)lParam2;

	int column = LOBYTE(lParamSort);
	bool ascending = HIBYTE(lParamSort) == true;

	int diff = 0;

	switch(column)
	{
	case COL_NAME:
		diff = _wcsicmp(module1->getFilename(), module2->getFilename());
		break;
	case COL_IMAGEBASE:
		diff = module1->modBaseAddr < module2->modBaseAddr ? -1 : 1;
		break;
	case COL_IMAGESIZE:
		diff = module1->modBaseSize < module2->modBaseSize ? -1 : 1;
		break;
	case COL_PATH:
		diff = _wcsicmp(module1->fullPath, module2->fullPath);
		break;
	}

	return ascending ? diff : -diff;
}
