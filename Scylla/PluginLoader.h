#pragma once

#include <windows.h>
#include <vector>

class Plugin {
public:
	DWORD fileSize;
	WCHAR fullpath[MAX_PATH];
	WCHAR pluginName[MAX_PATH];
};

typedef wchar_t * (__cdecl * def_ScyllaPluginNameW)();
typedef char * (__cdecl * def_ScyllaPluginNameA)();

typedef DWORD ( * def_Imprec_Trace)(DWORD hFileMap, DWORD dwSizeMap, DWORD dwTimeOut, DWORD dwToTrace, DWORD dwExactCall);

class PluginLoader {
public:
	WCHAR imprecWrapperDllPath[MAX_PATH];

	bool findAllPlugins();

	std::vector<Plugin> & getScyllaPluginList();
	std::vector<Plugin> & getImprecPluginList();

private:

	static const WCHAR PLUGIN_DIR[];
	static const WCHAR PLUGIN_SEARCH_STRING[];
	static const WCHAR PLUGIN_IMPREC_DIR[];
	static const WCHAR PLUGIN_IMPREC_WRAPPER_DLL[];

	std::vector<Plugin> scyllaPluginList;
	std::vector<Plugin> imprecPluginList;

	WCHAR dirSearchString[MAX_PATH];
	WCHAR baseDirPath[MAX_PATH];

	bool buildSearchString();
	bool buildSearchStringImprecPlugins();

	bool getScyllaPluginName(Plugin * pluginData);
	bool searchForPlugin(std::vector<Plugin> & newPluginList, const WCHAR * searchPath, bool isScyllaPlugin);

	static bool fileExists(const WCHAR * fileName);
	static bool isValidDllFile(const WCHAR * fullpath);
	static bool isValidImprecPlugin(const WCHAR * fullpath);
};
