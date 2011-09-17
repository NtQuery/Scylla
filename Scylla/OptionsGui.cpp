#include "OptionsGui.h"

#include "ConfigurationHolder.h"

BOOL OptionsGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	EditSectionName.Attach(GetDlgItem(IDC_OPTIONS_SECTIONNAME));
	EditSectionName.LimitText(IMAGE_SIZEOF_SHORT_NAME);

	loadOptions();

	CenterWindow();

	return TRUE;
}

void OptionsGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
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
		getConfigOptionsFromDlg((*mapIter).second);
	}
}

void OptionsGui::loadOptions()
{
	std::map<Configuration, ConfigObject>::iterator mapIter;

	for (mapIter = ConfigurationHolder::getConfigList().begin() ; mapIter != ConfigurationHolder::getConfigList().end(); mapIter++)
	{
		displayConfigInDlg((*mapIter).second);
	}
}

void OptionsGui::setCheckBox( int nIDDlgItem, bool bValue )
{
	CButton Button(GetDlgItem(nIDDlgItem));
	Button.SetCheck(bValue ? BST_CHECKED : BST_UNCHECKED);
}

void OptionsGui::displayConfigInDlg( ConfigObject & config )
{
	switch (config.configType)
	{
	case String:
		{
			setEditControl(config.dialogItemValue, config.valueString);
		}
		break;
	case Boolean:
		{
			setCheckBox(config.dialogItemValue, config.isTrue());
		}
		break;
	case Decimal:
		{
#ifdef _WIN64
			swprintf_s(config.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%I64u"),config.valueNumeric);
#else
			swprintf_s(config.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%u"),config.valueNumeric);
#endif
			setEditControl(config.dialogItemValue, config.valueString);
		}
		break;
	case Hexadecimal:
		{
#ifdef _WIN64
			swprintf_s(config.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%016I64X"),config.valueNumeric);
#else
			swprintf_s(config.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%08X"),config.valueNumeric);
#endif
			setEditControl(config.dialogItemValue, config.valueString);
		}
		break;
	}
}

void OptionsGui::setEditControl( int nIDDlgItem, const WCHAR * valueString )
{
	CEdit Edit(GetDlgItem(nIDDlgItem));
	Edit.SetWindowText(valueString);
}

void OptionsGui::getConfigOptionsFromDlg( ConfigObject & config )
{
	switch (config.configType)
	{
	case String:
		{
			getEditControl(config.dialogItemValue, config.valueString);
		}
		break;
	case Boolean:
		{
			getCheckBox(config.dialogItemValue, &config.valueNumeric);
		}
		break;
	case Decimal:
		{
			getEditControlNumeric(config.dialogItemValue, &config.valueNumeric, 10);
		}
		break;
	case Hexadecimal:
		{
			getEditControlNumeric(config.dialogItemValue, &config.valueNumeric, 16);
		}
		break;
	}
}

bool OptionsGui::getEditControl( int nIDDlgItem, WCHAR * valueString )
{
	CEdit Edit(GetDlgItem(nIDDlgItem));
	return (Edit.GetWindowText(valueString, CONFIG_OPTIONS_STRING_LENGTH) > 0);
}

void OptionsGui::getCheckBox( int nIDDlgItem, DWORD_PTR * valueNumeric )
{
	CButton Button(GetDlgItem(nIDDlgItem));
	switch (Button.GetCheck())
	{
	case BST_CHECKED:
		*valueNumeric = 1;
		return;
	case BST_UNCHECKED:
		*valueNumeric = 0;
		return;
	default:
		*valueNumeric = 0;
	}
}

void OptionsGui::getEditControlNumeric( int nIDDlgItem, DWORD_PTR * valueNumeric, int nBase )
{
	WCHAR temp[CONFIG_OPTIONS_STRING_LENGTH] = {0};

	if (getEditControl(nIDDlgItem, temp))
	{
#ifdef _WIN64
		*valueNumeric = _wcstoui64(temp, NULL, nBase);
#else
		*valueNumeric = wcstoul(temp, NULL, nBase);
#endif
	}
	else
	{
		*valueNumeric = 0;
	}
}
