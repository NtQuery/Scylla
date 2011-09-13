
#pragma once

#include <windows.h>
#include <stdio.h>
#include <map>

#define CONFIG_FILE_NAME "Scylla.ini"
#define CONFIG_FILE_SECTION_NAME "SCYLLA_CONFIG"

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

#define CONFIG_OPTIONS_STRING_LENGTH 100

class ConfigObject {
public:
	WCHAR name[MAX_PATH];
	ConfigType configType;

	DWORD_PTR valueNumeric;
	WCHAR valueString[CONFIG_OPTIONS_STRING_LENGTH];

	int dialogItemValue;

	ConfigObject& newValues(WCHAR * configname, ConfigType config, int dlgValue)
	{
		wcscpy_s(name, MAX_PATH, configname);
		configType = config;
		valueNumeric = 0;
		ZeroMemory(valueString, sizeof(valueString));
		dialogItemValue = dlgValue;

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

	static bool loadConfiguration();
	static bool saveConfiguration();

	static ConfigObject * getConfigObject(Configuration configuration);
	static std::map<Configuration, ConfigObject> & getConfigList();

private:
	static ConfigurationInitializer config;
	static WCHAR configPath[MAX_PATH];

	static bool buildConfigFilePath();

	

	static bool readStringFromConfigFile(ConfigObject & configObject);
	static bool readBooleanFromConfigFile(ConfigObject & configObject);
	static bool readNumericFromConfigFile(ConfigObject & configObject, int nBase);

	static bool saveStringToConfigFile(ConfigObject & configObject);
	static bool saveBooleanToConfigFile(ConfigObject & configObject);
	static bool saveNumericToConfigFile(ConfigObject & configObject, int nBase);

	static bool loadConfig(ConfigObject & configObject);
	static bool saveConfig(ConfigObject & configObject);
};