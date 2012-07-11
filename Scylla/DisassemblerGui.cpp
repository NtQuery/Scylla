#include "DisassemblerGui.h"
#include <algorithm>
#include "ProcessAccessHelp.h"
#include "Architecture.h"
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

DisassemblerGui::DisassemblerGui(DWORD_PTR startAddress, ApiReader * apiReaderObject)
{
	apiReader = apiReaderObject;
	addressHistoryIndex = 0;
	addressHistory.push_back(startAddress);
	hMenuDisassembler.LoadMenu(IDR_MENU_DISASSEMBLER);

	initAddressCommentList();
}

BOOL DisassemblerGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange(); // attach controls
	DlgResize_Init(true, true);

	addColumnsToDisassembler(ListDisassembler);
	displayDisassembly();

	EditAddress.SetValue(addressHistory[addressHistoryIndex]);

	CenterWindow();

	return TRUE;
}

void DisassemblerGui::OnContextMenu(CWindow wnd, CPoint point)
{
	if (wnd.GetDlgCtrlID() == IDC_LIST_DISASSEMBLER)
	{
		int selection = ListDisassembler.GetSelectionMark();
		if(selection == -1) // no item selected
			return;

		if(point.x == -1 && point.y == -1) // invoked by keyboard
		{
			ListDisassembler.EnsureVisible(selection, TRUE);
			ListDisassembler.GetItemPosition(selection, &point);
			ListDisassembler.ClientToScreen(&point);
		}

		CMenuHandle hSub = hMenuDisassembler.GetSubMenu(0);
		BOOL menuItem = hSub.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, wnd);
		if (menuItem)
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
			case ID__DIS_FOLLOW:
				followInstruction(selection);
				break;
			case ID__DIS_DISASSEMBLEHERE:
				{
					disassembleNewAddress((DWORD_PTR)ProcessAccessHelp::decomposerResult[selection].addr);
				}
				
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

LRESULT DisassemblerGui::OnNMCustomdraw(NMHDR* pnmh)
{
	LRESULT pResult = 0;
	LPNMLVCUSTOMDRAW lpLVCustomDraw = (LPNMLVCUSTOMDRAW)(pnmh);
	DWORD_PTR itemIndex = 0;

	switch(lpLVCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_ITEMPREPAINT:
	case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		{
			itemIndex = lpLVCustomDraw->nmcd.dwItemSpec;

			if (lpLVCustomDraw->iSubItem == COL_INSTRUCTION)
			{
					doColorInstruction(lpLVCustomDraw, itemIndex);
			}
			else 
			{
				lpLVCustomDraw->clrText = CLR_DEFAULT;
				lpLVCustomDraw->clrTextBk = CLR_DEFAULT;
			}
		}
		break;
	}


	pResult |= CDRF_NOTIFYPOSTPAINT;
	pResult |= CDRF_NOTIFYITEMDRAW;
	pResult |= CDRF_NOTIFYSUBITEMDRAW;

	return pResult;
}

void DisassemblerGui::OnExit(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void DisassemblerGui::OnDisassemble(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DWORD_PTR address = EditAddress.GetValue();
	if (address)
	{
		disassembleNewAddress(address);
	}
}

void DisassemblerGui::disassembleNewAddress(DWORD_PTR address)
{
	if (addressHistory[addressHistory.size() - 1] != address)
	{
		addressHistory.push_back(address);
		addressHistoryIndex = (int)addressHistory.size() - 1;
		EditAddress.SetValue(addressHistory[addressHistoryIndex]);

		if (!displayDisassembly())
		{
			MessageBox(L"Cannot disassemble memory at this address",L"Error",MB_ICONERROR);
		}
	}

}

void DisassemblerGui::OnDisassembleForward(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (addressHistoryIndex != (addressHistory.size() - 1))
	{
		addressHistoryIndex++;
		EditAddress.SetValue(addressHistory[addressHistoryIndex]);
		if (!displayDisassembly())
		{
			MessageBox(L"Cannot disassemble memory at this address",L"Error",MB_ICONERROR);
		}
	}
}

void DisassemblerGui::OnDisassembleBack(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	if (addressHistoryIndex != 0)
	{
		addressHistoryIndex--;
		EditAddress.SetValue(addressHistory[addressHistoryIndex]);
		if (!displayDisassembly())
		{
			MessageBox(L"Cannot disassemble memory at this address",L"Error",MB_ICONERROR);
		}
	}
}

void DisassemblerGui::addColumnsToDisassembler(CListViewCtrl& list)
{
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	list.InsertColumn(COL_ADDRESS, L"Address", LVCFMT_LEFT);
	list.InsertColumn(COL_INSTRUCTION_SIZE, L"Size", LVCFMT_CENTER);
	list.InsertColumn(COL_OPCODES, L"Opcodes", LVCFMT_LEFT);
	list.InsertColumn(COL_INSTRUCTION, L"Instructions", LVCFMT_LEFT);
	list.InsertColumn(COL_COMMENT, L"Comment", LVCFMT_LEFT);
}

bool DisassemblerGui::displayDisassembly()
{
	ListDisassembler.DeleteAllItems();

	if(!ProcessAccessHelp::readMemoryFromProcess(addressHistory[addressHistoryIndex], sizeof(data), data))
		return false;

	if (!ProcessAccessHelp::decomposeMemory(data, sizeof(data), addressHistory[addressHistoryIndex]))
		return false;

	if (!ProcessAccessHelp::disassembleMemory(data, sizeof(data), addressHistory[addressHistoryIndex]))
		return false;

	for (unsigned int i = 0; i < ProcessAccessHelp::decodedInstructionsCount; i++)
	{
		swprintf_s(tempBuffer, PRINTF_DWORD_PTR_FULL,ProcessAccessHelp::decodedInstructions[i].offset);

		ListDisassembler.InsertItem(i, tempBuffer);

		swprintf_s(tempBuffer, L"%02d",ProcessAccessHelp::decodedInstructions[i].size);

		ListDisassembler.SetItemText(i, COL_INSTRUCTION_SIZE, tempBuffer);

		swprintf_s(tempBuffer, L"%S", (char *)ProcessAccessHelp::decodedInstructions[i].instructionHex.p);

		toUpperCase(tempBuffer);
		ListDisassembler.SetItemText(i, COL_OPCODES, tempBuffer);

		swprintf_s(tempBuffer, L"%S%S%S",(char*)ProcessAccessHelp::decodedInstructions[i].mnemonic.p, ProcessAccessHelp::decodedInstructions[i].operands.length != 0 ? " " : "", (char*)ProcessAccessHelp::decodedInstructions[i].operands.p);

		toUpperCase(tempBuffer);
		ListDisassembler.SetItemText(i, COL_INSTRUCTION, tempBuffer);

		tempBuffer[0] = 0;
		if (getDisassemblyComment(i))
		{
			ListDisassembler.SetItemText(i, COL_COMMENT, tempBuffer);
		}
	}

	ListDisassembler.SetColumnWidth(COL_ADDRESS, LVSCW_AUTOSIZE_USEHEADER);
	ListDisassembler.SetColumnWidth(COL_INSTRUCTION_SIZE, LVSCW_AUTOSIZE_USEHEADER);
	ListDisassembler.SetColumnWidth(COL_OPCODES, 140);
	ListDisassembler.SetColumnWidth(COL_INSTRUCTION, LVSCW_AUTOSIZE_USEHEADER);
	ListDisassembler.SetColumnWidth(COL_COMMENT, LVSCW_AUTOSIZE_USEHEADER);

	return true;
}

void DisassemblerGui::copyToClipboard(const WCHAR * text)
{
	if(OpenClipboard())
	{
		EmptyClipboard();
		size_t len = wcslen(text);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(WCHAR));
		if(hMem)
		{
			wcscpy_s(static_cast<WCHAR *>(GlobalLock(hMem)), len + 1, text);
			GlobalUnlock(hMem);
			if(!SetClipboardData(CF_UNICODETEXT, hMem))
			{
				GlobalFree(hMem);
			}
		}
		CloseClipboard();
	}
}

void DisassemblerGui::toUpperCase(WCHAR * lowercase)
{
	for (size_t i = 0; i < wcslen(lowercase); i++)
	{
		if (lowercase[i] != L'x')
		{
			lowercase[i] = towupper(lowercase[i]);
		}
	}
}

void DisassemblerGui::doColorInstruction( LPNMLVCUSTOMDRAW lpLVCustomDraw, DWORD_PTR itemIndex )
{
	if (ProcessAccessHelp::decomposerResult[itemIndex].flags == FLAG_NOT_DECODABLE)
	{
		lpLVCustomDraw->clrText = RGB(255,255,255); // white text
		lpLVCustomDraw->clrTextBk = RGB(255,0,0); // red background
	}
	else if (META_GET_FC(ProcessAccessHelp::decomposerResult[itemIndex].meta) == FC_RET)
	{
		lpLVCustomDraw->clrTextBk = RGB(0,255,255); // aqua
	}
	else if (META_GET_FC(ProcessAccessHelp::decomposerResult[itemIndex].meta) == FC_CALL)
	{
		lpLVCustomDraw->clrTextBk = RGB(255,255,0); // yellow
	}
	else if (META_GET_FC(ProcessAccessHelp::decomposerResult[itemIndex].meta) == FC_UNC_BRANCH)
	{
		lpLVCustomDraw->clrTextBk = RGB(0x32,0xCD,0x32); // limegreen
	}
	else if (META_GET_FC(ProcessAccessHelp::decomposerResult[itemIndex].meta) == FC_CND_BRANCH)
	{
		lpLVCustomDraw->clrTextBk = RGB(0xAD,0xFF,0x2F); // greenyellow
	}

}

void DisassemblerGui::followInstruction(int index)
{
	DWORD_PTR address = 0;
	DWORD_PTR addressTemp = 0;
	DWORD type = META_GET_FC(ProcessAccessHelp::decomposerResult[index].meta);

	if (ProcessAccessHelp::decomposerResult[index].flags != FLAG_NOT_DECODABLE)
	{
		if (type == FC_CALL || type == FC_UNC_BRANCH || type == FC_CND_BRANCH)
		{
#ifdef _WIN64
			if (ProcessAccessHelp::decomposerResult[index].flags & FLAG_RIP_RELATIVE)
			{
				addressTemp = INSTRUCTION_GET_RIP_TARGET(&ProcessAccessHelp::decomposerResult[index]);

				if(!ProcessAccessHelp::readMemoryFromProcess(addressTemp, sizeof(DWORD_PTR), &address))
				{
					address = 0;
				}
			}
#endif

			if (ProcessAccessHelp::decomposerResult[index].ops[0].type == O_PC)
			{
				address = (DWORD_PTR)INSTRUCTION_GET_TARGET(&ProcessAccessHelp::decomposerResult[index]);
			}

			if (ProcessAccessHelp::decomposerResult[index].ops[0].type == O_DISP)
			{
				addressTemp = (DWORD_PTR)ProcessAccessHelp::decomposerResult[index].disp;

				if(!ProcessAccessHelp::readMemoryFromProcess(addressTemp, sizeof(DWORD_PTR), &address))
				{
					address = 0;
				}
			}

			if (address != 0)
			{
				disassembleNewAddress(address);
			}
		}

	}
}

bool DisassemblerGui::getDisassemblyComment(unsigned int index)
{
	DWORD_PTR address = 0;
	DWORD_PTR addressTemp = 0;
	DWORD type = META_GET_FC(ProcessAccessHelp::decomposerResult[index].meta);

	tempBuffer[0] = 0;

	if (ProcessAccessHelp::decomposerResult[index].flags != FLAG_NOT_DECODABLE)
	{
		if (type == FC_CALL || type == FC_UNC_BRANCH || type == FC_CND_BRANCH)
		{
			if (ProcessAccessHelp::decomposerResult[index].flags & FLAG_RIP_RELATIVE)
			{
				#ifdef _WIN64
				addressTemp = (DWORD_PTR)INSTRUCTION_GET_RIP_TARGET(&ProcessAccessHelp::decomposerResult[index]);

				swprintf_s(tempBuffer,L"-> "PRINTF_DWORD_PTR_FULL,addressTemp);

				if(ProcessAccessHelp::readMemoryFromProcess(addressTemp, sizeof(DWORD_PTR), &address))
				{
					swprintf_s(tempBuffer,L"%s -> "PRINTF_DWORD_PTR_FULL,tempBuffer,address);
				}
				#endif
			}
			else if (ProcessAccessHelp::decomposerResult[index].ops[0].type == O_PC)
			{
				address = (DWORD_PTR)INSTRUCTION_GET_TARGET(&ProcessAccessHelp::decomposerResult[index]);
				swprintf_s(tempBuffer,L"-> "PRINTF_DWORD_PTR_FULL,address);
			}
			else if (ProcessAccessHelp::decomposerResult[index].ops[0].type == O_DISP)
			{
				addressTemp = (DWORD_PTR)ProcessAccessHelp::decomposerResult[index].disp;

				swprintf_s(tempBuffer,L"-> "PRINTF_DWORD_PTR_FULL,addressTemp);

				address = 0;
				if(ProcessAccessHelp::readMemoryFromProcess(addressTemp, sizeof(DWORD_PTR), &address))
				{
					swprintf_s(tempBuffer,L"%s -> "PRINTF_DWORD_PTR_FULL,tempBuffer,address);
				}
			}
		}
	}

	if (address != 0)
	{
		analyzeAddress(address, tempBuffer);
		return true;
	}
	else
	{
		return false;
	}
}

void DisassemblerGui::initAddressCommentList()
{
	HMODULE * hMods = 0;
	HMODULE hModResult = 0;
	WCHAR target[MAX_PATH];

	DWORD numHandles = ProcessAccessHelp::getModuleHandlesFromProcess(ProcessAccessHelp::hProcess, &hMods);
	if (numHandles == 0)
	{
		return;
	}

	for (DWORD i = 0; i < numHandles; i++)
	{
		if (ProcessAccessHelp::targetImageBase != (DWORD_PTR)hMods[i])
		{
			if (GetModuleFileNameExW(ProcessAccessHelp::hProcess, hMods[i], target, _countof(target)))
			{
				addModuleAddressCommentEntry((DWORD_PTR)hMods[i], (DWORD)ProcessAccessHelp::getSizeOfImageProcess(ProcessAccessHelp::hProcess, (DWORD_PTR)hMods[i]), target);
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"DllInjection::getModuleHandle :: GetModuleFileNameExW failed 0x%X", GetLastError());
#endif
			}
		}
	}

	std::sort(addressCommentList.begin(), addressCommentList.end());
}

void DisassemblerGui::addModuleAddressCommentEntry( DWORD_PTR address, DWORD moduleSize, const WCHAR * modulePath )
{
	DisassemblerAddressComment commentObj;
	//get filename
	const WCHAR* slash = wcsrchr(modulePath, L'\\');
	if(slash)
	{
		modulePath = slash+1;
	}

	wcscpy_s(commentObj.comment, _countof(commentObj.comment), modulePath);
	commentObj.address = address;
	commentObj.type = ADDRESS_TYPE_MODULE;
	commentObj.moduleSize = moduleSize;

	addressCommentList.push_back(commentObj);
}

void DisassemblerGui::analyzeAddress( DWORD_PTR address, WCHAR * comment )
{
	if (addressCommentList[0].address > address) //list is sorted, TODO: binary search
	{
		return;
	}
	bool isSuspect;
	ApiInfo * api = apiReader->getApiByVirtualAddress(address, &isSuspect);

	if (api != 0 && api != (ApiInfo *)1)
	{
		if (api->name[0] == 0)
		{
			swprintf_s(tempBuffer,L"%s = %s.%04X", comment, api->module->getFilename(), api->ordinal);
		}
		else
		{
			swprintf_s(tempBuffer,L"%s = %s.%S", comment, api->module->getFilename(), api->name);
		}
	}
	else
	{
		for (size_t i = 0; i < addressCommentList.size(); i++) 
		{
			if (addressCommentList[i].type == ADDRESS_TYPE_MODULE)
			{
				if (address >= addressCommentList[i].address && address < (addressCommentList[i].address + addressCommentList[i].moduleSize))
				{
					swprintf_s(tempBuffer,L"%s = %s", comment, addressCommentList[i].comment);
					return;
				}
			}
		}
	}


}
