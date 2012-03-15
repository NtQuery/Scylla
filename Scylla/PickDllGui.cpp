#include "PickDllGui.h"

#include "definitions.h"

PickDllGui::PickDllGui(std::vector<ModuleInfo> &moduleList) : moduleList(moduleList)
{
	selectedModule = 0;

	prevColumn = -1;
	ascending = true;
}

BOOL PickDllGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange(); // attach controls
	DlgResize_Init(true, true);

	addColumnsToModuleList(ListDLLSelect);
	displayModuleList(ListDLLSelect);

	CenterWindow();
	return TRUE;
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

		swprintf_s(temp, TEXT(PRINTF_DWORD_PTR_FULL), iter->modBaseAddr);
		
		list.SetItemText(count, COL_IMAGEBASE, temp);

		swprintf_s(temp, L"%08X",iter->modBaseSize);
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
	bool ascending = (HIBYTE(lParamSort) == TRUE);

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
