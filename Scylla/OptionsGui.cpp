#include "OptionsGui.h"
#include "ConfigurationHolder.h"

HWND OptionsGui::hWndDlg = 0;

INT_PTR OptionsGui::initOptionsDialog(HINSTANCE hInstance, HWND hWndParent)
{
	return DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_OPTIONS),hWndParent, (DLGPROC)optionsDlgProc);
}

LRESULT CALLBACK OptionsGui::optionsDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hWndDlg = hWnd;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		Edit_LimitText(GetDlgItem(hWndDlg,IDC_OPTIONS_SECTIONNAME), IMAGE_SIZEOF_SHORT_NAME);
		loadOptions();
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BTN_OPTIONS_OK:
			{
				saveOptions();
				ConfigurationHolder::saveConfiguration();
				EndDialog(hWnd, 0);
			}
			return TRUE;
		case IDC_BTN_OPTIONS_CANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		case IDCANCEL:
			EndDialog(hWnd, 0);
			return TRUE;
		}
	}
	return FALSE;
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
	if (bValue)
	{
		Button_SetCheck(GetDlgItem(hWndDlg, nIDDlgItem),BST_CHECKED);
	}
	else
	{
		Button_SetCheck(GetDlgItem(hWndDlg, nIDDlgItem),BST_UNCHECKED);
	}
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
	SetDlgItemText(hWndDlg,nIDDlgItem,valueString);
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
	if (GetDlgItemText(hWndDlg, nIDDlgItem, valueString, CONFIG_OPTIONS_STRING_LENGTH))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void OptionsGui::getCheckBox( int nIDDlgItem, DWORD_PTR * valueNumeric )
{
	switch (Button_GetCheck(GetDlgItem(hWndDlg, nIDDlgItem)))
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
