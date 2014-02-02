#include "ConfigurationHolder.h"

#include <shlwapi.h>
#include "Architecture.h"

const WCHAR ConfigurationHolder::CONFIG_FILE_SECTION_NAME[] = L"SCYLLA_CONFIG";

//#define DEBUG_COMMENTS

ConfigurationHolder::ConfigurationHolder(const WCHAR* fileName)
{
	config[USE_PE_HEADER_FROM_DISK]     = Configuration(L"USE_PE_HEADER_FROM_DISK",      Configuration::Boolean);
	config[DEBUG_PRIVILEGE]             = Configuration(L"DEBUG_PRIVILEGE",              Configuration::Boolean);
	config[CREATE_BACKUP]               = Configuration(L"CREATE_BACKUP",                Configuration::Boolean);
	config[DLL_INJECTION_AUTO_UNLOAD]   = Configuration(L"DLL_INJECTION_AUTO_UNLOAD",    Configuration::Boolean);
	config[UPDATE_HEADER_CHECKSUM]      = Configuration(L"UPDATE_HEADER_CHECKSUM",       Configuration::Boolean);
	config[IAT_SECTION_NAME]            = Configuration(L"IAT_SECTION_NAME",             Configuration::String);
	config[REMOVE_DOS_HEADER_STUB]      = Configuration(L"REMOVE_DOS_HEADER_STUB",       Configuration::Boolean);
	config[IAT_FIX_AND_OEP_FIX]         = Configuration(L"IAT_FIX_AND_OEP_FIX",          Configuration::Boolean);
	config[SUSPEND_PROCESS_FOR_DUMPING] = Configuration(L"SUSPEND_PROCESS_FOR_DUMPING",  Configuration::Boolean);
	config[OriginalFirstThunk_SUPPORT]  = Configuration(L"OriginalFirstThunk_SUPPORT",	 Configuration::Boolean);
	config[USE_ADVANCED_IAT_SEARCH]     = Configuration(L"USE_ADVANCED_IAT_SEARCH",	     Configuration::Boolean);
	config[SCAN_AND_FIX_DIRECT_IMPORTS] = Configuration(L"SCAN_AND_FIX_DIRECT_IMPORTS",	 Configuration::Boolean);
	config[CREATE_NEW_IAT_IN_SECTION] = Configuration(L"CREATE_NEW_IAT_IN_SECTION",	 Configuration::Boolean);
	buildConfigFilePath(fileName);
}

bool ConfigurationHolder::loadConfiguration()
{
	std::map<ConfigOption, Configuration>::iterator mapIter;

	if (configPath[0] == '\0')
	{
		return false;
	}

	for (mapIter = config.begin() ; mapIter != config.end(); mapIter++)
	{
		Configuration& configObject = mapIter->second;
		loadConfig(configObject);
	}

	return true;
}

bool ConfigurationHolder::saveConfiguration() const
{
	std::map<ConfigOption, Configuration>::const_iterator mapIter;

	if (configPath[0] == '\0')
	{
		return false;
	}

	for (mapIter = config.begin() ; mapIter != config.end(); mapIter++)
	{
		const Configuration& configObject = mapIter->second;
		if (!saveConfig(configObject))
		{
			return false;
		}
	}

	return true;
}

Configuration& ConfigurationHolder::operator[](ConfigOption option)
{
	return config[option];
}

const Configuration& ConfigurationHolder::operator[](ConfigOption option) const
{
	static const Configuration dummy;

	std::map<ConfigOption, Configuration>::const_iterator found = config.find(option);
	if(found != config.end())
	{
		return found->second;
	}
	else
	{
		return dummy;
	}
}

bool ConfigurationHolder::saveNumericToConfigFile(const Configuration & configObject, int nBase) const
{
	WCHAR buf[21]; // UINT64_MAX in dec has 20 digits

	if (nBase == 16)
	{
		swprintf_s(buf, PRINTF_DWORD_PTR_FULL, configObject.getNumeric());
	}
	else
	{
		swprintf_s(buf, PRINTF_INTEGER, configObject.getNumeric());
	}

	BOOL ret = WritePrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.getName(), buf, configPath);
	return ret == TRUE;
}

bool ConfigurationHolder::readNumericFromConfigFile(Configuration & configObject, int nBase)
{
	WCHAR buf[21]; // UINT64_MAX in dec has 20 digits
	DWORD read = GetPrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.getName(), L"", buf, _countof(buf), configPath);

	if (read > 0 && wcslen(buf) > 0)
	{
#ifdef _WIN64
		configObject.setNumeric(_wcstoui64(buf, NULL, nBase));
#else
		configObject.setNumeric(wcstoul(buf, NULL, nBase));
#endif
		return true;
	}

	return false;
}

bool ConfigurationHolder::saveStringToConfigFile(const Configuration & configObject) const
{
	BOOL ret = WritePrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.getName(), configObject.getString(), configPath);
	return ret == TRUE;
}

bool ConfigurationHolder::readStringFromConfigFile(Configuration & configObject)
{
	WCHAR buf[Configuration::CONFIG_STRING_LENGTH];
	DWORD read = GetPrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.getName(), L"", buf, _countof(buf), configPath);
	if(read > 0 && wcslen(buf) > 0)
	{
		configObject.setString(buf);
		return true;
	}

	return false;
}

bool ConfigurationHolder::readBooleanFromConfigFile(Configuration & configObject)
{
	UINT val = GetPrivateProfileInt(CONFIG_FILE_SECTION_NAME, configObject.getName(), 0, configPath);
	configObject.setBool(val != 0);
	return true;
}

bool ConfigurationHolder::saveBooleanToConfigFile(const Configuration & configObject) const
{
	const WCHAR *boolValue = configObject.isTrue() ? L"1" : L"0";
	BOOL ret = WritePrivateProfileString(CONFIG_FILE_SECTION_NAME, configObject.getName(), boolValue, configPath);
	return ret == TRUE;
}

bool ConfigurationHolder::loadConfig(Configuration & configObject)
{
	switch (configObject.getType())
	{
	case Configuration::String:
		return readStringFromConfigFile(configObject);
	case Configuration::Boolean:
		return readBooleanFromConfigFile(configObject);
	case Configuration::Decimal:
		return readNumericFromConfigFile(configObject, 10);
	case Configuration::Hexadecimal:
		return readNumericFromConfigFile(configObject, 16);
	default:
		return false;
	}
}

bool ConfigurationHolder::saveConfig(const Configuration & configObject) const
{
	switch (configObject.getType())
	{
	case Configuration::String:
		return saveStringToConfigFile(configObject);
	case Configuration::Boolean:
		return saveBooleanToConfigFile(configObject);
	case Configuration::Decimal:
		return saveNumericToConfigFile(configObject, 10);
	case Configuration::Hexadecimal:
		return saveNumericToConfigFile(configObject, 16);
	default:
		return false;
	}
}

bool ConfigurationHolder::buildConfigFilePath(const WCHAR* fileName)
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
	PathAppend(configPath, fileName);

	return true;
}
