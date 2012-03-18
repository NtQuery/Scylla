#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlframe.h>      // WTL window frame helpers
#include <atlmisc.h>       // WTL utility classes
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls
#include <atlddx.h>        // WTL dialog data exchange

#include <vector>
#include "hexedit.h"
#include "DeviceNameResolver.h"

class Memory
{
public:
	DWORD_PTR address;
	DWORD size;
	WCHAR filename[MAX_PATH];
	WCHAR mappedFilename[MAX_PATH];
	WCHAR peSection[IMAGE_SIZEOF_SHORT_NAME *4];
	DWORD  state;
	DWORD  protect;
	DWORD  type;
};

class DumpMemoryGui : public CDialogImpl<DumpMemoryGui>, public CWinDataExchange<DumpMemoryGui>, public CDialogResize<DumpMemoryGui>
{
public:
	enum { IDD = IDD_DLG_DUMPMEMORY };

	BEGIN_DDX_MAP(DumpMemoryGui)
		DDX_CONTROL_HANDLE(IDC_LIST_DUMPMEMORY, ListMemorySelect)
		DDX_CONTROL(IDC_EDIT_DUMPADDRESS, EditMemoryAddress)
		DDX_CONTROL(IDC_EDIT_DUMPSIZE, EditMemorySize)
		DDX_CHECK(IDC_CHECK_FORCEDUMP, forceDump)
	END_DDX_MAP()

	BEGIN_MSG_MAP(DumpMemoryGui)
		MSG_WM_INITDIALOG(OnInitDialog)

		NOTIFY_HANDLER_EX(IDC_LIST_DUMPMEMORY, LVN_COLUMNCLICK, OnListMemoryColumnClicked)
		NOTIFY_HANDLER_EX(IDC_LIST_DUMPMEMORY, NM_CLICK, OnListMemoryClick)

		COMMAND_ID_HANDLER_EX(IDC_BTN_DUMPMEMORY_OK, OnOK)
		COMMAND_ID_HANDLER_EX(IDC_BTN_DUMPMEMORY_CANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)

		CHAIN_MSG_MAP(CDialogResize<DumpMemoryGui>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(DumpMemoryGui)
		DLGRESIZE_CONTROL(IDC_LIST_DUMPMEMORY,     DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_DUMPMEMORY_OK,     DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_BTN_DUMPMEMORY_CANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EDIT_DUMPADDRESS,   DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_EDIT_DUMPSIZE,      DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_SIZE,   DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_STATIC_ADDRESS,      DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_CHECK_FORCEDUMP,      DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	DumpMemoryGui();
	~DumpMemoryGui();

	BYTE * dumpedMemory;
	DWORD dumpedMemorySize;
	WCHAR dumpFilename[39];

protected:
	CListViewCtrl ListMemorySelect;
	CHexEdit<DWORD_PTR> EditMemoryAddress;
	CHexEdit<DWORD> EditMemorySize;

	std::vector<Memory> memoryList;
	Memory * selectedMemory;

	DeviceNameResolver * deviceNameResolver;

	enum ListColumns {
		COL_ADDRESS = 0,
		COL_SIZE,
		COL_FILENAME,
		COL_PESECTION,
		COL_TYPE,
		COL_PROTECTION,
		COL_STATE,
		COL_MAPPED_FILE
	};

	int prevColumn;
	bool ascending;

	bool forceDump;

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);

	LRESULT OnListMemoryColumnClicked(NMHDR* pnmh);
	LRESULT OnListMemoryClick(NMHDR* pnmh);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI functions

	void addColumnsToMemoryList(CListViewCtrl& list);
	void displayMemoryList(CListViewCtrl& list);

	static int CALLBACK listviewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

private:

	enum enumMemoryStateValues {
		STATE_COMMIT = 0,
		STATE_FREE,
		STATE_RESERVE
	};

	//"IMAGE",L"MAPPED",L"PRIVATE"
	enum enumMemoryTypeValues {
		TYPE_IMAGE = 0,
		TYPE_MAPPED,
		TYPE_PRIVATE
	};

	//"EXECUTE",L"EXECUTE_READ",L"EXECUTE_READWRITE",L"EXECUTE_WRITECOPY",L"NOACCESS",L"READONLY",L"READWRITE",L"WRITECOPY",L"GUARD",L"NOCACHE",L"WRITECOMBINE"
	enum enumMemoryProtectionValues {
		PROT_EXECUTE = 0,
		PROT_EXECUTE_READ,
		PROT_EXECUTE_READWRITE,
		PROT_EXECUTE_WRITECOPY,
		PROT_NOACCESS,
		PROT_READONLY,
		PROT_READWRITE,
		PROT_WRITECOPY,
		PROT_GUARD,
		PROT_NOCACHE,
		PROT_WRITECOMBINE
	};

	static const WCHAR * MemoryStateValues[];
	static const WCHAR * MemoryTypeValues[];
	static const WCHAR * MemoryProtectionValues[];
	static const WCHAR MemoryUnknown[];
	static const WCHAR MemoryUndefined[];

	static WCHAR protectionString[100];

	static const WCHAR * getMemoryTypeString(DWORD value);
	static const WCHAR * getMemoryStateString(DWORD value);
	static WCHAR * getMemoryProtectionString(DWORD value);

	void updateAddressAndSize( Memory * selectedMemory );
	void getMemoryList();
	SIZE_T getSizeOfImage(DWORD_PTR moduleBase);
	void setModuleName(DWORD_PTR moduleBase, const WCHAR * moduleName);
	void setAllSectionNames( DWORD_PTR moduleBase, WCHAR * moduleName );

	void setSectionName(DWORD_PTR sectionAddress, DWORD sectionSize, const WCHAR * sectionName);
	bool dumpMemory();
	bool getMappedFilename( Memory* memory );
};
