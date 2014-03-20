#pragma once

#include "ConfigurationHolder.h"
#include "PluginLoader.h"
#include "ProcessLister.h"
#include "Logger.h"

#define APPNAME_S "Scylla"
#define APPVERSION_S "v0.9.5b"
#define APPVERSIONDWORD 0x00009500

#define DONATE_BTC_ADDRESS "1GmVrhWwUhwLohaCLP4SKV5kkz8rd16N8h"

#define APPNAME TEXT(APPNAME_S)
#define APPVERSION TEXT(APPVERSION_S)

class Scylla
{
public:

	static void initAsGuiApp();
	static void initAsDll();

	static ConfigurationHolder config;
	static PluginLoader plugins;

	static ProcessLister processLister;

	static FileLog debugLog;
	static ListboxLog windowLog;

private:

	static const WCHAR DEBUG_LOG_FILENAME[];
};
