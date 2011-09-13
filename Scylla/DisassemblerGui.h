#pragma once

#include "MainGui.h"

static const enum DisassemblerColumns {
	COL_ADDRESS,
	COL_INSTRUCTION_SIZE,
	COL_OPCODES,
	COL_INSTRUCTION
};

#define DISASSEMBLER_GUI_MEMORY_SIZE 0x100

class DisassemblerGui {
public:
	static WCHAR tempBuffer[100];
	static HWND hWndDlg;
	static HINSTANCE hInstance;
	static DWORD_PTR startAddress;
	
	static INT_PTR initDialog(HINSTANCE hInstance, HWND hWndParent, DWORD_PTR address);
	static void addColumnsToDisassembler( HWND list );
	static void displayDisassembly();
private:
	static LRESULT CALLBACK disassemblerDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static HWND mouseInDialogItem(int dlgItem, POINT pt);
	static void OnContextMenu(int x, int y);
	static HMENU getCorrectSubMenu(int menuItem, int subMenuItem);
	static void getModuleListItem(int column, int iItem, WCHAR * buffer);
	static void copyToClipboard();
};