#pragma once

#include <windows.h>
#include <vector>

class PeFileSection
{
public:
	BYTE * data;
	DWORD dataSize;
	DWORD normalSize;
};

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

	bool readPeSectionsFromFile();
	bool savePeFileToDisk(const WCHAR * newFile);
	void removeDosStub();

protected:
	PeParser();

	const WCHAR * filename;
	DWORD_PTR moduleBaseAddress;

	/************************************************************************/
	/* PE FILE                                                              */
	/*                                                                      */
	/*  IMAGE_DOS_HEADER      64   0x40                                     */
	/*	IMAGE_NT_HEADERS32   248   0xF8                                     */
	/*	IMAGE_NT_HEADERS64   264  0x108                                     */
	/*	IMAGE_SECTION_HEADER  40   0x28                                     */
	/************************************************************************/

	PIMAGE_DOS_HEADER pDosHeader;
	BYTE * pDosStub; //between dos header and section header
	DWORD dosStubSize;
	PIMAGE_NT_HEADERS32 pNTHeader32;
	PIMAGE_NT_HEADERS64 pNTHeader64;
	std::vector<IMAGE_SECTION_HEADER> listSectionHeaders;
	std::vector<PeFileSection> listPeSection;
	BYTE * overlayData;
	DWORD overlaySize;
	/************************************************************************/

	BYTE * fileMemory;
	BYTE * headerMemory;

	HANDLE hFile;
	DWORD fileSize;

	bool readPeHeaderFromFile(bool readSectionHeaders);
	bool readPeHeaderFromProcess(bool readSectionHeaders);

	bool hasDirectory(const int directoryIndex);
	bool getSectionHeaders();
	void getDosAndNtHeader(BYTE * memory, LONG size);
	DWORD calcCorrectPeHeaderSize( bool readSectionHeaders );
	DWORD getInitialHeaderReadSize( bool readSectionHeaders );
	bool openFileHandle();
	void closeFileHandle();
	void initClass();
	bool readSectionFromFile( DWORD readOffset, DWORD readSize, PeFileSection & peFileSection );
	DWORD isMemoryNotNull( BYTE * data, int dataSize );
	bool openWriteFileHandle( const WCHAR * newFile );
	bool writeZeroMemoryToFile(HANDLE hFile, DWORD fileOffset, DWORD size);
	bool readPeSectionFromFile( DWORD readOffset, PeFileSection & peFileSection );
};

