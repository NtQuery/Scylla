#include "PluginLoader.h"
#include "Logger.h"

#include "ProcessAccessHelp.h"

std::vector<Plugin> PluginLoader::pluginList;
WCHAR PluginLoader::dirSearchString[MAX_PATH];
WCHAR PluginLoader::baseDirPath[MAX_PATH];

//#define DEBUG_COMMENTS

std::vector<Plugin> & PluginLoader::getPluginList()
{
	return pluginList;
}

bool PluginLoader::findAllPlugins()
{
	WIN32_FIND_DATA ffd;
	HANDLE hFind = 0;
	DWORD dwError = 0;
	Plugin pluginData;

	if (!pluginList.empty())
	{
		pluginList.clear();
	}

	if (!buildSearchString())
	{
		return false;
	}

	hFind = FindFirstFile(dirSearchString, &ffd);

	dwError = GetLastError();

	if (dwError == ERROR_FILE_NOT_FOUND)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("findAllPlugins :: No files found\r\n");
#endif
		return true;
	}

	if (hFind == INVALID_HANDLE_VALUE)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("findAllPlugins :: FindFirstFile failed %d\r\n", dwError);
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
				Logger::debugLog(TEXT("findAllPlugins :: Plugin invalid file size: %s\r\n"), ffd.cFileName);
#endif
			}
			else
			{
				pluginData.fileSize = ffd.nFileSizeLow;
				wcscpy_s(pluginData.fullpath, _countof(baseDirPath), baseDirPath);
				wcscat_s(pluginData.fullpath, _countof(baseDirPath), ffd.cFileName);

#ifdef DEBUG_COMMENTS
				Logger::debugLog(L"findAllPlugins :: Plugin %s\r\n",pluginData.fullpath);
#endif
				if (isValidDllFile(pluginData.fullpath))
				{
					if (getPluginName(&pluginData))
					{
						//add valid plugin
						pluginList.push_back(pluginData);
					}
					else
					{
#ifdef DEBUG_COMMENTS
						Logger::debugLog(TEXT("Cannot get plugin name %s\r\n"),pluginData.fullpath);
#endif
					}
				}

			}

		}
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();

	if (dwError == ERROR_NO_MORE_FILES)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool PluginLoader::getPluginName(Plugin * pluginData)
{
	bool retValue = false;
	char * pluginName = 0;
	size_t convertedChars = 0;
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
			Logger::debugLog(L"getPluginName :: Plugin name %s\r\n", pluginData->pluginName);
#endif

			retValue = true;
		}
		else
		{
			ScyllaPluginNameA = (def_ScyllaPluginNameA)GetProcAddress(hModule, "ScyllaPluginNameA");

			if (ScyllaPluginNameA)
			{
				pluginName = ScyllaPluginNameA();

				mbstowcs_s(&convertedChars, pluginData->pluginName, strlen(pluginName) + 1, pluginName, _TRUNCATE);

#ifdef DEBUG_COMMENTS
				Logger::debugLog(L"getPluginName :: Plugin name mbstowcs_s %s\r\n", pluginData->pluginName);
#endif

				if (convertedChars > 1)
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
		Logger::debugLog(L"getPluginName :: LoadLibraryEx failed %s\r\n", pluginData->fullpath);
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
		Logger::debugLog("buildSearchString :: GetModuleFileName failed %d\r\n",GetLastError());
#endif
		return false;
	}

	//wprintf(L"dirSearchString 1 %s\n\n", dirSearchString);


	//remove exe file name
	for (size_t i = wcslen(dirSearchString) - 1; i >= 0; i--)
	{
		if (dirSearchString[i] == L'\\')
		{
			dirSearchString[i + 1] = 0;
			break;
		}
	}

	//wprintf(L"dirSearchString 2 %s\n\n", dirSearchString);

	wcscat_s(dirSearchString, _countof(dirSearchString), TEXT(PLUGIN_DIR)TEXT("\\") );

	wcscpy_s(baseDirPath, _countof(baseDirPath), dirSearchString);

	wcscat_s(dirSearchString, _countof(dirSearchString), TEXT(PLUGIN_SEARCH_STRING) );

	//wprintf(L"dirSearchString 3 %s\n\n", dirSearchString);

#ifdef DEBUG_COMMENTS
	Logger::debugLog(L"dirSearchString final %s\r\n", dirSearchString);
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
