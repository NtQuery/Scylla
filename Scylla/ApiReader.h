#pragma once

#include <windows.h>
#include <map>
#include <hash_map>
#include "ProcessAccessHelp.h"
#include "Thunks.h"

typedef std::pair<DWORD_PTR, ApiInfo *> API_Pair;

class ApiReader : public ProcessAccessHelp
{
public:
	static stdext::hash_multimap<DWORD_PTR, ApiInfo *> apiList; //api look up table

	static std::map<DWORD_PTR, ImportModuleThunk> * moduleThunkList; //store found apis

	static DWORD_PTR minApiAddress;
	static DWORD_PTR maxApiAddress;

	/*
	 * Read all APIs from target process
	 */
	void readApisFromModuleList();

	bool isApiAddressValid(DWORD_PTR virtualAddress);
	ApiInfo * getApiByVirtualAddress(DWORD_PTR virtualAddress, bool * isSuspect);
	void readAndParseIAT(DWORD_PTR addressIAT, DWORD sizeIAT, std::map<DWORD_PTR, ImportModuleThunk> &moduleListNew );

	void clearAll();

private:

	void parseIAT(DWORD_PTR addressIAT, BYTE * iatBuffer, SIZE_T size);

	void addApi(char *functionName, WORD hint, WORD ordinal, DWORD_PTR va, DWORD_PTR rva, bool isForwarded, ModuleInfo *moduleInfo);
	void addApiWithoutName(WORD ordinal, DWORD_PTR va, DWORD_PTR rva,bool isForwarded, ModuleInfo *moduleInfo);
	inline bool isApiForwarded(DWORD_PTR rva, PIMAGE_NT_HEADERS pNtHeader);
	void handleForwardedApi(DWORD_PTR vaStringPointer,char *functionNameParent,DWORD_PTR rvaParent, WORD ordinalParent, ModuleInfo *moduleParent);
	void parseModule(ModuleInfo *module);
	void parseModuleWithProcess(ModuleInfo * module);
	
	void parseExportTable(ModuleInfo *module, PIMAGE_NT_HEADERS pNtHeader, PIMAGE_EXPORT_DIRECTORY pExportDir, DWORD_PTR deltaAddress);

	ModuleInfo * findModuleByName(WCHAR *name);

	void findApiByModuleAndOrdinal(ModuleInfo * module, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi);
	void findApiByModuleAndName(ModuleInfo * module, char * searchFunctionName, DWORD_PTR * vaApi, DWORD_PTR * rvaApi);
	void findApiByModule(ModuleInfo * module, char * searchFunctionName, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi);

	bool isModuleLoadedInOwnProcess( ModuleInfo * module );
	void parseModuleWithOwnProcess( ModuleInfo * module );
	bool isPeAndExportTableValid(PIMAGE_NT_HEADERS pNtHeader);
	void findApiInProcess( ModuleInfo * module, char * searchFunctionName, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi );
	bool findApiInExportTable(ModuleInfo *module, PIMAGE_EXPORT_DIRECTORY pExportDir, DWORD_PTR deltaAddress, char * searchFunctionName, WORD ordinal, DWORD_PTR * vaApi, DWORD_PTR * rvaApi);

	BYTE * getHeaderFromProcess(ModuleInfo * module);
	BYTE * getExportTableFromProcess(ModuleInfo * module, PIMAGE_NT_HEADERS pNtHeader);

	void setModulePriority(ModuleInfo * module);
	void setMinMaxApiAddress(DWORD_PTR virtualAddress);
	
	void parseModuleWithMapping(ModuleInfo *moduleInfo); //not used
	void addFoundApiToModuleList(DWORD_PTR iatAddress, ApiInfo * apiFound, bool isNewModule, bool isSuspect);
	bool addModuleToModuleList(const WCHAR * moduleName, DWORD_PTR firstThunk);
	bool addFunctionToModuleList(ApiInfo * apiFound, DWORD_PTR va, DWORD_PTR rva, WORD ordinal, bool valid, bool suspect);
	bool addNotFoundApiToModuleList(DWORD_PTR iatAddressVA, DWORD_PTR apiAddress);
	
	void addUnknownModuleToModuleList(DWORD_PTR firstThunk);
	bool isApiBlacklisted( const char * functionName );
	bool isWinSxSModule( ModuleInfo * module );

	ApiInfo * getScoredApi(stdext::hash_map<DWORD_PTR, ApiInfo *>::iterator it1,size_t countDuplicates, bool hasName, bool hasUnicodeAnsiName, bool hasNoUnderlineInName, bool hasPrioDll,bool hasPrio0Dll,bool hasPrio1Dll, bool hasPrio2Dll, bool firstWin );
};
