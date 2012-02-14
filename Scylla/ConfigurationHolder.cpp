#include "ConfigurationHolder.h"

#include <shlwapi.h>
#include <stdio.h>
#include "Architecture.h"

const WCHAR ConfigurationHolder::CONFIG_FILE_NAME[] = L"Scylla.ini";
const WCHAR ConfigurationHolder::CONFIG_FILE_SECTION_NAME[] = L"SCYLLA_CONFIG";

//#define DEBUG_COMMENTS

ConfigurationInitializer::ConfigurationInitializer()
{
	ConfigObject configObject;

	mapConfig[USE_PE_HEADER_FROM_DISK]   = configObject.newValues(L"USE_PE_HEADER_FROM_DISK",   Boolean);
	mapConfig[DEBUG_PRIVILEGE]           = configObject.newValues(L"DEBUG_PRIVILEGE",           Boolean);
	mapConfig[CREATE_BACKUP]             = configObject.newValues(L"CREATE_BACKUP",             Boolean);
	mapConfig[DLL_INJECTION_AUTO_UNLOAD] = configObject.newValues(L"DLL_INJECTION_AUTO_UNLOAD", Boolean);
	mapConfig[UPDATE_HEADER_CHECKSUM]    = configObject.newValues(L"UPDATE_HEADER_CHECKSUM",    Boolean);
	mapConfig[IAT_SECTION_NAME]          = configObject.newValues(L"IAT_SECTION_NAME",          String);
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
		swprintf_s(configObject.valueString, CONFIG_OPTIONS_STRING_LENGTH, PRINTF_DWORD_PTR_FULL, configObject.valueNumeric);
	}
	else
	{
		swprintf_s(configObject.valueString, CONFIG_OPTIONS_STRING_LENGTH, PRINTF_INTEGER, configObject.valueNumeric);
	}


	BOOL ret = WritePrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.name, configObject.valueString, configPath);
	return ret == TRUE;
}

bool ConfigurationHolder::readNumericFromConfigFile(ConfigObject & configObject, int nBase)
{
	DWORD read = GetPrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.name, L"", configObject.valueString, _countof(configObject.valueString), configPath);

	if (read > 0 && wcslen(configObject.valueString) > 0)
	{

#ifdef _WIN64
		configObject.valueNumeric = _wcstoui64(configObject.valueString, NULL, nBase);
#else
		configObject.valueNumeric = wcstoul(configObject.valueString, NULL, nBase);
#endif

		return (configObject.valueNumeric != 0);
	}
	else
	{
		return false;
	}
}

bool ConfigurationHolder::saveStringToConfigFile(ConfigObject & configObject)
{
	BOOL ret = WritePrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.name, configObject.valueString, configPath);
	return ret == TRUE;
}

bool ConfigurationHolder::readStringFromConfigFile(ConfigObject & configObject)
{
	DWORD read = GetPrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.name, L"", configObject.valueString, _countof(configObject.valueString), configPath);
	return (read > 0 && wcslen(configObject.valueString) > 0);
}

bool ConfigurationHolder::readBooleanFromConfigFile(ConfigObject & configObject)
{
	UINT val = GetPrivateProfileInt(CONFIG_FILE_SECTION_NAME, configObject.name, 0, configPath);
	configObject.valueNumeric = val ? 1 : 0;
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

	BOOL ret = WritePrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.name, boolValue, configPath);
	return ret == TRUE;
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
		Scylla::debugLog.log(L"buildConfigFilePath :: GetModuleFileName failed %d", GetLastError());
#endif
		return false;
	}

	PathRemoveFileSpec(configPath);
	PathAppend(configPath, CONFIG_FILE_NAME);

	//wprintf(L"configPath %s\n\n", configPath);

	return true;
}

std::map<Configuration, ConfigObject> & ConfigurationHolder::getConfigList()
{
	return config.mapConfig;
}
