#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlframe.h>      // WTL window frame helpers
#include <atlmisc.h>       // WTL utility classes like CString
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls
#include <atlddx.h>        // WTL dialog data exchange
#include <vector>
#include "hexedit.h"

#include "ApiReader.h"

enum DisassemblerAddressType {
	ADDRESS_TYPE_MODULE,
	ADDRESS_TYPE_API,
	ADDRESS_TYPE_SPECIAL
};

class DisassemblerAddressComment
{
public:
	DWORD_PTR address;
	WCHAR comment[MAX_PATH];
	DisassemblerAddressType type;
	DWORD moduleSize;

	bool operator<(const DisassemblerAddressComment& rhs) { return address < rhs.address; }
};

class DisassemblerGui : public CDialogImpl<DisassemblerGui>, public CWinDataExchange<DisassemblerGui>, public CDialogResize<DisassemblerGui>
{
public:
	enum { IDD = IDD_DLG_DISASSEMBLER };

	BEGIN_DDX_MAP(DisassemblerGui)
		DDX_CONTROL_HANDLE(IDC_LIST_DISASSEMBLER, ListDisassembler)
		DDX_CONTROL(IDC_EDIT_ADDRESS_DISASSEMBLE, EditAddress)
	END_DDX_MAP()

	BEGIN_MSG_MAP(DisassemblerGui)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_CONTEXTMENU(OnContextMenu)

		NOTIFY_HANDLER_EX(IDC_LIST_DISASSEMBLER, NM_CUSTOMDRAW, OnNMCustomdraw)

		COMMAND_ID_HANDLER_EX(IDC_BUTTON_DISASSEMBLE, OnDisassemble)
		COMMAND_ID_HANDLER_EX(IDC_BUTTON_DISASSEMBLER_BACK, OnDisassembleBack)
		COMMAND_ID_HANDLER_EX(IDC_BUTTON_DISASSEMBLER_FORWARD, OnDisassembleForward)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnExit)
		COMMAND_ID_HANDLER_EX(IDOK, OnExit)

		CHAIN_MSG_MAP(CDialogResize<DisassemblerGui>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(DisassemblerGui)
		DLGRESIZE_CONTROL(IDC_LIST_DISASSEMBLER,     DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_BUTTON_DISASSEMBLE,     DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BUTTON_DISASSEMBLER_BACK,     DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BUTTON_DISASSEMBLER_FORWARD,     DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EDIT_ADDRESS_DISASSEMBLE,   DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_ADDRESS_DISASSEMBLE,   DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	DisassemblerGui(DWORD_PTR startAddress, ApiReader * apiReaderObject);

protected:

	// Variables

	static const size_t DISASSEMBLER_GUI_MEMORY_SIZE = 0x120;

	WCHAR tempBuffer[500];
	int addressHistoryIndex;

	std::vector<DWORD_PTR> addressHistory;

	std::vector<DisassemblerAddressComment> addressCommentList;

	// Controls

	CListViewCtrl ListDisassembler;
	CHexEdit<DWORD_PTR> EditAddress;

	enum DisassemblerColumns {
		COL_ADDRESS = 0,
		COL_INSTRUCTION_SIZE,
		COL_OPCODES,
		COL_INSTRUCTION,
		COL_COMMENT
	};

	// Handles

	CMenu hMenuDisassembler;

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);
	void OnContextMenu(CWindow wnd, CPoint point);
	void OnExit(UINT uNotifyCode, int nID, CWindow wndCtl);
	LRESULT OnNMCustomdraw(NMHDR* pnmh);
	void OnDisassemble(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDisassembleBack(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnDisassembleForward(UINT uNotifyCode, int nID, CWindow wndCtl);
	// GUI functions

	void addColumnsToDisassembler(CListViewCtrl& list);
	bool displayDisassembly();

	// Misc

	void copyToClipboard(const WCHAR * text);

private:
	ApiReader * apiReader;
	BYTE data[DISASSEMBLER_GUI_MEMORY_SIZE];

	void toUpperCase(WCHAR * lowercase);
	void doColorInstruction( LPNMLVCUSTOMDRAW lpLVCustomDraw, DWORD_PTR itemIndex );
	void followInstruction(int index);
	bool getDisassemblyComment(unsigned int index);

	void disassembleNewAddress(DWORD_PTR address);
	void initAddressCommentList();
	void addModuleAddressCommentEntry( DWORD_PTR address, DWORD moduleSize, const WCHAR * modulePath );
	void analyzeAddress( DWORD_PTR address, WCHAR * comment );
};
