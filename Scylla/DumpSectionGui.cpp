#include "DumpSectionGui.h"

#include "Architecture.h"
#include "ProcessAccessHelp.h"


bool PeSection::highlightVirtualSize()
{
	//highlight big virtual sizes -> anti-dump protection
	if (virtualSize > 0x2000000)
	{
		return true;
	}
	else
	{
		return false;
	}
}

std::vector<PeSection> & DumpSectionGui::getSectionList()
{
	return sectionList;
}

BOOL DumpSectionGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange(); // attach controls
	DlgResize_Init(true, true);

	addColumnsToSectionList(ListSectionSelect);
	displaySectionList(ListSectionSelect);

	selectOrDeselectAll();

	isEditing = false;
	selectedSection = 0;

	CenterWindow();
	return TRUE;
}

LRESULT DumpSectionGui::OnListSectionColumnClicked(NMHDR* pnmh)
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
	ListSectionSelect.SortItems(&listviewCompareFunc, MAKEWORD(column, ascending));

	return 0;
}
LRESULT DumpSectionGui::OnListSectionClick(NMHDR* pnmh)
{
	//int index = ListSectionSelect.GetSelectionMark();
	//if (index != -1)
	//{

	//}
	return 0;
}

LRESULT DumpSectionGui::OnListDoubleClick(NMHDR* pnmh)
{
	RECT rect;
	RECT rect1,rect2;
	NMITEMACTIVATE* ia = (NMITEMACTIVATE*)pnmh;

	if (ia->iSubItem != COL_VSize)
	{
		return 0;
	}

	LVHITTESTINFO hti;
	hti.pt = ia->ptAction;
	int clicked = ListSectionSelect.HitTest(&hti);
	if(clicked != -1)
	{
		selectedSection = (PeSection *)ListSectionSelect.GetItemData(clicked);
	}


	ListSectionSelect.GetSubItemRect(ia->iItem,ia->iSubItem,LVIR_BOUNDS,&rect);

	//Get the Rectange of the listControl
	ListSectionSelect.GetWindowRect(&rect1);
	//Get the Rectange of the Dialog
	GetWindowRect(&rect2);

	int x = rect1.left - rect2.left;
	int y = rect1.top - rect2.top;

	isEditing = true;

	EditListControl.SetWindowPos(HWND_TOP,rect.left + 7, rect.top + 7, rect.right - rect.left, rect.bottom - rect.top, NULL);
	EditListControl.ShowWindow(SW_SHOW);
	EditListControl.SetFocus();

	 //Draw a Rectangle around the SubItem
	//Rectangle(ListSectionSelect.GetDC(),rect.left,rect.top-1,rect.right,rect.bottom);
	 //Set the listItem text in the EditBox
	EditListControl.SetValue(selectedSection->virtualSize);

	return 0;
}

void DumpSectionGui::OnSectionSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	selectOrDeselectAll();
}

void DumpSectionGui::OnEditList(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	switch (uNotifyCode)
	{
	case EN_KILLFOCUS:
		{
			isEditing = false;
			
			updateEditedItem();
			
			EditListControl.ShowWindow(SW_HIDE);
		}
		break;
	}
}

void DumpSectionGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (isEditing) //EN_KILLFOCUS not sent?
	{
		updateEditedItem();
	}

	updateCheckState();

	EndDialog(1);
}
void DumpSectionGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

int DumpSectionGui::listviewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const PeSection * module1 = (PeSection *)lParam1;
	const PeSection * module2 = (PeSection *)lParam2;

	int column = LOBYTE(lParamSort);
	bool ascending = (HIBYTE(lParamSort) == TRUE);

	int diff = 0;

	switch(column)
	{
	case COL_NAME:
		diff = _wcsicmp(module1->name, module2->name);
		break;
	case COL_VA:
		diff = module1->virtualAddress < module2->virtualAddress ? -1 : 1;
		break;
	case COL_VSize:
		diff = module1->virtualSize < module2->virtualSize ? -1 : 1;
		break;
	case COL_RVA:
		diff = module1->rawAddress < module2->rawAddress ? -1 : 1;
		break;
	case COL_RSize:
		diff = module1->rawSize < module2->rawSize ? -1 : 1;
		break;
	case COL_Characteristics:
		diff = module1->characteristics < module2->characteristics ? -1 : 1;
		break;
	}

	return ascending ? diff : -diff;
}

void DumpSectionGui::addColumnsToSectionList(CListViewCtrl& list)
{
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT|LVS_EX_CHECKBOXES|LVS_EX_GRIDLINES);

	list.InsertColumn(COL_NAME, L"Name", LVCFMT_CENTER);
	list.InsertColumn(COL_VA, L"Virtual Address", LVCFMT_CENTER);
	list.InsertColumn(COL_VSize, L"Virtual Size", LVCFMT_CENTER);
	list.InsertColumn(COL_RVA, L"Raw Address", LVCFMT_CENTER);
	list.InsertColumn(COL_RSize, L"Raw Size", LVCFMT_CENTER);
	list.InsertColumn(COL_Characteristics, L"Characteristics", LVCFMT_CENTER);
}

void DumpSectionGui::displaySectionList(CListViewCtrl& list)
{
	int count = 0;
	WCHAR temp[20];

	list.DeleteAllItems();

	if (sectionList.empty())
	{
		getAllSectionsFromFile();
	}
	

	std::vector<PeSection>::const_iterator iter;

	for( iter = sectionList.begin(); iter != sectionList.end(); iter++ , count++)
	{
		list.InsertItem(count, iter->name);

		swprintf_s(temp, PRINTF_DWORD_PTR_FULL, iter->virtualAddress);
		list.SetItemText(count, COL_VA, temp);

		swprintf_s(temp, L"%08X", iter->virtualSize);
		list.SetItemText(count, COL_VSize, temp);

		swprintf_s(temp, L"%08X", iter->rawAddress);
		list.SetItemText(count, COL_RVA, temp);

		swprintf_s(temp, L"%08X", iter->rawSize);
		list.SetItemText(count, COL_RSize, temp);

		swprintf_s(temp, L"%08X", iter->characteristics);
		list.SetItemText(count, COL_Characteristics, temp);


		list.SetItemData(count, (DWORD_PTR)&(*iter));
	}

	list.SetColumnWidth(COL_NAME, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_VA, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_VSize, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_RVA, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_RSize, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_Characteristics, LVSCW_AUTOSIZE_USEHEADER);

}

LRESULT DumpSectionGui::OnNMCustomdraw(NMHDR* pnmh)
{
	LRESULT pResult = 0;
	unsigned int vectorIndex = 0;
	LPNMLVCUSTOMDRAW lpLVCustomDraw = (LPNMLVCUSTOMDRAW)(pnmh);

	switch(lpLVCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_ITEMPREPAINT:
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		{
			vectorIndex = (unsigned int)lpLVCustomDraw->nmcd.dwItemSpec;

			if (lpLVCustomDraw->iSubItem == COL_VSize)
			{
				if (sectionList[vectorIndex].highlightVirtualSize())
				{
					lpLVCustomDraw->clrText = RGB(255,255,255); // white text
					lpLVCustomDraw->clrTextBk = RGB(255,0,0); // red background
				}
			}
			else 
			{
				lpLVCustomDraw->clrText = CLR_DEFAULT;
				lpLVCustomDraw->clrTextBk = CLR_DEFAULT;
			}
		}
		break;
	default:
		break;    
	}


	pResult |= CDRF_NOTIFYPOSTPAINT;
	pResult |= CDRF_NOTIFYITEMDRAW;
	pResult |= CDRF_NOTIFYSUBITEMDRAW;

	return pResult;
}

void DumpSectionGui::getAllSectionsFromFile()
{
	PIMAGE_DOS_HEADER pDos = 0;
	PIMAGE_SECTION_HEADER pSec = 0;
	PIMAGE_NT_HEADERS pNT = 0;
	DWORD size = sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS) + 200;
	DWORD correctSize = 0;
	CHAR sectionNameA[IMAGE_SIZEOF_SHORT_NAME + 1] = {0};
	PeSection peSection;

	if (sectionList.empty())
	{
		sectionList.reserve(3);
	}
	else
	{
		sectionList.clear();
	}

	BYTE * buffer = new BYTE[size];

	if (ProcessAccessHelp::readHeaderFromFile(buffer,size,fullpath))
	{
		pDos = (PIMAGE_DOS_HEADER)buffer;

		if (pDos->e_magic == IMAGE_DOS_SIGNATURE)
		{
			pNT = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDos + pDos->e_lfanew);
			if (pNT->Signature == IMAGE_NT_SIGNATURE)
			{
				correctSize = (pNT->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER)) + sizeof(IMAGE_NT_HEADERS) + pDos->e_lfanew + 50;

				if (size < correctSize)
				{
					size = correctSize;
					delete [] buffer;
					buffer = new BYTE[size];
					if (!ProcessAccessHelp::readHeaderFromFile(buffer,size,fullpath))
					{
						delete [] buffer;
						return;
					}

					pDos = (PIMAGE_DOS_HEADER)buffer;
					pNT = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDos + pDos->e_lfanew);
				}

				pSec = IMAGE_FIRST_SECTION(pNT);

				for (WORD i = 0; i < pNT->FileHeader.NumberOfSections; i++)
				{
					ZeroMemory(sectionNameA, sizeof(sectionNameA));
					memcpy(sectionNameA,pSec->Name,8);
					swprintf_s(peSection.name,L"%S",sectionNameA);


					peSection.virtualAddress = imageBase + pSec->VirtualAddress;
					peSection.virtualSize = pSec->Misc.VirtualSize;
					peSection.rawAddress = pSec->PointerToRawData;
					peSection.rawSize = pSec->SizeOfRawData;
					peSection.characteristics = pSec->Characteristics;
					peSection.isDumped = true;

					sectionList.push_back(peSection);

					pSec++;
				}

			}
		}
	}

	delete [] buffer;
}

void DumpSectionGui::updateEditedItem()
{
	if (selectedSection)
	{
		selectedSection->virtualSize = EditListControl.GetValue();

		displaySectionList(ListSectionSelect);
	}
}

void DumpSectionGui::updateCheckState()
{
	PeSection * pesection;

	for (size_t i = 0; i < sectionList.size(); i++)
	{
		pesection = (PeSection *)ListSectionSelect.GetItemData((int)i);
		pesection->isDumped = ListSectionSelect.GetCheckState((int)i) == TRUE;
	}
}

void DumpSectionGui::selectOrDeselectAll()
{
	BOOL checkState = ListSectionSelect.GetCheckState((int)0) ? FALSE : TRUE;

	for (size_t i = 0; i < sectionList.size(); i++)
	{
		ListSectionSelect.SetCheckState((int)i, checkState);
	}
}
