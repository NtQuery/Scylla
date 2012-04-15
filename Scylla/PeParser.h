#pragma once

#include <windows.h>
#include <vector>

class PeParser
{
public:
	PeParser(const WCHAR * file, bool readSectionHeaders = true);
	PeParser(const DWORD_PTR moduleBase, bool readSectionHeaders = true);

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

	bool getSectionNameUnicode(const int sectionIndex, WCHAR * output, const int outputLen);

	DWORD getSectionHeaderBasedFileSize();

private:
	PeParser();

	const WCHAR * filename;
	DWORD_PTR moduleBaseAddress;

	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_NT_HEADERS32 pNTHeader32;
	PIMAGE_NT_HEADERS64 pNTHeader64;

	std::vector<IMAGE_SECTION_HEADER> listSectionHeaders;

	BYTE * fileMemory;
	BYTE * headerMemory;

	bool readPeHeaderFromFile(bool readSectionHeaders);
	bool readPeHeaderFromProcess(bool readSectionHeaders);
	bool readFileToMemory();

	bool hasDirectory(const int directoryIndex);
	bool getSectionHeaders();
	void getDosAndNtHeader(BYTE * memory, LONG size);
	DWORD calcCorrectPeHeaderSize( bool readSectionHeaders );
	DWORD getInitialHeaderReadSize( bool readSectionHeaders );
};

