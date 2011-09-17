#include "DisassemblerGui.h"

#include "ProcessAccessHelp.h"

BOOL DisassemblerGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	ListDisassembler.Attach(GetDlgItem(IDC_LIST_DISASSEMBLER));

	addColumnsToDisassembler(ListDisassembler);
	displayDisassembly(ListDisassembler);

	return TRUE;
}

void DisassemblerGui::OnContextMenu(CWindow wnd, CPoint point)
{
	if (wnd.GetDlgCtrlID() == IDC_LIST_DISASSEMBLER)
	{
		CMenuHandle hmenuTrackPopup = getCorrectSubMenu(IDR_MENU_DISASSEMBLER, 0);

		BOOL menuItem = hmenuTrackPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, wnd);
		hmenuTrackPopup.DestroyMenu();

		if (menuItem)
		{
			int selection = ListDisassembler.GetSelectionMark();
			if (selection != -1) //valid selection?
			{
				int column = -1;
				switch (menuItem)
				{
				case ID__DIS_ADDRESS:
					column = COL_ADDRESS;
					break;
				case ID__DIS_SIZE:
					column = COL_INSTRUCTION_SIZE;
					break;
				case ID__DIS_OPCODES:
					column = COL_OPCODES;
					break;
				case ID__DIS_INSTRUCTIONS:
					column = COL_INSTRUCTION;
					break;
				}
				if(column != -1)
				{
					tempBuffer[0] = '\0';
					ListDisassembler.GetItemText(selection, column, tempBuffer, _countof(tempBuffer));
					copyToClipboard(tempBuffer);
				}
			}
		}
	}
}

void DisassemblerGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void DisassemblerGui::addColumnsToDisassembler(CListViewCtrl& list)
{
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	list.InsertColumn(COL_ADDRESS, L"Address", LVCFMT_LEFT, 105);
	list.InsertColumn(COL_INSTRUCTION_SIZE, L"Size", LVCFMT_CENTER, 40);
	list.InsertColumn(COL_OPCODES, L"OpCodes", LVCFMT_LEFT, 130);
	list.InsertColumn(COL_INSTRUCTION, L"Instructions", LVCFMT_LEFT, 200);
}

void DisassemblerGui::displayDisassembly(CListViewCtrl& list)
{
	BYTE data[DISASSEMBLER_GUI_MEMORY_SIZE];

	list.DeleteAllItems();

	if(!ProcessAccessHelp::readMemoryFromProcess(startAddress, sizeof(data), data))
		return;

	ProcessAccessHelp::disassembleMemory(data, sizeof(data), startAddress);

	for (unsigned int i = 0; i < ProcessAccessHelp::decodedInstructionsCount; i++)
	{

#ifdef _WIN64
		swprintf_s(tempBuffer, _countof(tempBuffer),L"%016I64X",ProcessAccessHelp::decodedInstructions[i].offset);
#else
		swprintf_s(tempBuffer, _countof(tempBuffer),L"%08X",ProcessAccessHelp::decodedInstructions[i].offset);
#endif

		list.InsertItem(i, tempBuffer);

		swprintf_s(tempBuffer, _countof(tempBuffer),L"%02d",ProcessAccessHelp::decodedInstructions[i].size);

		list.SetItemText(i, COL_INSTRUCTION_SIZE, tempBuffer);

		swprintf_s(tempBuffer, _countof(tempBuffer),L"%-24S",(char *)ProcessAccessHelp::decodedInstructions[i].instructionHex.p);

		list.SetItemText(i, COL_OPCODES, tempBuffer);

		swprintf_s(tempBuffer, _countof(tempBuffer),L"%S%S%S",(char*)ProcessAccessHelp::decodedInstructions[i].mnemonic.p, ProcessAccessHelp::decodedInstructions[i].operands.length != 0 ? " " : "", (char*)ProcessAccessHelp::decodedInstructions[i].operands.p);

		list.SetItemText(i, COL_INSTRUCTION, tempBuffer);
	}
}

void DisassemblerGui::copyToClipboard(const WCHAR * text)
{
	if(OpenClipboard())
	{
		EmptyClipboard();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wcslen(text)+1)*sizeof(WCHAR));
		if(hMem)
		{
			wcscpy((WCHAR *)GlobalLock(hMem), text);
			GlobalUnlock(hMem);
			if(!SetClipboardData(CF_UNICODETEXT, hMem))
			{
				GlobalFree(hMem);
			}
		}
		CloseClipboard();
	}
}

CMenuHandle DisassemblerGui::getCorrectSubMenu(int menuItem, int subMenuItem)
{
	CMenuHandle hmenu; // top-level menu 

	// Load the menu resource. 
	if (!hmenu.LoadMenu(menuItem)) 
		return NULL; 

	return hmenu.GetSubMenu(subMenuItem);
}
