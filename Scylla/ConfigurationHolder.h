#pragma once

#include <windows.h>
#include <map>

enum ConfigType {
	String,
	Decimal,
	Hexadecimal,
	Boolean
};

enum Configuration {
	USE_PE_HEADER_FROM_DISK,
	DEBUG_PRIVILEGE,
	CREATE_BACKUP,
	DLL_INJECTION_AUTO_UNLOAD,
	IAT_SECTION_NAME,
	UPDATE_HEADER_CHECKSUM,
};

const size_t CONFIG_OPTIONS_STRING_LENGTH = 100;

class ConfigObject {
public:
	WCHAR name[MAX_PATH];
	ConfigType configType;

	DWORD_PTR valueNumeric;
	WCHAR valueString[CONFIG_OPTIONS_STRING_LENGTH];

	ConfigObject& newValues(WCHAR * configname, ConfigType config)
	{
		wcscpy_s(name, MAX_PATH, configname);
		configType = config;
		valueNumeric = 0;
		ZeroMemory(valueString, sizeof(valueString));

		return *this;
	}

	bool isTrue()
	{
		return (valueNumeric == 1);
	}

	void setTrue()
	{
		valueNumeric = 1;
	}

	void setFalse()
	{
		valueNumeric = 0;
	}
};

class ConfigurationInitializer {
public:
	std::map<Configuration, ConfigObject> mapConfig;

	ConfigurationInitializer();
};

class ConfigurationHolder {
public:

	bool loadConfiguration();
	bool saveConfiguration();

	ConfigObject * getConfigObject(Configuration configuration);
	std::map<Configuration, ConfigObject> & getConfigList();

private:

	static const WCHAR CONFIG_FILE_NAME[];
	static const WCHAR CONFIG_FILE_SECTION_NAME[];

	ConfigurationInitializer config;
	WCHAR configPath[MAX_PATH];

	bool buildConfigFilePath();

	bool readStringFromConfigFile(ConfigObject & configObject);
	bool readBooleanFromConfigFile(ConfigObject & configObject);
	bool readNumericFromConfigFile(ConfigObject & configObject, int nBase);

	bool saveStringToConfigFile(ConfigObject & configObject);
	bool saveBooleanToConfigFile(ConfigObject & configObject);
	bool saveNumericToConfigFile(ConfigObject & configObject, int nBase);

	bool loadConfig(ConfigObject & configObject);
	bool saveConfig(ConfigObject & configObject);
};
