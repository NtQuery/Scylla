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
#define PLUGIN_IMPREC_DIR "ImpRec_Plugins"
#define PLUGIN_IMPREC_WRAPPER_DLL "Imprec_Wrapper_DLL.dll"

typedef wchar_t * (__cdecl * def_ScyllaPluginNameW)();
typedef char * (__cdecl * def_ScyllaPluginNameA)();

typedef DWORD ( * def_Imprec_Trace)(DWORD hFileMap, DWORD dwSizeMap, DWORD dwTimeOut, DWORD dwToTrace, DWORD dwExactCall);

class PluginLoader {
public:
	static WCHAR imprecWrapperDllPath[MAX_PATH];

	static bool findAllPlugins();

	static std::vector<Plugin> & getScyllaPluginList();
	static std::vector<Plugin> & getImprecPluginList();

private:
	static std::vector<Plugin> scyllaPluginList;
	static std::vector<Plugin> imprecPluginList;

	static WCHAR dirSearchString[MAX_PATH];
	static WCHAR baseDirPath[MAX_PATH];

	static bool buildSearchString();
	static bool getScyllaPluginName(Plugin * pluginData);
	static bool isValidDllFile( const WCHAR * fullpath );
	static bool searchForPlugin(std::vector<Plugin> & newPluginList, const WCHAR * searchPath, bool isScyllaPlugin);
	static bool isValidImprecPlugin(const WCHAR * fullpath);
	static bool buildSearchStringImprecPlugins();

	static bool fileExists(const WCHAR * fileName);
};