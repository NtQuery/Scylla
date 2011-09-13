#include "DisassemblerGui.h"
#include "ProcessAccessHelp.h"

HWND DisassemblerGui::hWndDlg = 0;
HINSTANCE DisassemblerGui::hInstance = 0;
DWORD_PTR DisassemblerGui::startAddress = 0;
WCHAR DisassemblerGui::tempBuffer[100];

INT_PTR DisassemblerGui::initDialog(HINSTANCE hInst, HWND hWndParent, DWORD_PTR address)
{
	hInstance = hInst;
	startAddress = address;
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_DISASSEMBLER),hWndParent, (DLGPROC)disassemblerDlgProc);
}

LRESULT CALLBACK DisassemblerGui::disassemblerDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hWndDlg = hWnd;
	switch (uMsg)
	{
	case WM_INITDIALOG:
		addColumnsToDisassembler(GetDlgItem(hWnd, IDC_LIST_DISASSEMBLER));
		displayDisassembly();
		break;

	case WM_CONTEXTMENU:
		OnContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BTN_PICKDLL_OK:
			/*index = ListView_GetSelectionMark(GetDlgItem(hWnd, IDC_LIST_DLLSELECT));
			if (index != -1)
			{
				selectedModule = &(*moduleList).at(index);
				EndDialog(hWnd, 1);
			}*/

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

void DisassemblerGui::addColumnsToDisassembler( HWND list )
{
	if (list)
	{
		LVCOLUMN * lvc = (LVCOLUMN*)malloc(sizeof(LVCOLUMN));

		ListView_SetExtendedListViewStyleEx(list,LVS_EX_FULLROWSELECT,LVS_EX_FULLROWSELECT);

		lvc->mask = LVCF_TEXT | LVCF_WIDTH;
		lvc->fmt = LVCFMT_LEFT;
		lvc->cx = 105;
		lvc->pszText = L"Address";
		ListView_InsertColumn(list, COL_ADDRESS, lvc);

		lvc->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvc->fmt = LVCFMT_CENTER;
		lvc->cx = 40;
		lvc->pszText = L"Size";
		ListView_InsertColumn(list, COL_INSTRUCTION_SIZE, lvc);

		lvc->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvc->fmt = LVCFMT_LEFT;
		lvc->cx = 130;
		lvc->pszText = L"OpCodes";
		ListView_InsertColumn(list, COL_OPCODES, lvc);

		lvc->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
		lvc->fmt = LVCFMT_LEFT;
		lvc->cx = 200;
		lvc->pszText = L"Instructions";
		ListView_InsertColumn(list, COL_INSTRUCTION, lvc);


		free(lvc);
	}
}

void DisassemblerGui::displayDisassembly()
{
	LVITEM item;
	HWND hList = GetDlgItem(hWndDlg, IDC_LIST_DISASSEMBLER);

	ListView_DeleteAllItems(hList);

	item.mask = LVIF_TEXT;

	BYTE * data = new BYTE[DISASSEMBLER_GUI_MEMORY_SIZE];

	ProcessAccessHelp::readMemoryFromProcess(startAddress,DISASSEMBLER_GUI_MEMORY_SIZE, data);

	ProcessAccessHelp::disassembleMemory(data,DISASSEMBLER_GUI_MEMORY_SIZE, startAddress);

	for (unsigned int i = 0; i < ProcessAccessHelp::decodedInstructionsCount; i++)
	{

#ifdef _WIN64
		swprintf_s(tempBuffer, _countof(tempBuffer),L"%016I64X",ProcessAccessHelp::decodedInstructions[i].offset);
#else
		swprintf_s(tempBuffer, _countof(tempBuffer),L"%08X",ProcessAccessHelp::decodedInstructions[i].offset);
#endif

		item.iItem = i;
		item.iSubItem = COL_ADDRESS;
		item.pszText = tempBuffer;
		item.iItem = ListView_InsertItem(hList, &item);

		swprintf_s(tempBuffer, _countof(tempBuffer),L"%02d",ProcessAccessHelp::decodedInstructions[i].size);

		item.iSubItem = COL_INSTRUCTION_SIZE;
		item.pszText = tempBuffer;
		ListView_SetItem(hList, &item);

		swprintf_s(tempBuffer, _countof(tempBuffer),L"%-24S",(char *)ProcessAccessHelp::decodedInstructions[i].instructionHex.p);

		item.iSubItem = COL_OPCODES;
		item.pszText = tempBuffer;
		ListView_SetItem(hList, &item);

		swprintf_s(tempBuffer, _countof(tempBuffer),L"%S%S%S",(char*)ProcessAccessHelp::decodedInstructions[i].mnemonic.p, ProcessAccessHelp::decodedInstructions[i].operands.length != 0 ? " " : "", (char*)ProcessAccessHelp::decodedInstructions[i].operands.p);

		item.iSubItem = COL_INSTRUCTION;
		item.pszText = tempBuffer;
		ListView_SetItem(hList, &item);
	}

	delete [] data;
}

void DisassemblerGui::OnContextMenu(int x, int y) 
{ 
	HWND hwnd = 0;
	int selection;
	POINT pt = { x, y };        // location of mouse click
	HTREEITEM selectedTreeNode = 0;

	if ((hwnd = mouseInDialogItem(IDC_LIST_DISASSEMBLER, pt)) != NULL)
	{
		HMENU hmenuTrackPopup = getCorrectSubMenu(IDR_MENU_DISASSEMBLER, 0);

		BOOL menuItem = TrackPopupMenu(hmenuTrackPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, 0);

		if (menuItem)
		{
			selection = ListView_GetSelectionMark(hwnd);
			if (selection != -1) //valid selection?
			{
				switch (menuItem)
				{
				case ID__DIS_ADDRESS:
					getModuleListItem(COL_ADDRESS,selection,tempBuffer);
					copyToClipboard();
					break;
				case ID__DIS_SIZE:
					getModuleListItem(COL_INSTRUCTION_SIZE,selection,tempBuffer);
					copyToClipboard();
					break;
				case ID__DIS_OPCODES:
					getModuleListItem(COL_OPCODES,selection,tempBuffer);
					copyToClipboard();
					break;
				case ID__DIS_INSTRUCTIONS:
					getModuleListItem(COL_INSTRUCTION,selection,tempBuffer);
					copyToClipboard();
					break;
				}
			}
		}
	}

}

void DisassemblerGui::copyToClipboard()
{
	HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, sizeof(tempBuffer));
	LPVOID lock = GlobalLock(hMem);

	CopyMemory(lock,tempBuffer, sizeof(tempBuffer));

	if (!OpenClipboard(hWndDlg)) 
		return;

	EmptyClipboard();

	GlobalUnlock(hMem);

	SetClipboardData(CF_UNICODETEXT, hMem);
	CloseClipboard();

	GlobalFree(hMem);
}

void DisassemblerGui::getModuleListItem(int column, int iItem, WCHAR * buffer)
{
	LVITEM pitem = {0};
	pitem.iItem = iItem;
	pitem.iSubItem = column;
	pitem.mask = LVIF_TEXT;
	pitem.cchTextMax = 256;
	pitem.pszText = buffer;
	ListView_GetItem(GetDlgItem(hWndDlg, IDC_LIST_DISASSEMBLER),&pitem);
}


HMENU DisassemblerGui::getCorrectSubMenu(int menuItem, int subMenuItem)
{
	HMENU hmenu;            // top-level menu 
	HMENU hmenuTrackPopup;  // shortcut menu 
	// Load the menu resource. 
	if ((hmenu = LoadMenu(hInstance, MAKEINTRESOURCE(menuItem))) == NULL) 
		return 0; 

	hmenuTrackPopup = GetSubMenu(hmenu, subMenuItem);

	if (hmenuTrackPopup)
	{
		return hmenuTrackPopup;
	}
	else
	{
		return 0;
	}
}

HWND DisassemblerGui::mouseInDialogItem(int dlgItem, POINT pt)
{
	RECT rc;
	HWND hwnd = GetDlgItem(hWndDlg, dlgItem);
	if (hwnd)
	{
		// Get the bounding rectangle of the client area. 
		GetClientRect(hwnd, &rc); 

		// Convert the mouse position to client coordinates. 
		ScreenToClient(hwnd, &pt); 

		// If the position is in the client area, display a  
		// shortcut menu.
		if (PtInRect(&rc, pt))
		{
			return hwnd;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}
