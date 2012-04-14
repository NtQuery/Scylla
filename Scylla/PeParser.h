#pragma once

#include <windows.h>
#include <vector>

class PeParser
{
public:
	PeParser(const WCHAR * file, bool readSectionHeaders = true);
	//PeParser(HANDLE hProcess, DWORD_PTR moduleBase, bool readSectionHeaders = true);

	~PeParser();

	bool isValidPeFile();
	bool isPE64();
	bool isPE32();

	bool isTargetFileSamePeFormat();

	WORD getNumberOfSections();
	std::vector<IMAGE_SECTION_HEADER> & getSectionHeaderList();

	bool hasExportDirectory();
	bool hasTLSDirectory();
	bool hasRelocationDirectory();

	DWORD getEntryPoint();

	bool getSectionNameUnicode(const int sectionIndex, WCHAR * output, int outputLen);

private:
	const WCHAR * filename;

	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_NT_HEADERS32 pNTHeader32;
	PIMAGE_NT_HEADERS64 pNTHeader64;

	std::vector<IMAGE_SECTION_HEADER> listSectionHeaders;

	BYTE * fileMemory;
	BYTE * headerMemory;

	bool readPeHeader(bool readSectionHeaders);
	bool readFileToMemory();

	bool hasDirectory(const int directoryIndex);
	bool getSectionHeaders();
	bool readPeHeaderFromProcess( HANDLE hProcess, DWORD_PTR moduleBase );
};

