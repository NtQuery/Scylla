#include "OptionsGui.h"
#include "Scylla.h"

BOOL OptionsGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	loadOptions();
	DoDataExchange(DDX_LOAD); // show settings

	EditSectionName.LimitText(IMAGE_SIZEOF_SHORT_NAME);

	CenterWindow();

	return TRUE;
}

void OptionsGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE);
	saveOptions();
	Scylla::config.saveConfiguration();

	EndDialog(0);
}

void OptionsGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void OptionsGui::saveOptions() const
{
	Scylla::config[USE_PE_HEADER_FROM_DISK].setBool(usePEHeaderFromDisk);
	Scylla::config[DEBUG_PRIVILEGE].setBool(debugPrivilege);
	Scylla::config[CREATE_BACKUP].setBool(createBackup);
	Scylla::config[DLL_INJECTION_AUTO_UNLOAD].setBool(dllInjectionAutoUnload);
	Scylla::config[UPDATE_HEADER_CHECKSUM].setBool(updateHeaderChecksum);
	Scylla::config[IAT_SECTION_NAME].setString(iatSectionName);
	Scylla::config[REMOVE_DOS_HEADER_STUB].setBool(removeDosHeaderStub);
	Scylla::config[IAT_FIX_AND_OEP_FIX].setBool(fixIatAndOep);
	Scylla::config[SUSPEND_PROCESS_FOR_DUMPING].setBool(suspendProcessForDumping);
	Scylla::config[OriginalFirstThunk_SUPPORT].setBool(oftSupport);
	Scylla::config[USE_ADVANCED_IAT_SEARCH].setBool(useAdvancedIatSearch);
	Scylla::config[SCAN_AND_FIX_DIRECT_IMPORTS].setBool(scanAndFixDirectImports);
	Scylla::config[CREATE_NEW_IAT_IN_SECTION].setBool(createNewIatInSection);
}

void OptionsGui::loadOptions()
{
	usePEHeaderFromDisk    = Scylla::config[USE_PE_HEADER_FROM_DISK].getBool();
	debugPrivilege         = Scylla::config[DEBUG_PRIVILEGE].getBool();
	createBackup           = Scylla::config[CREATE_BACKUP].getBool();
	dllInjectionAutoUnload = Scylla::config[DLL_INJECTION_AUTO_UNLOAD].getBool();
	updateHeaderChecksum   = Scylla::config[UPDATE_HEADER_CHECKSUM].getBool();
	wcsncpy_s(iatSectionName, Scylla::config[IAT_SECTION_NAME].getString(), _countof(iatSectionName)-1);
	iatSectionName[_countof(iatSectionName) - 1] = L'\0';

	removeDosHeaderStub = Scylla::config[REMOVE_DOS_HEADER_STUB].getBool();
	fixIatAndOep = Scylla::config[IAT_FIX_AND_OEP_FIX].getBool();
	suspendProcessForDumping = Scylla::config[SUSPEND_PROCESS_FOR_DUMPING].getBool();
	oftSupport = Scylla::config[OriginalFirstThunk_SUPPORT].getBool();
	useAdvancedIatSearch = Scylla::config[USE_ADVANCED_IAT_SEARCH].getBool();
	scanAndFixDirectImports = Scylla::config[SCAN_AND_FIX_DIRECT_IMPORTS].getBool();
	createNewIatInSection = Scylla::config[CREATE_NEW_IAT_IN_SECTION].getBool();
}
