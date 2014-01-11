#include <windows.h>
#include "PeParser.h"
#include "ProcessAccessHelp.h"
#include "Scylla.h"
#include "Architecture.h"
#include "FunctionExport.h"
#include "ProcessLister.h"
#include "ApiReader.h"
#include "IATSearch.h"
#include "ImportRebuilder.h"


extern HINSTANCE hDllModule;

const WCHAR * WINAPI ScyllaVersionInformationW()
{
	return APPNAME L" " ARCHITECTURE L" " APPVERSION;
}

const char * WINAPI ScyllaVersionInformationA()
{
	return APPNAME_S " " ARCHITECTURE_S " " APPVERSION_S;
}

DWORD WINAPI ScyllaVersionInformationDword()
{
	return APPVERSIONDWORD;
}

BOOL DumpProcessW(const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult)
{
	PeParser * peFile = 0;

	if (fileToDump)
	{
		peFile = new PeParser(fileToDump, true);
	}
	else
	{
		peFile = new PeParser(imagebase, true);
	}

	return peFile->dumpProcess(imagebase, entrypoint, fileResult);
}

BOOL WINAPI ScyllaRebuildFileW(const WCHAR * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum, BOOL createBackup)
{

	if (createBackup)
	{
		if (!ProcessAccessHelp::createBackupFile(fileToRebuild))
		{
			return FALSE;
		}
	}

	PeParser peFile(fileToRebuild, true);
	if (peFile.readPeSectionsFromFile())
	{
		peFile.setDefaultFileAlignment();
		if (removeDosStub)
		{
			peFile.removeDosStub();
		}
		peFile.alignAllSectionHeaders();
		peFile.fixPeHeader();

		if (peFile.savePeFileToDisk(fileToRebuild))
		{
			if (updatePeHeaderChecksum)
			{
				PeParser::updatePeHeaderChecksum(fileToRebuild, (DWORD)ProcessAccessHelp::getFileSize(fileToRebuild));
			}
			return TRUE;
		}
	}

	return FALSE;
}

BOOL WINAPI ScyllaRebuildFileA(const char * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum, BOOL createBackup)
{
	WCHAR fileToRebuildW[MAX_PATH];
	if (MultiByteToWideChar(CP_ACP, 0, fileToRebuild, -1, fileToRebuildW, _countof(fileToRebuildW)) == 0)
	{
		return FALSE;
	}

	return ScyllaRebuildFileW(fileToRebuildW, removeDosStub, updatePeHeaderChecksum, createBackup);
}

BOOL WINAPI ScyllaDumpCurrentProcessW(const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult)
{
	ProcessAccessHelp::setCurrentProcessAsTarget();

	return DumpProcessW(fileToDump, imagebase, entrypoint, fileResult);
}

BOOL WINAPI ScyllaDumpProcessW(DWORD_PTR pid, const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult)
{
	if (ProcessAccessHelp::openProcessHandle((DWORD)pid))
	{
		return DumpProcessW(fileToDump, imagebase, entrypoint, fileResult);
	}
	else
	{
		return FALSE;
	}	
}

BOOL WINAPI ScyllaDumpCurrentProcessA(const char * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const char * fileResult)
{
	WCHAR fileToDumpW[MAX_PATH];
	WCHAR fileResultW[MAX_PATH];

	if (fileResult == 0)
	{
		return FALSE;
	}

	if (MultiByteToWideChar(CP_ACP, 0, fileResult, -1, fileResultW, _countof(fileResultW)) == 0)
	{
		return FALSE;
	}

	if (fileToDump != 0)
	{
		if (MultiByteToWideChar(CP_ACP, 0, fileToDump, -1, fileToDumpW, _countof(fileToDumpW)) == 0)
		{
			return FALSE;
		}

		return ScyllaDumpCurrentProcessW(fileToDumpW, imagebase, entrypoint, fileResultW);
	}
	else
	{
		return ScyllaDumpCurrentProcessW(0, imagebase, entrypoint, fileResultW);
	}
}

BOOL WINAPI ScyllaDumpProcessA(DWORD_PTR pid, const char * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const char * fileResult)
{
	WCHAR fileToDumpW[MAX_PATH];
	WCHAR fileResultW[MAX_PATH];

	if (fileResult == 0)
	{
		return FALSE;
	}

	if (MultiByteToWideChar(CP_ACP, 0, fileResult, -1, fileResultW, _countof(fileResultW)) == 0)
	{
		return FALSE;
	}

	if (fileToDump != 0)
	{
		if (MultiByteToWideChar(CP_ACP, 0, fileToDump, -1, fileToDumpW, _countof(fileToDumpW)) == 0)
		{
			return FALSE;
		}

		return ScyllaDumpProcessW(pid, fileToDumpW, imagebase, entrypoint, fileResultW);
	}
	else
	{
		return ScyllaDumpProcessW(pid, 0, imagebase, entrypoint, fileResultW);
	}
}

INT WINAPI ScyllaStartGui(DWORD dwProcessId, HINSTANCE mod)
{
	GUI_DLL_PARAMETER guiParam;
	guiParam.dwProcessId = dwProcessId;
	guiParam.mod = mod;

	return InitializeGui(hDllModule, (LPARAM)&guiParam);
}

int WINAPI ScyllaIatSearch(DWORD dwProcessId, DWORD_PTR * iatStart, DWORD * iatSize, DWORD_PTR searchStart, BOOL advancedSearch)
{
	ApiReader apiReader;
	ProcessLister processLister;
	Process *processPtr = 0;
	IATSearch iatSearch;

	std::vector<Process>& processList = processLister.getProcessListSnapshot();
	for(std::vector<Process>::iterator it = processList.begin(); it != processList.end(); ++it)
	{
		if(it->PID == dwProcessId)
		{
			processPtr = &(*it);
			break;
		}
	}

	if(!processPtr) return SCY_ERROR_PIDNOTFOUND;

	ProcessAccessHelp::closeProcessHandle();
	apiReader.clearAll();

	if (!ProcessAccessHelp::openProcessHandle(processPtr->PID))
	{
		return SCY_ERROR_PROCOPEN;
	}

	ProcessAccessHelp::getProcessModules(processPtr->PID, ProcessAccessHelp::moduleList);

	ProcessAccessHelp::selectedModule = 0;
	ProcessAccessHelp::targetImageBase = processPtr->imageBase;
	ProcessAccessHelp::targetSizeOfImage = ProcessAccessHelp::getSizeOfImageProcess(ProcessAccessHelp::hProcess, ProcessAccessHelp::targetImageBase);

	apiReader.readApisFromModuleList();

	int retVal = SCY_ERROR_IATNOTFOUND;

	if (advancedSearch)
	{
		if (iatSearch.searchImportAddressTableInProcess(searchStart, iatStart, iatSize, true))
		{
			retVal = SCY_ERROR_SUCCESS;
		}
	}
	else
	{
		if (iatSearch.searchImportAddressTableInProcess(searchStart, iatStart, iatSize, false))
		{
			retVal = SCY_ERROR_SUCCESS;
		}
	}

	processList.clear();
	ProcessAccessHelp::closeProcessHandle();
	apiReader.clearAll();

	return retVal;
}


int WINAPI ScyllaIatFixAutoW(DWORD_PTR iatAddr, DWORD iatSize, DWORD dwProcessId, const WCHAR * dumpFile, const WCHAR * iatFixFile)
{
	ApiReader apiReader;
	ProcessLister processLister;
	Process *processPtr = 0;
	std::map<DWORD_PTR, ImportModuleThunk> moduleList;

	std::vector<Process>& processList = processLister.getProcessListSnapshot();
	for(std::vector<Process>::iterator it = processList.begin(); it != processList.end(); ++it)
	{
		if(it->PID == dwProcessId)
		{
			processPtr = &(*it);
			break;
		}
	}

	if(!processPtr) return SCY_ERROR_PIDNOTFOUND;

	ProcessAccessHelp::closeProcessHandle();
	apiReader.clearAll();

	if (!ProcessAccessHelp::openProcessHandle(processPtr->PID))
	{
		return SCY_ERROR_PROCOPEN;
	}

	ProcessAccessHelp::getProcessModules(processPtr->PID, ProcessAccessHelp::moduleList);

	ProcessAccessHelp::selectedModule = 0;
	ProcessAccessHelp::targetImageBase = processPtr->imageBase;
	ProcessAccessHelp::targetSizeOfImage = ProcessAccessHelp::getSizeOfImageProcess(ProcessAccessHelp::hProcess, ProcessAccessHelp::targetImageBase);

	apiReader.readApisFromModuleList();

	apiReader.readAndParseIAT(iatAddr, iatSize, moduleList);

	//add IAT section to dump
	ImportRebuilder importRebuild(dumpFile);
	importRebuild.enableOFTSupport();

	int retVal = SCY_ERROR_IATWRITE;

	if (importRebuild.rebuildImportTable(iatFixFile, moduleList))
	{
		retVal = SCY_ERROR_SUCCESS;
	}

	processList.clear();
	moduleList.clear();
	ProcessAccessHelp::closeProcessHandle();
	apiReader.clearAll();


	return retVal;
}
