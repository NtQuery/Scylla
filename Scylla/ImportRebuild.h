#pragma once

#include "ProcessAccessHelp.h"
#include "Thunks.h"

class ImportRebuild {
public:

	bool rebuildImportTable(const WCHAR * targetFilePath, const WCHAR * newFilePath, std::map<DWORD_PTR, ImportModuleThunk> & moduleList);

	DWORD convertRVAToOffset(DWORD dwRVA);
	DWORD convertOffsetToRVA(DWORD dwOffset);

	bool addNewSection(char * sectionName, DWORD sectionSize, BYTE * sectionData);

	bool writeZeroMemoryToFile(HANDLE hFile, DWORD fileOffset, DWORD size);

private:
	std::vector<IMAGE_SECTION_HEADER> vecSectionHeaders;
	std::vector<BYTE *> vecSectionData;

	BYTE * imageData;
	BYTE * pDosStub;
	PIMAGE_DOS_HEADER pDosHeader;
	PIMAGE_NT_HEADERS pNTHeader;

	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor;
	PIMAGE_THUNK_DATA pThunkData;
	PIMAGE_IMPORT_BY_NAME pImportByName;

	DWORD getOffsetLastSection();

	void clearAllData();
	void updatePeHeader();
	DWORD fillImportSection( std::map<DWORD_PTR, ImportModuleThunk> & moduleList, size_t lastSectionIndex );
	bool splitTargetFile();
	DWORD calculateMinSize(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);

	DWORD convertRVAToOffsetVector(DWORD dwRVA);
	DWORD_PTR convertOffsetToRVAVector(DWORD dwOffset);

	BYTE * getMemoryPointerFromRVA(DWORD_PTR dwRVA);

	DWORD alignValue(DWORD badValue, DWORD alignTo);

	bool alignSectionHeaders();

	bool saveNewFile(const WCHAR * filepath);
	bool loadTargetFile(const WCHAR * filepath);

	bool createNewImportSection(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	bool buildNewImportTable(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	void setFlagToIATSection( DWORD_PTR iatAddress );
};