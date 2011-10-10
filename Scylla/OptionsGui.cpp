#include "OptionsGui.h"
#include "ConfigurationHolder.h"

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
	ConfigurationHolder::saveConfiguration();

	EndDialog(0);
}

void OptionsGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void OptionsGui::saveOptions()
{
	std::map<Configuration, ConfigObject>::iterator mapIter;

	for (mapIter = ConfigurationHolder::getConfigList().begin() ; mapIter != ConfigurationHolder::getConfigList().end(); mapIter++)
	{
		switch(mapIter->first)
		{
		case USE_PE_HEADER_FROM_DISK:
			usePEHeaderFromDisk ? mapIter->second.setTrue() : mapIter->second.setFalse();
			break;
		case DEBUG_PRIVILEGE:
			debugPrivilege ? mapIter->second.setTrue() : mapIter->second.setFalse();
			break;
		case CREATE_BACKUP:
			createBackup ? mapIter->second.setTrue() : mapIter->second.setFalse();
			break;
		case DLL_INJECTION_AUTO_UNLOAD:
			dllInjectionAutoUnload ? mapIter->second.setTrue() : mapIter->second.setFalse();
			break;
		case UPDATE_HEADER_CHECKSUM:
			updateHeaderChecksum ? mapIter->second.setTrue() : mapIter->second.setFalse();
			break;
		case IAT_SECTION_NAME:
			wcscpy_s(mapIter->second.valueString, _countof(mapIter->second.valueString), iatSectionName);
			break;
		}
	}
}

void OptionsGui::loadOptions()
{
	std::map<Configuration, ConfigObject>::iterator mapIter;

	for (mapIter = ConfigurationHolder::getConfigList().begin() ; mapIter != ConfigurationHolder::getConfigList().end(); mapIter++)
	{
		switch(mapIter->first)
		{
		case USE_PE_HEADER_FROM_DISK:
			usePEHeaderFromDisk = mapIter->second.isTrue();
			break;
		case DEBUG_PRIVILEGE:
			debugPrivilege = mapIter->second.isTrue();
			break;
		case CREATE_BACKUP:
			createBackup = mapIter->second.isTrue();
			break;
		case DLL_INJECTION_AUTO_UNLOAD:
			dllInjectionAutoUnload = mapIter->second.isTrue();
			break;
		case UPDATE_HEADER_CHECKSUM:
			updateHeaderChecksum = mapIter->second.isTrue();
			break;
		case IAT_SECTION_NAME:
			wcsncpy_s(iatSectionName, _countof(iatSectionName), mapIter->second.valueString, _countof(iatSectionName)-1);
			iatSectionName[_countof(iatSectionName)-1] = L'\0';
			break;
		}
	}
}
