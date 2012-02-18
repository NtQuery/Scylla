#pragma once

#include "ConfigurationHolder.h"
#include "PluginLoader.h"
#include "ProcessLister.h"
#include "Logger.h"

#define APPNAME_S "Scylla"
#define APPVERSION_S "v0.5a"

#define APPNAME TEXT(APPNAME_S)
#define APPVERSION TEXT(APPVERSION_S)

class Scylla
{
public:

	static void init();

	static ConfigurationHolder config;
	static PluginLoader plugins;

	static ProcessLister processLister;

	static FileLog debugLog;
	static ListboxLog windowLog;

private:

	static const WCHAR DEBUG_LOG_FILENAME[];
};
