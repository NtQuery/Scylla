#pragma once

#include <map>
#include "PeParser.h"
#include "Thunks.h"


class ImportRebuilder : public PeParser {
public:
	ImportRebuilder(const WCHAR * file) : PeParser(file, true)
	{
		pImportDescriptor = 0;
		pThunkData = 0;
		pImportByName = 0;

		numberOfImportDescriptors = 0;
		sizeOfImportSection = 0;
		sizeOfApiAndModuleNames = 0;
		importSectionIndex = 0;
		useOFT = false;
		sizeOfOFTArray = 0;
	}

	bool rebuildImportTable(const WCHAR * newFilePath, std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	void enableOFTSupport();
private:
	PIMAGE_IMPORT_DESCRIPTOR pImportDescriptor;
	PIMAGE_THUNK_DATA pThunkData;
	PIMAGE_IMPORT_BY_NAME pImportByName;

	size_t numberOfImportDescriptors;
	size_t sizeOfImportSection;
	size_t sizeOfApiAndModuleNames;
	size_t importSectionIndex;

	//OriginalFirstThunk Array in Import Section
	size_t sizeOfOFTArray;
	bool useOFT;

	DWORD fillImportSection(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	BYTE * getMemoryPointerFromRVA(DWORD_PTR dwRVA);

	bool createNewImportSection(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	bool buildNewImportTable(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);
	void setFlagToIATSection(DWORD_PTR iatAddress);
	size_t addImportToImportTable( ImportThunk * pImport, PIMAGE_THUNK_DATA pThunk, PIMAGE_IMPORT_BY_NAME pImportByName, DWORD sectionOffset);
	size_t addImportDescriptor(ImportModuleThunk * pImportModule, DWORD sectionOffset, DWORD sectionOffsetOFTArray);

	void calculateImportSizes(std::map<DWORD_PTR, ImportModuleThunk> & moduleList);

	void addSpecialImportDescriptor(DWORD_PTR rvaFirstThunk, DWORD sectionOffsetOFTArray);
};