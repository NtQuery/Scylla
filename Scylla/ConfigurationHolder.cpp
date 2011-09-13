
#include "ConfigurationHolder.h"
#include "resource.h"

WCHAR ConfigurationHolder::configPath[MAX_PATH];
ConfigurationInitializer ConfigurationHolder::config;

//#define DEBUG_COMMENTS

ConfigurationInitializer::ConfigurationInitializer()
{
	ConfigObject configObject;

	mapConfig[USE_PE_HEADER_FROM_DISK] = configObject.newValues(L"USE_PE_HEADER_FROM_DISK", Boolean, IDC_CHECK_PE_HEADER_FROM_DISK);
	mapConfig[DEBUG_PRIVILEGE] = configObject.newValues(L"DEBUG_PRIVILEGE", Boolean, IDC_CHECK_DEBUG_PRIVILEGES);
	mapConfig[CREATE_BACKUP] = configObject.newValues(L"CREATE_BACKUP", Boolean, IDC_CHECK_CREATE_BACKUP);
	mapConfig[DLL_INJECTION_AUTO_UNLOAD] = configObject.newValues(L"DLL_INJECTION_AUTO_UNLOAD", Boolean, IDC_CHECK_UNLOAD_DLL);
	mapConfig[UPDATE_HEADER_CHECKSUM] = configObject.newValues(L"UPDATE_HEADER_CHECKSUM", Boolean, IDC_CHECK_HEADER_CHECKSUM);

	mapConfig[IAT_SECTION_NAME] = configObject.newValues(L"IAT_SECTION_NAME", String, IDC_OPTIONS_SECTIONNAME);
}


bool ConfigurationHolder::loadConfiguration()
{
	std::map<Configuration, ConfigObject>::iterator mapIter;

	if (!buildConfigFilePath())
	{
		return false;
	}

	for (mapIter = config.mapConfig.begin() ; mapIter != config.mapConfig.end(); mapIter++)
	{
		if (!loadConfig((*mapIter).second))
		{
			return false;
		}
	}

	return true;
}

bool ConfigurationHolder::saveConfiguration()
{
	std::map<Configuration, ConfigObject>::iterator mapIter;

	if (!buildConfigFilePath())
	{
		return false;
	}

	for (mapIter = config.mapConfig.begin() ; mapIter != config.mapConfig.end(); mapIter++)
	{
		if (!saveConfig((*mapIter).second))
		{
			return false;
		}
	}

	return true;
}

bool ConfigurationHolder::saveNumericToConfigFile(ConfigObject & configObject, int nBase)
{

	if (nBase == 16)
	{
#ifdef _WIN64
		swprintf_s(configObject.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%016I64X"),configObject.valueNumeric);
#else
		swprintf_s(configObject.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%08X"),configObject.valueNumeric);
#endif
	}
	else
	{
#ifdef _WIN64
		swprintf_s(configObject.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%I64u"),configObject.valueNumeric);
#else
		swprintf_s(configObject.valueString, CONFIG_OPTIONS_STRING_LENGTH, TEXT("%u"),configObject.valueNumeric);
#endif
	}


	if (WritePrivateProfileString(TEXT(CONFIG_FILE_SECTION_NAME), configObject.name, configObject.valueString, configPath))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ConfigurationHolder::readNumericFromConfigFile(ConfigObject & configObject, int nBase)
{
	GetPrivateProfileString(TEXT(CONFIG_FILE_SECTION_NAME),configObject.name,TEXT(""),configObject.valueString, 100, configPath);

	if (wcslen(configObject.valueString) > 0)
	{

#ifdef _WIN64
		configObject.valueNumeric = _wcstoui64(configObject.valueString, NULL, nBase);
#else
		configObject.valueNumeric = wcstoul(configObject.valueString, NULL, nBase);
#endif

		if (configObject.valueNumeric)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}


bool ConfigurationHolder::saveStringToConfigFile(ConfigObject & configObject)
{
	if (WritePrivateProfileString(TEXT(CONFIG_FILE_SECTION_NAME), configObject.name, configObject.valueString, configPath))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ConfigurationHolder::readStringFromConfigFile(ConfigObject & configObject)
{
	GetPrivateProfileString(TEXT(CONFIG_FILE_SECTION_NAME),configObject.name,TEXT(""),configObject.valueString, 100, configPath);

	if (wcslen(configObject.valueString) > 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ConfigurationHolder::readBooleanFromConfigFile(ConfigObject & configObject)
{
	if (GetPrivateProfileInt(TEXT(CONFIG_FILE_SECTION_NAME), configObject.name, 0, configPath) != 0)
	{
		configObject.valueNumeric = 1;
	}
	else
	{
		configObject.valueNumeric = 0;
	}

	return true;
}

bool ConfigurationHolder::saveBooleanToConfigFile(ConfigObject & configObject)
{
	WCHAR *boolValue = 0;

	if (configObject.valueNumeric == 0)
	{
		boolValue = L"0";
	}
	else
	{
		boolValue = L"1";
	}

	if (WritePrivateProfileString(TEXT(CONFIG_FILE_SECTION_NAME), configObject.name, boolValue, configPath))
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ConfigurationHolder::loadConfig(ConfigObject & configObject)
{
	switch (configObject.configType)
	{
	case String:
		return readStringFromConfigFile(configObject);
		break;
	case Boolean:
		return readBooleanFromConfigFile(configObject);
		break;
	case Decimal:
		return readNumericFromConfigFile(configObject, 10);
		break;
	case Hexadecimal:
		return readNumericFromConfigFile(configObject, 16);
		break;
	default:
		return false;
	}
}

bool ConfigurationHolder::saveConfig(ConfigObject & configObject)
{
	switch (configObject.configType)
	{
	case String:
		return saveStringToConfigFile(configObject);
		break;
	case Boolean:
		return saveBooleanToConfigFile(configObject);
		break;
	case Decimal:
		return saveNumericToConfigFile(configObject, 10);
		break;
	case Hexadecimal:
		return saveNumericToConfigFile(configObject, 16);
		break;
	default:
		return false;
	}
}

ConfigObject * ConfigurationHolder::getConfigObject(Configuration configuration)
{
	return &(config.mapConfig[configuration]);
}

bool ConfigurationHolder::buildConfigFilePath()
{
	ZeroMemory(configPath, sizeof(configPath));

	if (!GetModuleFileName(0, configPath, _countof(configPath)))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("buildConfigFilePath :: GetModuleFileName failed %d\r\n",GetLastError());
#endif
		return false;
	}

	//remove exe file name
	for (size_t i = wcslen(configPath) - 1; i >= 0; i--)
	{
		if (configPath[i] == L'\\')
		{
			configPath[i + 1] = 0;
			break;
		}
	}

	wcscat_s(configPath, _countof(configPath), TEXT(CONFIG_FILE_NAME) );

	//wprintf(L"configPath %s\n\n", configPath);

	return true;
}

std::map<Configuration, ConfigObject> & ConfigurationHolder::getConfigList()
{
	return config.mapConfig;
}