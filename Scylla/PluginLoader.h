#pragma once

#include <windows.h>
#include <stdio.h>
#include <vector>

class Plugin {
public:
	DWORD fileSize;
	WCHAR fullpath[MAX_PATH];
	WCHAR pluginName[MAX_PATH];
};

#define PLUGIN_DIR "Plugins"
#define PLUGIN_SEARCH_STRING "*.dll"

typedef wchar_t * (__cdecl * def_ScyllaPluginNameW)();
typedef char * (__cdecl * def_ScyllaPluginNameA)();

class PluginLoader {
public:
	static bool findAllPlugins();

	static std::vector<Plugin> & getPluginList();

private:
	static std::vector<Plugin> pluginList;

	static WCHAR dirSearchString[MAX_PATH];
	static WCHAR baseDirPath[MAX_PATH];

	static bool buildSearchString();
	static bool getPluginName(Plugin * pluginData);
	static bool isValidDllFile( const WCHAR * fullpath );
};