#pragma once

#include <windows.h>
#include "resource.h"

// WTL
#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include <atlwin.h>        // ATL GUI classes
#include <atlcrack.h>      // WTL enhanced msg map macros
#include <atlctrls.h>      // WTL controls
#include <atlddx.h>        // WTL dialog data exchange

class OptionsGui : public CDialogImpl<OptionsGui>, public CWinDataExchange<OptionsGui>
{
public:
	enum { IDD = IDD_DLG_OPTIONS };

	BEGIN_DDX_MAP(OptionsGui)
		DDX_CONTROL_HANDLE(IDC_OPTIONS_SECTIONNAME, EditSectionName)
		DDX_TEXT(IDC_OPTIONS_SECTIONNAME, iatSectionName)
		DDX_CHECK(IDC_CHECK_HEADER_CHECKSUM, updateHeaderChecksum)
		DDX_CHECK(IDC_CHECK_CREATE_BACKUP, createBackup)
		DDX_CHECK(IDC_CHECK_UNLOAD_DLL, dllInjectionAutoUnload)
		DDX_CHECK(IDC_CHECK_PE_HEADER_FROM_DISK, usePEHeaderFromDisk)
		DDX_CHECK(IDC_CHECK_DEBUG_PRIVILEGES, debugPrivilege)
		DDX_CHECK(IDC_CHECK_REMOVE_DOS_STUB, removeDosHeaderStub)
		DDX_CHECK(IDC_CHECK_FIX_IAT_AND_OEP, fixIatAndOep)
		DDX_CHECK(IDC_CHECK_SUSPEND_PROCESS, suspendProcessForDumping)
		DDX_CHECK(IDC_CHECKOFTSUPPORT, oftSupport)
		DDX_CHECK(IDC_CHECK_USEADVANCEDIATSEARCH, useAdvancedIatSearch)
		DDX_CHECK(IDC_SCANFIXDIRECTIMPORT, scanAndFixDirectImports)
		DDX_CHECK(IDC_NEWIATINSECTION, createNewIatInSection)
	END_DDX_MAP()

	BEGIN_MSG_MAP(OptionsGui)
		MSG_WM_INITDIALOG(OnInitDialog)

		COMMAND_ID_HANDLER_EX(IDC_BTN_OPTIONS_OK, OnOK)
		COMMAND_ID_HANDLER_EX(IDC_BTN_OPTIONS_CANCEL, OnCancel)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	END_MSG_MAP()

protected:

	// Settings (filled by DDX)

	WCHAR iatSectionName[IMAGE_SIZEOF_SHORT_NAME+1];
	bool updateHeaderChecksum;
	bool createBackup;
	bool dllInjectionAutoUnload;
	bool usePEHeaderFromDisk;
	bool debugPrivilege;
	bool removeDosHeaderStub;
	bool fixIatAndOep;
	bool suspendProcessForDumping;
	bool oftSupport;
	bool useAdvancedIatSearch;
	bool scanAndFixDirectImports;
	bool createNewIatInSection;
	// Controls

	CEdit EditSectionName;

	// Message handlers

	BOOL OnInitDialog(CWindow wndFocus, LPARAM lInitParam);

	void OnOK(UINT uNotifyCode, int nID, CWindow wndCtl);
	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl);

	// Gui helpers

	void saveOptions() const;
	void loadOptions();
};
