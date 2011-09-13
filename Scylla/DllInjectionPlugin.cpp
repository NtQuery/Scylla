#include "DllInjectionPlugin.h"
#include "Logger.h"

const WCHAR * DllInjectionPlugin::FILE_MAPPING_NAME = L"ScyllaPluginExchange";

HANDLE DllInjectionPlugin::hProcess = 0;

//#define DEBUG_COMMENTS

void DllInjectionPlugin::injectPlugin(Plugin & plugin, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR imageBase, DWORD_PTR imageSize)
{
	PSCYLLA_EXCHANGE scyllaExchange = 0;
	PUNRESOLVED_IMPORT unresImp = 0;

	BYTE * dataBuffer = 0;
	DWORD_PTR numberOfUnresolvedImports = getNumberOfUnresolvedImports(moduleList);

	if (numberOfUnresolvedImports == 0)
	{
		Logger::printfDialog(L"No unresolved Imports");
		return;
	}

	if (!createFileMapping((DWORD)(sizeof(SCYLLA_EXCHANGE) + sizeof(UNRESOLVED_IMPORT) + (sizeof(UNRESOLVED_IMPORT) * numberOfUnresolvedImports))))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(L"injectPlugin :: createFileMapping %X failed\r\n",sizeof(SCYLLA_EXCHANGE) + sizeof(UNRESOLVED_IMPORT) + (sizeof(UNRESOLVED_IMPORT) * numberOfUnresolvedImports));
#endif
		return;
	}

	scyllaExchange = (PSCYLLA_EXCHANGE)lpViewOfFile;
	scyllaExchange->status = 0xFF;
	scyllaExchange->imageBase = imageBase;
	scyllaExchange->imageSize = imageSize;
	scyllaExchange->numberOfUnresolvedImports = numberOfUnresolvedImports;
	scyllaExchange->offsetUnresolvedImportsArray = sizeof(SCYLLA_EXCHANGE);

	unresImp = (PUNRESOLVED_IMPORT)((DWORD_PTR)lpViewOfFile + sizeof(SCYLLA_EXCHANGE));

	addUnresolvedImports(unresImp, moduleList);

	UnmapViewOfFile(lpViewOfFile);
	lpViewOfFile = 0;

	HMODULE hDll = dllInjection(hProcess, plugin.fullpath);
	if (hDll)
	{
		Logger::printfDialog(L"Plugin injection was successful");
		if (!unloadDllInProcess(hProcess,hDll))
		{
			Logger::printfDialog(L"Plugin unloading failed");
		}
		lpViewOfFile = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		if (lpViewOfFile)
		{
			scyllaExchange = (PSCYLLA_EXCHANGE)lpViewOfFile;
			handlePluginResults(scyllaExchange, moduleList);
		}

	}
	else
	{
		Logger::printfDialog(L"Plugin injection failed");
	}

	closeAllHandles();
}

void DllInjectionPlugin::injectImprecPlugin(Plugin & plugin, std::map<DWORD_PTR, ImportModuleThunk> & moduleList, DWORD_PTR imageBase, DWORD_PTR imageSize)
{
	Plugin newPlugin;
	size_t mapSize = (wcslen(plugin.fullpath) + 1) * sizeof(WCHAR);

	HANDLE hImprecMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE|SEC_COMMIT, 0, (DWORD)mapSize, TEXT(PLUGIN_IMPREC_EXCHANGE_DLL_PATH));
	
	if (hImprecMap == NULL)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("injectImprecPlugin :: CreateFileMapping failed 0x%X\r\n",GetLastError());
#endif
		return;
	}

	LPVOID lpImprecViewOfFile = MapViewOfFile(hImprecMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (lpImprecViewOfFile == NULL)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("injectImprecPlugin :: MapViewOfFile failed 0x%X\r\n",GetLastError());
#endif
		CloseHandle(hImprecMap);
		return;
	}

	CopyMemory(lpImprecViewOfFile,plugin.fullpath, mapSize);

	UnmapViewOfFile(lpImprecViewOfFile);

	newPlugin.fileSize = plugin.fileSize;
	wcscpy_s(newPlugin.pluginName, plugin.pluginName);
	wcscpy_s(newPlugin.fullpath, PluginLoader::imprecWrapperDllPath);

	injectPlugin(newPlugin,moduleList,imageBase,imageSize);

	CloseHandle(hImprecMap);
}



bool DllInjectionPlugin::createFileMapping(DWORD mappingSize)
{
	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE|SEC_COMMIT, 0, mappingSize, FILE_MAPPING_NAME);

	if (hMapFile == NULL)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("createFileMapping :: CreateFileMapping failed 0x%X\r\n",GetLastError());
#endif
		return false;
	}

	lpViewOfFile = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if (lpViewOfFile == NULL)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("createFileMapping :: MapViewOfFile failed 0x%X\r\n",GetLastError());
#endif
		CloseHandle(hMapFile);
		hMapFile = 0;
		return false;
	}
	else
	{
		return true;
	}
}

void DllInjectionPlugin::closeAllHandles()
{
	if (lpViewOfFile)
	{
		UnmapViewOfFile(lpViewOfFile);
		lpViewOfFile = 0;
	}
	if (hMapFile)
	{
		CloseHandle(hMapFile);
		hMapFile = 0;
	}
}

DWORD_PTR DllInjectionPlugin::getNumberOfUnresolvedImports( std::map<DWORD_PTR, ImportModuleThunk> & moduleList )
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk = 0;
	ImportThunk * importThunk = 0;
	DWORD_PTR dwNumber = 0;

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (importThunk->valid == false)
			{
				dwNumber++;
			}

			iterator2++;
		}

		iterator1++;
	}

	return dwNumber;
}

void DllInjectionPlugin::addUnresolvedImports( PUNRESOLVED_IMPORT firstUnresImp, std::map<DWORD_PTR, ImportModuleThunk> & moduleList )
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk = 0;
	ImportThunk * importThunk = 0;

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (importThunk->valid == false)
			{
				firstUnresImp->InvalidApiAddress = importThunk->apiAddressVA;
				firstUnresImp->ImportTableAddressPointer = importThunk->va;
				firstUnresImp++;
			}

			iterator2++;
		}

		iterator1++;
	}

	firstUnresImp->InvalidApiAddress = 0;
	firstUnresImp->ImportTableAddressPointer = 0;
}

void DllInjectionPlugin::handlePluginResults( PSCYLLA_EXCHANGE scyllaExchange, std::map<DWORD_PTR, ImportModuleThunk> & moduleList )
{
	PUNRESOLVED_IMPORT unresImp = (PUNRESOLVED_IMPORT)((DWORD_PTR)scyllaExchange + scyllaExchange->offsetUnresolvedImportsArray);;

	switch (scyllaExchange->status)
	{
	case SCYLLA_STATUS_SUCCESS:
		Logger::printfDialog(L"Plugin was successful");
		updateImportsWithPluginResult(unresImp, moduleList);
		break;
	case SCYLLA_STATUS_UNKNOWN_ERROR:
		Logger::printfDialog(L"Plugin reported Unknown Error");
		break;
	case SCYLLA_STATUS_UNSUPPORTED_PROTECTION:
		Logger::printfDialog(L"Plugin detected unknown protection");
		updateImportsWithPluginResult(unresImp, moduleList);
		break;
	case SCYLLA_STATUS_IMPORT_RESOLVING_FAILED:
		Logger::printfDialog(L"Plugin import resolving failed");
		updateImportsWithPluginResult(unresImp, moduleList);
		break;
	case SCYLLA_STATUS_MAPPING_FAILED:
		Logger::printfDialog(L"Plugin file mapping failed");
		break;
	default:
		Logger::printfDialog(L"Plugin failed without reason");
	}
}

void DllInjectionPlugin::updateImportsWithPluginResult( PUNRESOLVED_IMPORT firstUnresImp, std::map<DWORD_PTR, ImportModuleThunk> & moduleList )
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk = 0;
	ImportThunk * importThunk = 0;
	ApiInfo * apiInfo = 0;
	bool isSuspect = 0;

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (importThunk->valid == false)
			{
				if (apiReader->isApiAddressValid(firstUnresImp->InvalidApiAddress))
				{
					apiInfo = apiReader->getApiByVirtualAddress(firstUnresImp->InvalidApiAddress,&isSuspect);

					importThunk->suspect = isSuspect;
					importThunk->valid = true;
					importThunk->apiAddressVA = firstUnresImp->InvalidApiAddress;
					importThunk->hint = (WORD)apiInfo->hint;
					importThunk->ordinal = apiInfo->ordinal;
					strcpy_s(importThunk->name, MAX_PATH,apiInfo->name);
					wcscpy_s(importThunk->moduleName, MAX_PATH, apiInfo->module->getFilename());

					if (moduleThunk->moduleName[0] == TEXT('?'))
					{
						wcscpy_s(moduleThunk->moduleName, MAX_PATH, apiInfo->module->getFilename());
					}
				}
				
				firstUnresImp++;
			}

			iterator2++;
		}

		iterator1++;
	}
}
