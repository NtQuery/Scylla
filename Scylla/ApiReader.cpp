
#include "ApiReader.h"

#include "Scylla.h"
#include "Architecture.h"
#include "SystemInformation.h"
#include "StringConversion.h"

std::unordered_map<DWORD_PTR, ApiInfo *> ApiReader::apiList; //api look up table
std::map<DWORD_PTR, ImportModuleThunk> *  ApiReader::moduleThunkList; //store found apis

DWORD_PTR ApiReader::minApiAddress = 0xFFFFFFFF;
DWORD_PTR ApiReader::maxApiAddress = 0;

//#define DEBUG_COMMENTS

void ApiReader::readApisFromModuleList()
{
	for (unsigned int i = 0; i < moduleList.size();i++)
	{
		setModulePriority(&moduleList[i]);

		if (moduleList[i].modBaseAddr + moduleList[i].modBaseSize > maxValidAddress)
		{
			maxValidAddress = moduleList[i].modBaseAddr + moduleList[i].modBaseSize;
		}

		Scylla::windowLog.log(L"Module parsing: %s",moduleList[i].fullPath);

		if (!moduleList[i].isAlreadyParsed)
		{
			parseModule(&moduleList[i]);
		}
	}

#ifdef DEBUG_COMMENTS
	Scylla::debugLog.log(L"Address Min " PRINTF_DWORD_PTR_FULL L" Max " PRINTF_DWORD_PTR_FULL L"\nimagebase " PRINTF_DWORD_PTR_FULL L" maxValidAddress " PRINTF_DWORD_PTR_FULL, minApiAddress, maxApiAddress, targetImageBase ,maxValidAddress);
#endif
}

void ApiReader::parseModule(ModuleInfo *module)
{
	module->parsing = true;

	if (isWinSxSModule(module))
	{
		parseModuleWithMapping(module);
	}
	else if (isModuleLoadedInOwnProcess(module))
	{
		parseModuleWithOwnProcess(module);
	}
	else
	{
		parseModuleWithProcess(module);
	}
	
	module->isAlreadyParsed = true;
}

void ApiReader::parseModuleWithMapping(ModuleInfo *moduleInfo)
{
	LPVOID fileMapping = 0;
	PIMAGE_NT_HEADERS pNtHeader = 0;
	PIMAGE_DOS_HEADER pDosHeader = 0;

	fileMapping = createFileMappingViewRead(moduleInfo->fullPath);

	if (fileMapping == 0)
		return;

	pDosHeader = (PIMAGE_DOS_HEADER)fileMapping;
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)fileMapping + (DWORD_PTR)(pDosHeader->e_lfanew));

	if (isPeAndExportTableValid(pNtHeader))
	{
		parseExportTable(moduleInfo, pNtHeader, (PIMAGE_EXPORT_DIRECTORY)((DWORD_PTR)fileMapping + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), (DWORD_PTR)fileMapping);
	}


	UnmapViewOfFile(fileMapping);

}

inline bool ApiReader::isApiForwarded(DWORD_PTR rva, PIMAGE_NT_HEADERS pNtHeader)
{
	if ((rva > pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress) && (rva < (pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size)))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ApiReader::handleForwardedApi(DWORD_PTR vaStringPointer,char * functionNameParent, DWORD_PTR rvaParent, WORD ordinalParent, ModuleInfo *moduleParent)
{
	size_t dllNameLength = 0;
	WORD ordinal = 0;
	ModuleInfo *module = 0;
	DWORD_PTR vaApi = 0;
	DWORD_PTR rvaApi = 0;
	char dllName[100] = {0};
	WCHAR dllNameW[100] = {0};
	char *fordwardedString = (char *)vaStringPointer;
	char *searchFunctionName = strchr(fordwardedString, '.');
	

	if (!searchFunctionName)
		return;

	dllNameLength = searchFunctionName - fordwardedString;

	if (dllNameLength >= 99)
	{
		return;
	}
	else
	{
		strncpy_s(dllName, fordwardedString, dllNameLength);
	}

	searchFunctionName++;

	//Windows 7
	if (!strncmp(dllName,"api-ms-win-", 11))
	{
		/* 
		    Info: http://www.nirsoft.net/articles/windows_7_kernel_architecture_changes.html
		*/

		FARPROC addy = GetProcAddress(GetModuleHandleA(dllName), searchFunctionName);

		if (addy != 0)
		{
			addApi(functionNameParent,0, ordinalParent, (DWORD_PTR)addy, (DWORD_PTR)addy - (DWORD_PTR)GetModuleHandleA(dllName), true, moduleParent);
		}

		return;
	}

	strcat_s(dllName, ".dll");
	
	StringConversion::ToUTF16(dllName, dllNameW, _countof(dllNameW));
	
	if (!_wcsicmp(dllNameW, moduleParent->getFilename()))
	{
		module = moduleParent;
	}
	else
	{
		module = findModuleByName(dllNameW);
	}

	if (module != 0) // module == 0 -> can be ignored
	{
		/*if ((module->isAlreadyParsed == false) && (module != moduleParent))
		{
			//do API extract
			
			if (module->parsing == true)
			{
				//some stupid circle dependency
				printf("stupid circle dependency %s\n",module->getFilename());
			}
			else
			{
				parseModule(module);
			}
		}*/

		if (strchr(searchFunctionName,'#'))
		{
			//forwarding by ordinal
			searchFunctionName++;
			ordinal = (WORD)atoi(searchFunctionName);
			findApiByModuleAndOrdinal(module, ordinal, &vaApi, &rvaApi);
		}
		else
		{
			findApiByModuleAndName(module, searchFunctionName, &vaApi, &rvaApi);
		}

		if (rvaApi == 0)
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"handleForwardedApi :: Api not found, this is really BAD! %S",fordwardedString);
#endif
		}
		else
		{
			addApi(functionNameParent,0, ordinalParent, vaApi, rvaApi, true, moduleParent);
		}
	}

}

ModuleInfo * ApiReader::findModuleByName(WCHAR *name)
{
	for (unsigned int i = 0; i < moduleList.size(); i++) {
		if (!_wcsicmp(moduleList[i].getFilename(), name))
		{
			return &moduleList[i];
		}
	}

	return 0;
}

void ApiReader::addApiWithoutName(WORD ordinal, DWORD_PTR va, DWORD_PTR rva,bool isForwarded, ModuleInfo *moduleInfo)
{
	addApi(0, 0, ordinal, va, rva, isForwarded, moduleInfo);
}

void ApiReader::addApi(char *functionName, WORD hint, WORD ordinal, DWORD_PTR va, DWORD_PTR rva, bool isForwarded, ModuleInfo *moduleInfo)
{
	ApiInfo *apiInfo = new ApiInfo();

	if ((functionName != 0) && (strlen(functionName) < _countof(apiInfo->name)))
	{
		strcpy_s(apiInfo->name, functionName);
	}
	else
	{
		apiInfo->name[0] = 0x00;
	}

	apiInfo->ordinal = ordinal;
	apiInfo->isForwarded = isForwarded;
	apiInfo->module = moduleInfo;
	apiInfo->rva = rva;
	apiInfo->va = va;
	apiInfo->hint = hint;

	setMinMaxApiAddress(va);

	moduleInfo->apiList.push_back(apiInfo);

	apiList.insert(API_Pair(va, apiInfo));
}

BYTE * ApiReader::getHeaderFromProcess(ModuleInfo * module)
{
	BYTE *bufferHeader = 0;
	DWORD readSize = 0;

	if (module->modBaseSize < PE_HEADER_BYTES_COUNT)
	{
		readSize = module->modBaseSize;
	}
	else
	{
		readSize = PE_HEADER_BYTES_COUNT;
	}

	bufferHeader = new BYTE[readSize];

	if(!readMemoryFromProcess(module->modBaseAddr, readSize, bufferHeader))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"getHeaderFromProcess :: Error reading header");
#endif
		delete[] bufferHeader;
		return 0;
	}
	else
	{
		return bufferHeader;
	}
}

BYTE * ApiReader::getExportTableFromProcess(ModuleInfo * module, PIMAGE_NT_HEADERS pNtHeader)
{
	DWORD readSize = 0;
	BYTE *bufferExportTable = 0;

	readSize = pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	if (readSize < (sizeof(IMAGE_EXPORT_DIRECTORY) + 8))
	{
		//Something is wrong with the PE Header
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"Something is wrong with the PE Header here Export table size %d", readSize);
#endif
		readSize = sizeof(IMAGE_EXPORT_DIRECTORY) + 100;
	}

	if (readSize)
	{
		bufferExportTable = new BYTE[readSize];

		if(!readMemoryFromProcess(module->modBaseAddr + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress, readSize, bufferExportTable))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getExportTableFromProcess :: Error reading export table from process");
#endif
			delete[] bufferExportTable;
			return 0;
		}
		else
		{
			return bufferExportTable;
		}
	}
	else
	{
		return 0;
	}
}

void ApiReader::parseModuleWithProcess(ModuleInfo * module)
{
	PIMAGE_NT_HEADERS pNtHeader = 0;
	PIMAGE_DOS_HEADER pDosHeader = 0;
	BYTE *bufferHeader = 0;
	BYTE *bufferExportTable = 0;


	bufferHeader = getHeaderFromProcess(module);

	if (bufferHeader == 0)
		return;

	pDosHeader = (PIMAGE_DOS_HEADER)bufferHeader;
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)bufferHeader + (DWORD_PTR)(pDosHeader->e_lfanew));

	if (isPeAndExportTableValid(pNtHeader))
	{
		bufferExportTable = getExportTableFromProcess(module, pNtHeader);

		if(bufferExportTable)
		{
			parseExportTable(module,pNtHeader,(PIMAGE_EXPORT_DIRECTORY)bufferExportTable, (DWORD_PTR)bufferExportTable - pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
			delete[] bufferExportTable;
		}
	}

	delete[] bufferHeader;
}

void ApiReader::parseExportTable(ModuleInfo *module, PIMAGE_NT_HEADERS pNtHeader, PIMAGE_EXPORT_DIRECTORY pExportDir, DWORD_PTR deltaAddress)
{
	DWORD *addressOfFunctionsArray = 0,*addressOfNamesArray = 0;
	WORD *addressOfNameOrdinalsArray = 0;
	char *functionName = 0;
	DWORD_PTR RVA = 0, VA = 0;
	WORD ordinal = 0;
	WORD i = 0, j = 0;
	bool withoutName;


	addressOfFunctionsArray = (DWORD *)((DWORD_PTR)pExportDir->AddressOfFunctions + deltaAddress);
	addressOfNamesArray = (DWORD *)((DWORD_PTR)pExportDir->AddressOfNames + deltaAddress);
	addressOfNameOrdinalsArray = (WORD *)((DWORD_PTR)pExportDir->AddressOfNameOrdinals + deltaAddress);

#ifdef DEBUG_COMMENTS
	Scylla::debugLog.log(L"parseExportTable :: module %s NumberOfNames %X", module->fullPath, pExportDir->NumberOfNames);
#endif

	for (i = 0; i < pExportDir->NumberOfNames; i++)
	{
		functionName = (char*)(addressOfNamesArray[i] + deltaAddress);
		ordinal = (WORD)(addressOfNameOrdinalsArray[i] + pExportDir->Base);
		RVA = addressOfFunctionsArray[addressOfNameOrdinalsArray[i]];
		VA = addressOfFunctionsArray[addressOfNameOrdinalsArray[i]] + module->modBaseAddr;

#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"parseExportTable :: api %S ordinal %d imagebase " PRINTF_DWORD_PTR_FULL L" RVA " PRINTF_DWORD_PTR_FULL L" VA " PRINTF_DWORD_PTR_FULL, functionName, ordinal, module->modBaseAddr, RVA, VA);
#endif
		if (!isApiBlacklisted(functionName))
		{
			if (!isApiForwarded(RVA,pNtHeader))
			{
				addApi(functionName, i, ordinal,VA,RVA,false,module);
			}
			else
			{
				//printf("Forwarded: %s\n",functionName);
				handleForwardedApi(RVA + deltaAddress,functionName,RVA,ordinal,module);
			}
		}

	}

	/*Exports without name*/
	if (pExportDir->NumberOfNames != pExportDir->NumberOfFunctions)
	{
		for (i = 0; i < pExportDir->NumberOfFunctions; i++)
		{
			withoutName = true;
			for (j = 0; j < pExportDir->NumberOfNames; j++)
			{
				if(addressOfNameOrdinalsArray[j] == i)
				{
					withoutName = false;
					break;
				}
			}
			if (withoutName && addressOfFunctionsArray[i] != 0)
			{
				ordinal = (WORD)(i+pExportDir->Base);
				RVA = addressOfFunctionsArray[i];
				VA = (addressOfFunctionsArray[i] + module->modBaseAddr);


				if (!isApiForwarded(RVA,pNtHeader))
				{
					addApiWithoutName(ordinal,VA,RVA,false,module);
				}
				else
				{
					handleForwardedApi(RVA + deltaAddress,0,RVA,ordinal,module);
				}

			}
		}
	}
}

void ApiReader::findApiByModuleAndOrdinal(ModuleInfo * module, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi)
{
	findApiByModule(module,0,ordinal,vaApi,rvaApi);
}

void ApiReader::findApiByModuleAndName(ModuleInfo * module, char * searchFunctionName, DWORD_PTR * vaApi, DWORD_PTR * rvaApi)
{
	findApiByModule(module,searchFunctionName,0,vaApi,rvaApi);
}

void ApiReader::findApiByModule(ModuleInfo * module, char * searchFunctionName, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi)
{
	if (isModuleLoadedInOwnProcess(module))
	{
		HMODULE hModule = GetModuleHandle(module->getFilename());

		if (hModule)
		{
			if (ordinal)
			{
				*vaApi = (DWORD_PTR)GetProcAddress(hModule, (LPCSTR)ordinal);
			}
			else
			{
				*vaApi = (DWORD_PTR)GetProcAddress(hModule, searchFunctionName);
			}
			

			if (vaApi)
			{
				*rvaApi = (*vaApi) - (DWORD_PTR)hModule;
				*vaApi = (*rvaApi) + module->modBaseAddr;
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"findApiByModule :: vaApi == NULL, should never happen %S", searchFunctionName);
#endif
			}
		}
		else
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"findApiByModule :: hModule == NULL, should never happen %s", module->getFilename());
#endif
		}
	}
	else
	{
		//search api in extern process
		findApiInProcess(module,searchFunctionName,ordinal,vaApi,rvaApi);
	}
}

bool ApiReader::isModuleLoadedInOwnProcess(ModuleInfo * module)
{
	for (unsigned int i = 0; i < ownModuleList.size(); i++)
	{
		if (!_wcsicmp(module->fullPath, ownModuleList[i].fullPath))
		{
			//printf("isModuleLoadedInOwnProcess :: %s %s\n",module->fullPath,ownModuleList[i].fullPath);
			return true;
		}
	}
	return false;
}

void ApiReader::parseModuleWithOwnProcess( ModuleInfo * module )
{
	PIMAGE_NT_HEADERS pNtHeader = 0;
	PIMAGE_DOS_HEADER pDosHeader = 0;
	HMODULE hModule = GetModuleHandle(module->getFilename());

	if (hModule)
	{
		pDosHeader = (PIMAGE_DOS_HEADER)hModule;
		pNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)hModule + (DWORD_PTR)(pDosHeader->e_lfanew));

		if (isPeAndExportTableValid(pNtHeader))
		{
			parseExportTable(module, pNtHeader, (PIMAGE_EXPORT_DIRECTORY)((DWORD_PTR)hModule + pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress), (DWORD_PTR)hModule);
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"parseModuleWithOwnProcess :: hModule is NULL");
#endif
	}
}

bool ApiReader::isPeAndExportTableValid(PIMAGE_NT_HEADERS pNtHeader)
{
	if (pNtHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		Scylla::windowLog.log(L"-> IMAGE_NT_SIGNATURE doesn't match.");
		return false;
	}
	else if ((pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0) || (pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size == 0))
	{
		Scylla::windowLog.log(L"-> No export table.");
		return false;
	}
	else
	{
		return true;
	}
}

void ApiReader::findApiInProcess(ModuleInfo * module, char * searchFunctionName, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi)
{
	PIMAGE_NT_HEADERS pNtHeader = 0;
	PIMAGE_DOS_HEADER pDosHeader = 0;
	BYTE *bufferHeader = 0;
	BYTE *bufferExportTable = 0;


	bufferHeader = getHeaderFromProcess(module);

	if (bufferHeader == 0)
		return;

	pDosHeader = (PIMAGE_DOS_HEADER)bufferHeader;
	pNtHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)bufferHeader + (DWORD_PTR)(pDosHeader->e_lfanew));

	if (isPeAndExportTableValid(pNtHeader))
	{
		bufferExportTable = getExportTableFromProcess(module, pNtHeader);

		if(bufferExportTable)
		{
			findApiInExportTable(module,(PIMAGE_EXPORT_DIRECTORY)bufferExportTable, (DWORD_PTR)bufferExportTable - pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress,searchFunctionName,ordinal,vaApi,rvaApi);
			delete[] bufferExportTable;
		}
	}

	delete[] bufferHeader;
}

bool ApiReader::findApiInExportTable(ModuleInfo *module, PIMAGE_EXPORT_DIRECTORY pExportDir, DWORD_PTR deltaAddress, char * searchFunctionName, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi)
{
	DWORD *addressOfFunctionsArray = 0,*addressOfNamesArray = 0;
	WORD *addressOfNameOrdinalsArray = 0;
	char *functionName = 0;
	DWORD i = 0, j = 0;

	addressOfFunctionsArray = (DWORD *)((DWORD_PTR)pExportDir->AddressOfFunctions + deltaAddress);
	addressOfNamesArray = (DWORD *)((DWORD_PTR)pExportDir->AddressOfNames + deltaAddress);
	addressOfNameOrdinalsArray = (WORD *)((DWORD_PTR)pExportDir->AddressOfNameOrdinals + deltaAddress);

	if (searchFunctionName)
	{
		for (i = 0; i < pExportDir->NumberOfNames; i++)
		{
			functionName = (char*)(addressOfNamesArray[i] + deltaAddress);

			if (!strcmp(functionName,searchFunctionName))
			{
				*rvaApi = addressOfFunctionsArray[addressOfNameOrdinalsArray[i]];
				*vaApi = addressOfFunctionsArray[addressOfNameOrdinalsArray[i]] + module->modBaseAddr;
				return true;
			}
		}
	}
	else
	{
		for (i = 0; i < pExportDir->NumberOfFunctions; i++)
		{
			if (ordinal == (i+pExportDir->Base))
			{
				*rvaApi = addressOfFunctionsArray[i];
				*vaApi = (addressOfFunctionsArray[i] + module->modBaseAddr);
				return true;
			}
		}
	}

	return false;
}


void ApiReader::setModulePriority(ModuleInfo * module)
{
	const WCHAR *moduleFileName = module->getFilename();

	//imports by kernelbase don't exist
	if (!_wcsicmp(moduleFileName, L"kernelbase.dll"))
	{
		module->priority = -1;
	}
	else if (!_wcsicmp(moduleFileName, L"ntdll.dll"))
	{
		module->priority = 0;
	}
	else if (!_wcsicmp(moduleFileName, L"shlwapi.dll"))
	{
		module->priority = 0;
	}
	else if (!_wcsicmp(moduleFileName, L"ShimEng.dll"))
	{
		module->priority = 0;
	}
	else if (!_wcsicmp(moduleFileName, L"kernel32.dll"))
	{
		module->priority = 2;
	}
	else
	{
		module->priority = 1;
	}
}

bool ApiReader::isApiAddressValid(DWORD_PTR virtualAddress)
{
	return apiList.count(virtualAddress) > 0;
}

ApiInfo * ApiReader::getApiByVirtualAddress(DWORD_PTR virtualAddress, bool * isSuspect)
{
	std::unordered_map<DWORD_PTR, ApiInfo *>::iterator it1, it2;
	size_t c = 0;
	size_t countDuplicates = apiList.count(virtualAddress);
	int countHighPriority = 0;
	ApiInfo *apiFound = 0;


	if (countDuplicates == 0)
	{
		Scylla::windowLog.log(L"getApiByVirtualAddress :: No Api found " PRINTF_DWORD_PTR_FULL, virtualAddress);
		return 0;
	}
	else if (countDuplicates == 1)
	{
		//API is 100% correct
		*isSuspect = false;
		it1 = apiList.find(virtualAddress); // Find first match.
		return (ApiInfo *)((*it1).second);
	}
	else
	{
		it1 = apiList.find(virtualAddress); // Find first match.

		//any high priority with a name
		apiFound = getScoredApi(it1,countDuplicates,true,false,false,true,false,false,false,false);

		if (apiFound)
			return apiFound;

		*isSuspect = true;

		//high priority with a name and ansi/unicode name
		apiFound = getScoredApi(it1,countDuplicates,true,true,false,true,false,false,false,false);

		if (apiFound)
			return apiFound;

		//priority 2 with no underline in name
		apiFound = getScoredApi(it1,countDuplicates,true,false,true,false,false,false,true,false);

		if (apiFound)
			return apiFound;

		//priority 1 with a name
		apiFound = getScoredApi(it1,countDuplicates,true,false,false,false,false,true,false,false);

		if (apiFound)
			return apiFound;

		//With a name
		apiFound = getScoredApi(it1,countDuplicates,true,false,false,false,false,false,false,false);

		if (apiFound)
			return apiFound;

		//any with priority, name, ansi/unicode
		apiFound = getScoredApi(it1,countDuplicates,true,true,false,true,false,false,false,true);

		if (apiFound)
			return apiFound;

		//any with priority
		apiFound = getScoredApi(it1,countDuplicates,false,false,false,true,false,false,false,true);

		if (apiFound)
			return apiFound;

		//has prio 0 and name
		apiFound = getScoredApi(it1,countDuplicates,false,false,false,false,true,false,false,true);

		if (apiFound)
			return apiFound;
	}

	//is never reached
	Scylla::windowLog.log(L"getApiByVirtualAddress :: There is a api resolving bug, VA: " PRINTF_DWORD_PTR_FULL, virtualAddress);
	for (size_t c = 0; c < countDuplicates; c++, it1++)
	{
		apiFound = (ApiInfo *)((*it1).second);
		Scylla::windowLog.log(L"-> Possible API: %S ord: %d ", apiFound->name, apiFound->ordinal);
	}
	return (ApiInfo *) 1; 
}

/*ApiInfo * ApiReader::getApiByVirtualAddress(DWORD_PTR virtualAddress, bool * isSuspect)
{
	std::unordered_map<DWORD_PTR, ApiInfo *>::iterator it1, it2;
	size_t c = 0;
	size_t countDuplicates = apiList.count(virtualAddress);
	int countHighPriority = 0;
	ApiInfo *apiFound = 0;


	if (countDuplicates == 0)
	{
		Scylla::windowLog.log(L"getApiByVirtualAddress :: No Api found " PRINTF_DWORD_PTR_FULL, virtualAddress);
		return 0;
	}
	else if (countDuplicates == 1)
	{
		//API is 100% correct
		*isSuspect = false;
		it1 = apiList.find(virtualAddress); // Find first match.
		return (ApiInfo *)((*it1).second);
	}
	else
	{
		it1 = apiList.find(virtualAddress); // Find first match.
		it2 = it1;
		for (c = 0; c < countDuplicates; c++, it1++)
		{
			apiFound = (ApiInfo *)((*it1).second);
			
			if (apiFound->module->priority >= 1) //1 == high priority
			{
				countHighPriority++;
			}
		}

		it1 = it2;

		
		  This is flawed:
		  It chooses api(prio:1, name:no) over api(prio:0, name:yes)
		  (e.g. SHLWAPI.PathCombineW vs SHELL32.#25)

		  Maybe there should be a check higher up in the code, to see if this API is surrounded
		  by APIs of a DLL and pick the duplicate from that DLL
		 

		if (countHighPriority == 0)
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getApiByVirtualAddress :: countHighPriority == 0 " PRINTF_DWORD_PTR_FULL, virtualAddress);
#endif
			*isSuspect = true;
			return (ApiInfo *)((*it1).second);
		}
		else if (countHighPriority == 1) // what about kernel32, it has priority 2
		{
			//API is 100% correct if countHighPriority == 1 and name export

			*isSuspect = false;
			for (c = 0; c < countDuplicates; c++, it1++)
			{
				apiFound = (ApiInfo *)((*it1).second);

				if (apiFound->module->priority >= 1 && apiFound->name[0] != 0x00) //1 == high priority
				{
					return apiFound;
				}
			}
		}
		//else // fall through for case api1(priority:1, name:false) <> api2(priority:0, name:true)
		{
			//API not 100% correct
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getApiByVirtualAddress :: countHighPriority == %d " PRINTF_DWORD_PTR_FULL, countHighPriority, virtualAddress);
#endif
			*isSuspect = true;
			for (c = 0; c < countDuplicates; c++, it1++)
			{
				apiFound = (ApiInfo *)((*it1).second);
				Scylla::windowLog.log(L"%s - %s %X %X\n",apiFound->name, apiFound->module->getFilename(), apiFound->rva, apiFound->ordinal);
			}
			it1 = it2;

			for (c = 0; c < countDuplicates; c++, it1++)
			{
				apiFound = (ApiInfo *)((*it1).second);

				//prefer APIs with a name
				if (apiFound->module->priority >= 1 && apiFound->name[0] != 0x00) //1 == high priority
				{
					//prefer ANSI/UNICODE APIs
					if (strrchr(apiFound->name, 'W') || strrchr(apiFound->name, 'A'))
					{
						return apiFound;
					}
				}
			}

			it1 = it2;

			for (c = 0; c < countDuplicates; c++, it1++)
			{
				apiFound = (ApiInfo *)((*it1).second);

				//prefer APIs with a name
				if (apiFound->module->priority == 2 && !strrchr(apiFound->name, '_')) //1 == high priority
				{
					return apiFound;
				}
			}

			it1 = it2;

			for (c = 0; c < countDuplicates; c++, it1++)
			{
				apiFound = (ApiInfo *)((*it1).second);
				if (apiFound->module->priority == 1 && apiFound->name[0] != 0x00) //1 == high priority
				{
					return apiFound;
				}
			}

			it1 = it2;

			for (c = 0; c < countDuplicates; c++, it1++)
			{
				apiFound = (ApiInfo *)((*it1).second);

				if (apiFound->module->priority == 1) //1 == high priority
				{
					return apiFound;
				}
			}
		}
	}

	//is never reached
	Scylla::windowLog.log(L"getApiByVirtualAddress :: There is a big bug");
	return (ApiInfo *) 1; 
}*/

ApiInfo * ApiReader::getScoredApi(std::unordered_map<DWORD_PTR, ApiInfo *>::iterator it1,size_t countDuplicates, bool hasName, bool hasUnicodeAnsiName, bool hasNoUnderlineInName, bool hasPrioDll,bool hasPrio0Dll,bool hasPrio1Dll, bool hasPrio2Dll, bool firstWin )
{
	ApiInfo * foundApi = 0;
	ApiInfo * foundMatchingApi = 0;
	int countFoundApis = 0;
	int scoreNeeded = 0;
	int scoreValue = 0;
	size_t apiNameLength = 0;

	if (hasUnicodeAnsiName || hasNoUnderlineInName)
	{
		hasName = true;
	}

	if (hasName)
		scoreNeeded++;

	if (hasUnicodeAnsiName)
		scoreNeeded++;

	if (hasNoUnderlineInName)
		scoreNeeded++;

	if (hasPrioDll)
		scoreNeeded++;

	if (hasPrio0Dll)
		scoreNeeded++;

	if (hasPrio1Dll)
		scoreNeeded++;

	if (hasPrio2Dll)
		scoreNeeded++;

	for (size_t c = 0; c < countDuplicates; c++, it1++)
	{
		foundApi = (ApiInfo *)((*it1).second);
		scoreValue = 0;

		if (hasName)
		{
			if (foundApi->name[0] != 0x00)
			{
				scoreValue++;

				if (hasUnicodeAnsiName)
				{
					apiNameLength = strlen(foundApi->name);

					if ((foundApi->name[apiNameLength - 1] == 'W') || (foundApi->name[apiNameLength - 1] == 'A'))
					{
						scoreValue++;
					}
				}

				if (hasNoUnderlineInName)
				{
					if (!strrchr(foundApi->name, '_'))
					{
						scoreValue++;
					}
				}
			}
		}

		if (hasPrioDll)
		{
			if (foundApi->module->priority >= 1)
			{
				scoreValue++;
			}
		}

		if (hasPrio0Dll)
		{
			if (foundApi->module->priority == 0)
			{
				scoreValue++;
			}
		}

		if (hasPrio1Dll)
		{
			if (foundApi->module->priority == 1)
			{
				scoreValue++;
			}
		}

		if (hasPrio2Dll)
		{
			if (foundApi->module->priority == 2)
			{
				scoreValue++;
			}
		}


		if (scoreValue == scoreNeeded)
		{
			foundMatchingApi = foundApi;
			countFoundApis++;

			if (firstWin)
			{
				return foundMatchingApi;
			}
		}
	}

	if (countFoundApis == 1)
	{
		return foundMatchingApi;
	}
	else
	{
		return (ApiInfo *)0;
	}

}

void ApiReader::setMinMaxApiAddress(DWORD_PTR virtualAddress)
{
	if (virtualAddress < minApiAddress)
	{
		minApiAddress = virtualAddress - 1;
	}
	if (virtualAddress > maxApiAddress)
	{
		maxApiAddress = virtualAddress + 1;
	}
}

void  ApiReader::readAndParseIAT(DWORD_PTR addressIAT, DWORD sizeIAT, std::map<DWORD_PTR, ImportModuleThunk> &moduleListNew)
{
	moduleThunkList = &moduleListNew;
	BYTE *dataIat = new BYTE[sizeIAT];
	if (readMemoryFromProcess(addressIAT,sizeIAT,dataIat))
	{
		parseIAT(addressIAT,dataIat,sizeIAT);
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"ApiReader::readAndParseIAT :: error reading iat " PRINTF_DWORD_PTR_FULL, addressIAT);
#endif
	}

	delete[] dataIat;
}

void ApiReader::parseIAT(DWORD_PTR addressIAT, BYTE * iatBuffer, SIZE_T size)
{
	ApiInfo *apiFound = 0;
	ModuleInfo *module = 0;
	bool isSuspect = false;
	int countApiFound = 0, countApiNotFound = 0;
	DWORD_PTR * pIATAddress = (DWORD_PTR *)iatBuffer;
	SIZE_T sizeIAT = size / sizeof(DWORD_PTR);

	bool foundModuleBreak = false;

	for (SIZE_T i = 0; i < sizeIAT; i++)
	{
		//Scylla::windowLog.log(L"%08X %08X %d von %d", addressIAT + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)iatBuffer, pIATAddress[i],i,sizeIAT);


		if (pIATAddress[i] == 0 || pIATAddress[i] == -1)
		{
			/*if (pIATAddress[i+1] != 0)
			{
				printf("parseIAT :: Module break\n");
			}*/
			/*else
			{
				printf("parseIAT :: IAT finished\n");
				break;
			}*/
			foundModuleBreak = true;
		} 
		else if ( (pIATAddress[i] > minApiAddress) && (pIATAddress[i] < maxApiAddress) )
		{
			
			apiFound = getApiByVirtualAddress(pIATAddress[i], &isSuspect);
			if (apiFound == (ApiInfo *)1)
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"apiFound == (ApiInfo *)1 -> " PRINTF_DWORD_PTR_FULL, pIATAddress[i]);
#endif
			}
			else if (apiFound)
			{
				countApiFound++;
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(PRINTF_DWORD_PTR_FULL L" %s %d %s", apiFound->va, apiFound->module->getFilename(), apiFound->ordinal, apiFound->name);
#endif
				if (module != apiFound->module)
				{
					module = apiFound->module;
					addFoundApiToModuleList(addressIAT + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)iatBuffer, apiFound, true, isSuspect);
				}
				else
				{
					addFoundApiToModuleList(addressIAT + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)iatBuffer, apiFound, false, isSuspect);
				}
				
			}
			else
			{
				countApiNotFound++;
				addNotFoundApiToModuleList(addressIAT + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)iatBuffer, pIATAddress[i]);
				//printf("parseIAT :: API not found %08X\n", pIATAddress[i]);
			}
		}
		else
		{
			//printf("parseIAT :: API not found %08X\n", pIATAddress[i]);
			countApiNotFound++;
			addNotFoundApiToModuleList(addressIAT + (DWORD_PTR)&pIATAddress[i] - (DWORD_PTR)iatBuffer, pIATAddress[i]);
		}
	}

	Scylla::windowLog.log(L"IAT parsing finished, found %d valid APIs, missed %d APIs", countApiFound, countApiNotFound);
}

void ApiReader::addFoundApiToModuleList(DWORD_PTR iatAddressVA, ApiInfo * apiFound, bool isNewModule, bool isSuspect)
{
	if (isNewModule)
	{
		addModuleToModuleList(apiFound->module->getFilename(), iatAddressVA - targetImageBase);
	}
	addFunctionToModuleList(apiFound, iatAddressVA, iatAddressVA - targetImageBase, apiFound->ordinal, true, isSuspect);
}

bool ApiReader::addModuleToModuleList(const WCHAR * moduleName, DWORD_PTR firstThunk)
{
	ImportModuleThunk module;

	module.firstThunk = firstThunk;
	wcscpy_s(module.moduleName, moduleName);

	(*moduleThunkList).insert(std::pair<DWORD_PTR,ImportModuleThunk>(firstThunk,module));

	return true;
}

void ApiReader::addUnknownModuleToModuleList(DWORD_PTR firstThunk)
{
	ImportModuleThunk module;

	module.firstThunk = firstThunk;
	wcscpy_s(module.moduleName, L"?");

	(*moduleThunkList).insert(std::pair<DWORD_PTR,ImportModuleThunk>(firstThunk,module));
}

bool ApiReader::addFunctionToModuleList(ApiInfo * apiFound, DWORD_PTR va, DWORD_PTR rva, WORD ordinal, bool valid, bool suspect)
{
	ImportThunk import;
	ImportModuleThunk  * module = 0;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;

	if ((*moduleThunkList).size() > 1)
	{
		iterator1 = (*moduleThunkList).begin();
		while (iterator1 != (*moduleThunkList).end())
		{
			if (rva >= iterator1->second.firstThunk)
			{
				iterator1++;
				if (iterator1 == (*moduleThunkList).end())
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
				else if (rva < iterator1->second.firstThunk)
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"Error iterator1 != (*moduleThunkList).end()");
#endif
				break;
			}
		}
	}
	else
	{
		iterator1 = (*moduleThunkList).begin();
		module = &(iterator1->second);
	}

	if (!module)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"ImportsHandling::addFunction module not found rva " PRINTF_DWORD_PTR_FULL, rva);
#endif
		return false;
	}


	import.suspect = suspect;
	import.valid = valid;
	import.va = va;
	import.rva = rva;
	import.apiAddressVA = apiFound->va;
	import.ordinal = ordinal;
	import.hint = (WORD)apiFound->hint;

	wcscpy_s(import.moduleName, apiFound->module->getFilename());
	strcpy_s(import.name, apiFound->name);

	module->thunkList.insert(std::pair<DWORD_PTR,ImportThunk>(import.rva, import));

	return true;
}

void ApiReader::clearAll()
{
	minApiAddress = -1;
	maxApiAddress = 0;

	for ( std::unordered_map<DWORD_PTR, ApiInfo *>::iterator it = apiList.begin(); it != apiList.end(); ++it )
	{
		delete it->second;
	}
	apiList.clear();

	if (moduleThunkList != 0)
	{
		(*moduleThunkList).clear();
	}
}

bool ApiReader::addNotFoundApiToModuleList(DWORD_PTR iatAddressVA, DWORD_PTR apiAddress)
{
	ImportThunk import;
	ImportModuleThunk  * module = 0;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	DWORD_PTR rva = iatAddressVA - targetImageBase;

	if ((*moduleThunkList).size() > 0)
	{
		iterator1 = (*moduleThunkList).begin();
		while (iterator1 != (*moduleThunkList).end())
		{
			if (rva >= iterator1->second.firstThunk)
			{
				iterator1++;
				if (iterator1 == (*moduleThunkList).end())
				{
					iterator1--;
					//new unknown module
					if (iterator1->second.moduleName[0] == L'?')
					{
						module = &(iterator1->second);
					}
					else
					{
						addUnknownModuleToModuleList(rva);
						module = &((*moduleThunkList).find(rva)->second);
					}

					break;
				}
				else if (rva < iterator1->second.firstThunk)
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"Error iterator1 != (*moduleThunkList).end()\r\n");
#endif
				break;
			}
		}
	}
	else
	{
		//new unknown module
		addUnknownModuleToModuleList(rva);
		module = &((*moduleThunkList).find(rva)->second);
	}

	if (!module)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"ImportsHandling::addFunction module not found rva " PRINTF_DWORD_PTR_FULL,rva);
#endif
		return false;
	}


	import.suspect = true;
	import.valid = false;
	import.va = iatAddressVA;
	import.rva = rva;
	import.apiAddressVA = apiAddress;
	import.ordinal = 0;

	wcscpy_s(import.moduleName, L"?");
	strcpy_s(import.name, "?");

	module->thunkList.insert(std::pair<DWORD_PTR,ImportThunk>(import.rva, import));

	return true;
}

bool ApiReader::isApiBlacklisted( const char * functionName )
{
	if (SystemInformation::currenOS < WIN_VISTA_32)
	{
		if (!strcmp(functionName, "RestoreLastError"))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}


	/*#ifdef _WIN64
	else if (SystemInformation::currenOS == WIN_XP_64 && !strcmp(functionName, "DecodePointer"))
	{
		return true;
	}
#endif*/
}

bool ApiReader::isWinSxSModule( ModuleInfo * module )
{
	if (wcsstr(module->fullPath, L"\\WinSxS\\"))
	{
		return true;
	}
	else if (wcsstr(module->fullPath, L"\\winsxs\\"))
	{
		return true;
	}
	else
	{
		return false;
	}
}
