#pragma once

#include <windows.h>
#include <vector>
#include "DumpSectionGui.h"

class PeFileSection {
public:
	IMAGE_SECTION_HEADER sectionHeader;
	BYTE * data;
	DWORD dataSize;
	DWORD normalSize;

	PeFileSection()
	{
		ZeroMemory(&sectionHeader, sizeof(IMAGE_SECTION_HEADER));
		data = 0;
		dataSize = 0;
		normalSize = 0;
	}
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
	std::vector<PeFileSection> & getSectionHeaderList();

	bool hasExportDirectory();
	bool hasTLSDirectory();
	bool hasRelocationDirectory();
	bool hasOverlayData();

	DWORD getEntryPoint();

	bool getSectionNameUnicode(const int sectionIndex, WCHAR * output, const int outputLen);

	DWORD getSectionHeaderBasedFileSize();
	DWORD getSectionHeaderBasedSizeOfImage();

	bool readPeSectionsFromProcess();
	bool readPeSectionsFromFile();
	bool savePeFileToDisk(const WCHAR * newFile);
	void removeDosStub();
	void alignAllSectionHeaders();
	void fixPeHeader();
	void setDefaultFileAlignment();
	bool dumpProcess(DWORD_PTR modBase, DWORD_PTR entryPoint, const WCHAR * dumpFilePath);
	bool dumpProcess(DWORD_PTR modBase, DWORD_PTR entryPoint, const WCHAR * dumpFilePath, std::vector<PeSection> & sectionList);
protected:
	PeParser();


	static const DWORD FileAlignmentConstant = 0x200;

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
	
	DWORD isMemoryNotNull( BYTE * data, int dataSize );
	bool openWriteFileHandle( const WCHAR * newFile );
	bool writeZeroMemoryToFile(HANDLE hFile, DWORD fileOffset, DWORD size);

	bool readPeSectionFromFile( DWORD readOffset, PeFileSection & peFileSection );
	bool readPeSectionFromProcess( DWORD_PTR readOffset, PeFileSection & peFileSection );

	bool readSectionFromFile( DWORD readOffset, PeFileSection & peFileSection );
	bool readSectionFromProcess( DWORD_PTR readOffset, PeFileSection & peFileSection );

	bool addNewLastSection(const CHAR * sectionName, DWORD sectionSize, BYTE * sectionData);
	DWORD alignValue(DWORD badValue, DWORD alignTo);
	DWORD_PTR convertOffsetToRVAVector(DWORD_PTR dwOffset);
	DWORD_PTR convertRVAToOffsetVector(DWORD_PTR dwRVA);
	void setNumberOfSections(WORD numberOfSections);
	
	void removeIatDirectory();
	void setEntryPointVa( DWORD_PTR entryPoint );
	bool getFileOverlay();
};

