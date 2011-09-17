#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlmisc.h>       // WTL utility classes like CString
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls

class DisassemblerGui : public CDialogImpl<DisassemblerGui>
{
public:
	enum { IDD = IDD_DLG_DISASSEMBLER };

	BEGIN_MSG_MAP(DisassemblerGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CONTEXTMENU(OnContextMenu)

		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

	DisassemblerGui(DWORD_PTR startAddress) : startAddress(startAddress) { }

protected:

	// Variables

	static const size_t DISASSEMBLER_GUI_MEMORY_SIZE = 0x100;

	WCHAR tempBuffer[100];
	DWORD_PTR startAddress;

	// Controls

	CListViewCtrl ListDisassembler;

	enum DisassemblerColumns {
		COL_ADDRESS,
		COL_INSTRUCTION_SIZE,
		COL_OPCODES,
		COL_INSTRUCTION
	};

protected:

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnContextMenu(CWindow wnd, CPoint point);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI functions

	void addColumnsToDisassembler(CListViewCtrl& list);
	void displayDisassembly(CListViewCtrl& list);

	// Popup menu functions

	CMenuHandle getCorrectSubMenu(int menuItem, int subMenuItem);

	// Misc

	void copyToClipboard(const WCHAR * text);
};
