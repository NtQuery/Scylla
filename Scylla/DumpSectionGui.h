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


class PeSection
{
public:
	WCHAR name[IMAGE_SIZEOF_SHORT_NAME + 1];
	DWORD_PTR virtualAddress;
	DWORD  virtualSize;
	DWORD_PTR  rawAddress;
	DWORD  rawSize;
	DWORD characteristics;

	bool highlightVirtualSize();
};

class DumpSectionGui : public CDialogImpl<DumpSectionGui>, public CWinDataExchange<DumpSectionGui>, public CDialogResize<DumpSectionGui>
{
	public:
		enum { IDD = IDD_DLG_DUMPSECTION };

		BEGIN_DDX_MAP(DumpSectionGui)
			DDX_CONTROL_HANDLE(IDC_LIST_DUMPSECTION, ListSectionSelect)
			DDX_CONTROL(IDC_EDIT_LISTCONTROL, EditListControl)
		END_DDX_MAP()

		BEGIN_MSG_MAP(DumpSectionGui)
			MSG_WM_INITDIALOG(OnInitDialog)

			NOTIFY_HANDLER_EX(IDC_LIST_DUMPSECTION, LVN_COLUMNCLICK, OnListSectionColumnClicked)
			NOTIFY_HANDLER_EX(IDC_LIST_DUMPSECTION, NM_CLICK, OnListSectionClick)
			NOTIFY_HANDLER_EX(IDC_LIST_DUMPSECTION, NM_CUSTOMDRAW, OnNMCustomdraw)
			NOTIFY_HANDLER_EX(IDC_LIST_DUMPSECTION, NM_DBLCLK, OnListDoubleClick)

			COMMAND_ID_HANDLER_EX(IDC_BUTTON_SELECT_DESELECT, OnSectionSelectAll)
			COMMAND_ID_HANDLER_EX(IDC_BTN_DUMPSECTION_OK, OnOK)
			COMMAND_ID_HANDLER_EX(IDC_BTN_DUMPSECTION_CANCEL, OnCancel)
			COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)

			CHAIN_MSG_MAP(CDialogResize<DumpSectionGui>)
		END_MSG_MAP()

		BEGIN_DLGRESIZE_MAP(DumpSectionGui)
			DLGRESIZE_CONTROL(IDC_LIST_DUMPSECTION,     DLSZ_SIZE_X | DLSZ_SIZE_Y)
			DLGRESIZE_CONTROL(IDC_BTN_DUMPSECTION_OK,     DLSZ_MOVE_X | DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_BTN_DUMPSECTION_CANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
			DLGRESIZE_CONTROL(IDC_BUTTON_SELECT_DESELECT,   DLSZ_MOVE_Y)
		END_DLGRESIZE_MAP()

		DumpSectionGui()
		{
			imageBase = 0;
			sizeOfImage = 0;
			fullpath[0] = 0;
		}
		//~DumpSectionGui();

		DWORD_PTR imageBase;  //VA
		DWORD sizeOfImage;
		WCHAR fullpath[MAX_PATH];
private:
	CListViewCtrl ListSectionSelect;
	CHexEdit<DWORD> EditListControl;

	std::vector<PeSection> sectionList;

	PeSection *selectedSection;

	enum ListColumns {
		COL_NAME = 0,
		COL_VA,
		COL_VSize,
		COL_RVA,
		COL_RSize,
		COL_Characteristics
	};

	int prevColumn;
	bool ascending;

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);

	LRESULT OnListSectionColumnClicked(NMHDR* pnmh);
	LRESULT OnListSectionClick(NMHDR* pnmh);
	LRESULT OnNMCustomdraw(NMHDR* pnmh);
	LRESULT OnListDoubleClick(NMHDR* pnmh);

	void OnSectionSelectAll(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	// GUI functions

	void addColumnsToSectionList(CListViewCtrl& list);
	void displaySectionList(CListViewCtrl& list);

	static int CALLBACK listviewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	WCHAR * getCharacteristicsString( DWORD characteristics );
	void getAllSectionsFromFile();
};