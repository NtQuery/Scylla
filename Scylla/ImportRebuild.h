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

	size_t numberOfImportDescriptors;
	size_t sizeOfImportSection;
	size_t sizeOfApiAndModuleNames;
	size_t importSectionIndex;

	DWORD getOffsetLastSection();

	void clearAllData();
	void updatePeHeader();
	DWORD fillImportSection( std::map<DWORD_PTR, ImportModuleThunk> & moduleList );
	bool splitTargetFile();

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
	size_t addImportToImportTable( ImportThunk * pImport, PIMAGE_THUNK_DATA pThunk, PIMAGE_IMPORT_BY_NAME pImportByName, DWORD sectionOffset);
	size_t addImportDescriptor(ImportModuleThunk * pImportModule, DWORD sectionOffset);

	void calculateImportSizes(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);

	void addSpecialImportDescriptor(DWORD_PTR rvaFirstThunk);
};