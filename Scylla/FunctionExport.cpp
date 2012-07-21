#include <windows.h>
#include "PeParser.h"
#include "ProcessAccessHelp.h"

BOOL DumpProcessW(const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult);

BOOL WINAPI ScyllaDumpCurrentProcessW(const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult);
BOOL WINAPI ScyllaDumpCurrentProcessA(const char * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const char * fileResult);

BOOL WINAPI ScyllaDumpProcessW(DWORD_PTR pid, const WCHAR * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const WCHAR * fileResult);
BOOL WINAPI ScyllaDumpProcessA(DWORD_PTR pid, const char * fileToDump, DWORD_PTR imagebase, DWORD_PTR entrypoint, const char * fileResult);

BOOL WINAPI RebuildFileW(const WCHAR * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum);
BOOL WINAPI RebuildFileA(const char * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum);

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

BOOL WINAPI RebuildFileW(const WCHAR * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum)
{
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

BOOL WINAPI RebuildFileA(const char * fileToRebuild, BOOL removeDosStub, BOOL updatePeHeaderChecksum)
{
	WCHAR fileToRebuildW[MAX_PATH];
	if (MultiByteToWideChar(CP_ACP, 0, fileToRebuild, -1, fileToRebuildW, _countof(fileToRebuildW)) == 0)
	{
		return FALSE;
	}

	return RebuildFileW(fileToRebuildW, removeDosStub, updatePeHeaderChecksum);
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

