#pragma once

#include <windows.h>

class PeDump
{
public:

	DWORD_PTR entryPoint; //VA
	DWORD_PTR imageBase;  //VA
	DWORD sizeOfImage;
	WCHAR fullpath[MAX_PATH];

	//Settings
	static bool useHeaderFromDisk;
	static bool appendOverlayData;

	PeDump()
	{
		imageBase = 0;
		sizeOfImage = 0;
		dumpData = 0;
		headerData = 0;
		pNTHeader = 0;
		pDOSHeader = 0;
		pSectionHeader = 0;
	}

	~PeDump()
	{
		if (dumpData != 0)
		{
			delete [] dumpData;
		}

		if (headerData != 0)
		{
			delete [] headerData;
		}
	}

	bool dumpCompleteProcessToDisk(const WCHAR * dumpFilePath);

	bool saveDumpToDisk(const WCHAR * dumpFilePath, BYTE *dumpBuffer, DWORD dumpSize);


	bool copyFileDataFromOffset(const WCHAR * sourceFile, const WCHAR * destFile, DWORD_PTR fileOffset, DWORD dwSize);
	bool appendOverlayDataToDump(const WCHAR * dumpFilePath);
	bool getOverlayData(const WCHAR * filepath, DWORD_PTR * overlayFileOffset, DWORD * overlaySize);

private:

	BYTE * dumpData;
	BYTE * headerData;

	PIMAGE_DOS_HEADER pDOSHeader;
	PIMAGE_NT_HEADERS pNTHeader;
	PIMAGE_SECTION_HEADER pSectionHeader;

	bool validateHeaders();
	bool fillPeHeaderStructs(bool fromDisk);

	void fixDump(BYTE * dumpBuffer);
	void fixBadNtHeaderValues(PIMAGE_NT_HEADERS pNtHead);
	void fixSectionHeaderForDump(PIMAGE_SECTION_HEADER oldSecHead, PIMAGE_SECTION_HEADER newSecHead);
	void fixNtHeaderForDump(PIMAGE_NT_HEADERS oldNtHead, PIMAGE_NT_HEADERS newNtHead);
};
