#include "PluginLoader.h"
#include "Logger.h"

#include "ProcessAccessHelp.h"
#include "StringConversion.h"
#include <shlwapi.h>

const WCHAR PluginLoader::PLUGIN_DIR[] = L"Plugins\\";
const WCHAR PluginLoader::PLUGIN_SEARCH_STRING[] = L"*.dll";
const WCHAR PluginLoader::PLUGIN_IMPREC_DIR[] = L"ImpRec_Plugins\\";
const WCHAR PluginLoader::PLUGIN_IMPREC_WRAPPER_DLL[] = L"Imprec_Wrapper_DLL.dll";

//#define DEBUG_COMMENTS

std::vector<Plugin> & PluginLoader::getScyllaPluginList()
{
	return scyllaPluginList;
}

std::vector<Plugin> & PluginLoader::getImprecPluginList()
{
	return imprecPluginList;
}

bool PluginLoader::findAllPlugins()
{

	if (!scyllaPluginList.empty())
	{
		scyllaPluginList.clear();
	}

	if (!imprecPluginList.empty())
	{
		imprecPluginList.clear();
	}

	if (!buildSearchString())
	{
		return false;
	}

	if (!searchForPlugin(scyllaPluginList, dirSearchString, true))
	{
		return false;
	}

#ifndef _WIN64
	if (!buildSearchStringImprecPlugins())
	{
		return false;
	}

	if (!searchForPlugin(imprecPluginList, dirSearchString, false))
	{
		return false;
	}
#endif

	return true;
}

bool PluginLoader::searchForPlugin(std::vector<Plugin> & newPluginList, const WCHAR * searchPath, bool isScyllaPlugin)
{
	WIN32_FIND_DATA ffd;
	HANDLE hFind = 0;
	DWORD dwError = 0;
	Plugin pluginData;

	hFind = FindFirstFile(searchPath, &ffd);

	dwError = GetLastError();

	if (dwError == ERROR_FILE_NOT_FOUND)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"findAllPlugins :: No files found");
#endif
		return true;
	}

	if (hFind == INVALID_HANDLE_VALUE)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"findAllPlugins :: FindFirstFile failed %d", dwError);
#endif
		return false;
	}

	do
	{
		if ( !(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{

			if ((ffd.nFileSizeHigh != 0) || (ffd.nFileSizeLow < 200))
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"findAllPlugins :: Plugin invalid file size: %s", ffd.cFileName);
#endif
			}
			else
			{
				pluginData.fileSize = ffd.nFileSizeLow;
				wcscpy_s(pluginData.fullpath, _countof(baseDirPath), baseDirPath);
				wcscat_s(pluginData.fullpath, _countof(baseDirPath), ffd.cFileName);

#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"findAllPlugins :: Plugin %s", pluginData.fullpath);
#endif
				if (isValidDllFile(pluginData.fullpath))
				{
					if (isScyllaPlugin)
					{
						if (getScyllaPluginName(&pluginData))
						{
							//add valid plugin
							newPluginList.push_back(pluginData);
						}
						else
						{
#ifdef DEBUG_COMMENTS
							Scylla::debugLog.log(L"Cannot get scylla plugin name %s", pluginData.fullpath);
#endif
						}
					}
					else
					{
						if (isValidImprecPlugin(pluginData.fullpath))
						{
							wcscpy_s(pluginData.pluginName, MAX_PATH, ffd.cFileName);
							newPluginList.push_back(pluginData);
						}
					}

				}

			}

		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();

	FindClose(hFind);

	if (dwError == ERROR_NO_MORE_FILES)
	{
		return true;
	}
	else
	{
		return false;
	}

}

bool PluginLoader::getScyllaPluginName(Plugin * pluginData)
{
	bool retValue = false;
	char * pluginName = 0;
	def_ScyllaPluginNameW ScyllaPluginNameW = 0;
	def_ScyllaPluginNameA ScyllaPluginNameA = 0;

	HMODULE hModule = LoadLibraryEx(pluginData->fullpath, 0, DONT_RESOLVE_DLL_REFERENCES); //do not call DllMain

	if (hModule)
	{
		ScyllaPluginNameW = (def_ScyllaPluginNameW)GetProcAddress(hModule, "ScyllaPluginNameW");

		if (ScyllaPluginNameW)
		{
			wcscpy_s(pluginData->pluginName, MAX_PATH, ScyllaPluginNameW());

#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getPluginName :: Plugin name %s", pluginData->pluginName);
#endif

			retValue = true;
		}
		else
		{
			ScyllaPluginNameA = (def_ScyllaPluginNameA)GetProcAddress(hModule, "ScyllaPluginNameA");

			if (ScyllaPluginNameA)
			{
				pluginName = ScyllaPluginNameA();

				StringConversion::ToUTF16(pluginName, pluginData->pluginName, _countof(pluginData->pluginName));

#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"getPluginName :: Plugin name mbstowcs_s %s", pluginData->pluginName);
#endif

				if (wcslen(pluginData->pluginName) > 1)
				{
					retValue = true;
				}
				else
				{
					retValue = false;
				}
			}
			else
			{
				retValue = false;
			}
		}

		FreeLibrary(hModule);

		return retValue;
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"getPluginName :: LoadLibraryEx failed %s", pluginData->fullpath);
#endif
		return false;
	}
}

bool PluginLoader::buildSearchString()
{
	ZeroMemory(dirSearchString, sizeof(dirSearchString));
	ZeroMemory(baseDirPath, sizeof(baseDirPath));

	if (!GetModuleFileName(0, dirSearchString, _countof(dirSearchString)))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"buildSearchString :: GetModuleFileName failed %d", GetLastError());
#endif
		return false;
	}

	//wprintf(L"dirSearchString 1 %s\n\n", dirSearchString);
	PathRemoveFileSpec(dirSearchString);
	//wprintf(L"dirSearchString 2 %s\n\n", dirSearchString);
	PathAppend(dirSearchString, PLUGIN_DIR);

	wcscpy_s(baseDirPath, _countof(baseDirPath), dirSearchString);
	wcscat_s(dirSearchString, _countof(dirSearchString), PLUGIN_SEARCH_STRING);

	//wprintf(L"dirSearchString 3 %s\n\n", dirSearchString);

#ifdef DEBUG_COMMENTS
	Scylla::debugLog.log(L"dirSearchString final %s", dirSearchString);
#endif


	return true;
}

bool PluginLoader::isValidDllFile( const WCHAR * fullpath )
{
	BYTE * data = 0;
	DWORD lpNumberOfBytesRead = 0;
	PIMAGE_DOS_HEADER pDos = 0;
	PIMAGE_NT_HEADERS pNT = 0;
	bool retValue = false;

	HANDLE hFile = CreateFile(fullpath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		data = new BYTE[sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS) + 0x100];

		if (ReadFile(hFile, data, sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS) + 0x100, &lpNumberOfBytesRead, 0))
		{
			pDos = (PIMAGE_DOS_HEADER)data;

			if (pDos->e_magic == IMAGE_DOS_SIGNATURE)
			{
				pNT = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDos + pDos->e_lfanew);

				if (pNT->Signature == IMAGE_NT_SIGNATURE)
				{
#ifdef _WIN64
					if (pNT->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
#else
					if (pNT->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
#endif
					{
						retValue = true;
					}
				}			
			}
		}

		delete [] data;
		CloseHandle(hFile);
	}

	return retValue;
}

bool PluginLoader::isValidImprecPlugin(const WCHAR * fullpath)
{
	def_Imprec_Trace Imprec_Trace = 0;
	bool retValue = false;

	HMODULE hModule = LoadLibraryEx(fullpath, 0, DONT_RESOLVE_DLL_REFERENCES); //do not call DllMain

	if (hModule)
	{
		Imprec_Trace = (def_Imprec_Trace)GetProcAddress(hModule, "Trace");
		if (Imprec_Trace)
		{
			retValue = true;
		}
		else
		{
			retValue = false;
		}

		FreeLibrary(hModule);
		return retValue;
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"isValidImprecPlugin :: LoadLibraryEx failed %s", pluginData->fullpath);
#endif
		return false;
	}
}

bool PluginLoader::buildSearchStringImprecPlugins()
{
	wcscpy_s(dirSearchString, _countof(dirSearchString), baseDirPath);

	wcscat_s(dirSearchString, _countof(dirSearchString), PLUGIN_IMPREC_DIR);

	wcscpy_s(baseDirPath, _countof(baseDirPath), dirSearchString);

	//build imprec wrapper dll path
	wcscpy_s(imprecWrapperDllPath, _countof(imprecWrapperDllPath), dirSearchString);
	wcscat_s(imprecWrapperDllPath, _countof(imprecWrapperDllPath), PLUGIN_IMPREC_WRAPPER_DLL);

	if (!fileExists(imprecWrapperDllPath))
	{
		return false;
	}

	wcscat_s(dirSearchString, _countof(dirSearchString), PLUGIN_SEARCH_STRING);

	return true;
}

bool PluginLoader::fileExists(const WCHAR * fileName)
{
	if (GetFileAttributesW(fileName) == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	else
	{
		return true;
	}
}
