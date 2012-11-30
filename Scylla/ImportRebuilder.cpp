
#include "ImportRebuilder.h"
#include "Scylla.h"
#include "StringConversion.h"

//#define DEBUG_COMMENTS


bool ImportRebuilder::rebuildImportTable(const WCHAR * newFilePath, std::map<DWORD_PTR, ImportModuleThunk> & moduleList)
{
	bool retValue = false;

	if (isValidPeFile())
	{
		if (readPeSectionsFromFile())
		{
			setDefaultFileAlignment();

			retValue = buildNewImportTable(moduleList);

			if (retValue)
			{
				alignAllSectionHeaders();
				fixPeHeader();
				retValue = savePeFileToDisk(newFilePath);
			}
		}
	}

	return retValue;
}

bool ImportRebuilder::buildNewImportTable(std::map<DWORD_PTR, ImportModuleThunk> & moduleList)
{
	createNewImportSection(moduleList);

	importSectionIndex = listPeSection.size() - 1;

	DWORD dwSize = fillImportSection(moduleList);

	if (!dwSize)
	{
		return false;
	}

	setFlagToIATSection((*moduleList.begin()).second.firstThunk);

	DWORD vaImportAddress = listPeSection[importSectionIndex].sectionHeader.VirtualAddress;

	if (useOFT)
	{
		//OFT array is at the beginning of the import section
		vaImportAddress += (DWORD)sizeOfOFTArray;
	}

	if (isPE32())
	{
		pNTHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = vaImportAddress;
		pNTHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)(numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR));
	}
	else
	{
		pNTHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = vaImportAddress;
		pNTHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = (DWORD)(numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR));
	}


	return true;
}

bool ImportRebuilder::createNewImportSection(std::map<DWORD_PTR, ImportModuleThunk> & moduleList)
{
	char sectionName[IMAGE_SIZEOF_SHORT_NAME + 1] = {0};

	const WCHAR * sectionNameW = Scylla::config[IAT_SECTION_NAME].getString();

	calculateImportSizes(moduleList);

	if (wcslen(sectionNameW) > IMAGE_SIZEOF_SHORT_NAME)
	{
		strcpy_s(sectionName, ".SCY");
	}
	else
	{
		StringConversion::ToASCII(sectionNameW, sectionName, _countof(sectionName));
	}
	
	return addNewLastSection(sectionName, (DWORD)sizeOfImportSection, 0);
}

void ImportRebuilder::setFlagToIATSection(DWORD_PTR iatAddress)
{
	for (size_t i = 0; i < listPeSection.size(); i++)
	{
		if ((listPeSection[i].sectionHeader.VirtualAddress <= iatAddress) && ((listPeSection[i].sectionHeader.VirtualAddress + listPeSection[i].sectionHeader.Misc.VirtualSize) > iatAddress))
		{
			//section must be read and writeable
			listPeSection[i].sectionHeader.Characteristics |= IMAGE_SCN_MEM_READ|IMAGE_SCN_MEM_WRITE;
		}
	}
}

DWORD ImportRebuilder::fillImportSection(std::map<DWORD_PTR, ImportModuleThunk> & moduleList)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator mapIt;
	std::map<DWORD_PTR, ImportThunk>::iterator mapIt2;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = 0;
	PIMAGE_IMPORT_BY_NAME pImportByName = 0;
	PIMAGE_THUNK_DATA pThunk = 0;
	ImportModuleThunk * importModuleThunk = 0;
	ImportThunk * importThunk = 0;

	size_t stringLength = 0;
	DWORD_PTR lastRVA = 0;

	BYTE * sectionData = listPeSection[importSectionIndex].data;
	DWORD offset;
	DWORD offsetOFTArray = 0;

	if (useOFT)
	{
		//OFT Array is always at the beginning of the import section
		offset = (DWORD)sizeOfOFTArray; //size includes null termination
	}
	else
	{
		offset = 0;
	}

	pImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((DWORD_PTR)sectionData + offset);

	//skip the IMAGE_IMPORT_DESCRIPTOR
	offset += (DWORD)(numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR));

	for ( mapIt = moduleList.begin() ; mapIt != moduleList.end(); mapIt++ )
	{
		importModuleThunk = &((*mapIt).second);

		stringLength = addImportDescriptor(importModuleThunk, offset, offsetOFTArray);

#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"fillImportSection :: importDesc.Name %X", pImportDescriptor->Name);
#endif

		offset += (DWORD)stringLength; //stringLength has null termination char

		pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)sectionData + offset);

		//pThunk = (PIMAGE_THUNK_DATA)(getMemoryPointerFromRVA(importModuleThunk->firstThunk));

		lastRVA = importModuleThunk->firstThunk - sizeof(DWORD_PTR);

		for ( mapIt2 = (*mapIt).second.thunkList.begin() ; mapIt2 != (*mapIt).second.thunkList.end(); mapIt2++ )
		{
			importThunk = &((*mapIt2).second);

			if (useOFT)
			{
				//OFT Array is always at the beginning of the import section
				pThunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)sectionData + offsetOFTArray);
				offsetOFTArray += sizeof(DWORD_PTR); //increase OFT array index
			}
			else
			{
				pThunk = (PIMAGE_THUNK_DATA)(getMemoryPointerFromRVA(importThunk->rva));
			}

			//check wrong iat pointer
			if (!pThunk)
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"fillImportSection :: Failed to get pThunk RVA: %X", importThunk->rva);
#endif
				return 0;
			}

			if ((lastRVA + sizeof(DWORD_PTR)) != importThunk->rva)
			{
				//add additional import desc
				addSpecialImportDescriptor(importThunk->rva, offsetOFTArray);
				if (useOFT)
				{
					pThunk = (PIMAGE_THUNK_DATA)((DWORD_PTR)sectionData + offsetOFTArray);
					offsetOFTArray += sizeof(DWORD_PTR); //increase OFT array index, next module
				}				
			}
			lastRVA = importThunk->rva;

#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"fillImportSection :: importThunk %X pThunk %X pImportByName %X offset %X", importThunk,pThunk,pImportByName,offset);
#endif
			stringLength = addImportToImportTable(importThunk, pThunk, pImportByName, offset);

			offset += (DWORD)stringLength; //is 0 bei import by ordinal
			pImportByName = (PIMAGE_IMPORT_BY_NAME)((DWORD_PTR)pImportByName + stringLength);
		}

		offsetOFTArray += sizeof(DWORD_PTR); //increase OFT array index, next module
		pImportDescriptor++;
	}

	return offset;
}

size_t ImportRebuilder::addImportDescriptor(ImportModuleThunk * pImportModule, DWORD sectionOffset, DWORD sectionOffsetOFTArray)
{
	char dllName[MAX_PATH];

	StringConversion::ToASCII(pImportModule->moduleName, dllName, _countof(dllName));
	size_t stringLength = strlen(dllName) + 1;

	/*
		Warning: stringLength MUST include null termination char
	*/

	memcpy((listPeSection[importSectionIndex].data + sectionOffset), dllName, stringLength); //copy module name to section

	pImportDescriptor->FirstThunk = (DWORD)pImportModule->firstThunk;
	pImportDescriptor->Name = (DWORD)convertOffsetToRVAVector(listPeSection[importSectionIndex].sectionHeader.PointerToRawData + sectionOffset);
	
	if (useOFT)
	{
		pImportDescriptor->OriginalFirstThunk = (DWORD)convertOffsetToRVAVector(listPeSection[importSectionIndex].sectionHeader.PointerToRawData + sectionOffsetOFTArray);
	}

	return stringLength;
}

void ImportRebuilder::addSpecialImportDescriptor(DWORD_PTR rvaFirstThunk, DWORD sectionOffsetOFTArray)
{
	PIMAGE_IMPORT_DESCRIPTOR oldID = pImportDescriptor;
	pImportDescriptor++;

	pImportDescriptor->FirstThunk = (DWORD)rvaFirstThunk;
	pImportDescriptor->Name = oldID->Name;

	if (useOFT)
	{
		pImportDescriptor->OriginalFirstThunk = (DWORD)convertOffsetToRVAVector(listPeSection[importSectionIndex].sectionHeader.PointerToRawData + sectionOffsetOFTArray);
	}
}

void ImportRebuilder::calculateImportSizes(std::map<DWORD_PTR, ImportModuleThunk> & moduleList)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator mapIt;
	std::map<DWORD_PTR, ImportThunk>::iterator mapIt2;
	DWORD_PTR lastRVA = 0;

	numberOfImportDescriptors = 0;
	sizeOfImportSection = 0;
	sizeOfApiAndModuleNames = 0;
	sizeOfOFTArray = 0;

	numberOfImportDescriptors = moduleList.size() + 1; //last is zero'd

	for ( mapIt = moduleList.begin() ; mapIt != moduleList.end(); mapIt++ )
	{
		lastRVA = (*mapIt).second.firstThunk - sizeof(DWORD_PTR);

		sizeOfApiAndModuleNames += (DWORD)(wcslen((*mapIt).second.moduleName) + 1);

		for ( mapIt2 = (*mapIt).second.thunkList.begin() ; mapIt2 != (*mapIt).second.thunkList.end(); mapIt2++ )
		{
			if ((lastRVA + sizeof(DWORD_PTR)) != (*mapIt2).second.rva)
			{
				numberOfImportDescriptors++; //add additional import desc
				sizeOfOFTArray += sizeof(DWORD_PTR) + sizeof(DWORD_PTR);
			}

			if((*mapIt2).second.name[0] != '\0')
			{
				sizeOfApiAndModuleNames += sizeof(WORD); //Hint from IMAGE_IMPORT_BY_NAME
				sizeOfApiAndModuleNames += (DWORD)(strlen((*mapIt2).second.name) + 1);
			}

			//OriginalFirstThunk Array in Import Section: value
			sizeOfOFTArray += sizeof(DWORD_PTR);

			lastRVA = (*mapIt2).second.rva;
		}

		//OriginalFirstThunk Array in Import Section: NULL termination
		sizeOfOFTArray += sizeof(DWORD_PTR);
	}

	sizeOfImportSection = sizeOfOFTArray + sizeOfApiAndModuleNames + (numberOfImportDescriptors * sizeof(IMAGE_IMPORT_DESCRIPTOR));
}

size_t ImportRebuilder::addImportToImportTable( ImportThunk * pImport, PIMAGE_THUNK_DATA pThunk, PIMAGE_IMPORT_BY_NAME pImportByName, DWORD sectionOffset)
{
	size_t stringLength = 0;

	if(pImport->name[0] == '\0')
	{
		pThunk->u1.AddressOfData = (IMAGE_ORDINAL(pImport->ordinal) | IMAGE_ORDINAL_FLAG);
	}
	else
	{
		pImportByName->Hint = pImport->hint;

		stringLength = strlen(pImport->name) + 1;
		memcpy(pImportByName->Name, pImport->name, stringLength);

		pThunk->u1.AddressOfData = convertOffsetToRVAVector(listPeSection[importSectionIndex].sectionHeader.PointerToRawData + sectionOffset);

		if (!pThunk->u1.AddressOfData)
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"addImportToImportTable :: failed to get AddressOfData %X %X", listPeSection[importSectionIndex].sectionHeader.PointerToRawData, sectionOffset);
#endif
		}

		//next import should be nulled
		pThunk++;
		pThunk->u1.AddressOfData = 0;

#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"addImportToImportTable :: pThunk->u1.AddressOfData %X %X %X", pThunk->u1.AddressOfData, pThunk, listPeSection[importSectionIndex].sectionHeader.PointerToRawData + sectionOffset);
#endif
		stringLength += sizeof(WORD);
	}

	return stringLength;
}

BYTE * ImportRebuilder::getMemoryPointerFromRVA(DWORD_PTR dwRVA)
{
	int peSectionIndex = convertRVAToOffsetVectorIndex(dwRVA);

	if (peSectionIndex == -1)
	{
		return 0;
	}

	DWORD rvaPointer = ((DWORD)dwRVA - listPeSection[peSectionIndex].sectionHeader.VirtualAddress);
	DWORD minSectionSize = rvaPointer + (sizeof(DWORD_PTR) * 2); //add space for 1 IAT address

	if (listPeSection[peSectionIndex].data == 0 || listPeSection[peSectionIndex].dataSize == 0)
	{
		listPeSection[peSectionIndex].dataSize = minSectionSize; 
		listPeSection[peSectionIndex].normalSize = minSectionSize;
		listPeSection[peSectionIndex].data = new BYTE[listPeSection[peSectionIndex].dataSize];

		listPeSection[peSectionIndex].sectionHeader.SizeOfRawData = listPeSection[peSectionIndex].dataSize;
	}
	else if(listPeSection[peSectionIndex].dataSize < minSectionSize)
	{
		BYTE * temp = new BYTE[minSectionSize];
		memcpy(temp, listPeSection[peSectionIndex].data, listPeSection[peSectionIndex].dataSize);
		delete [] listPeSection[peSectionIndex].data;

		listPeSection[peSectionIndex].data = temp;
		listPeSection[peSectionIndex].dataSize = minSectionSize;
		listPeSection[peSectionIndex].normalSize = minSectionSize;

		listPeSection[peSectionIndex].sectionHeader.SizeOfRawData = listPeSection[peSectionIndex].dataSize;
	}

    return (BYTE *)((DWORD_PTR)listPeSection[peSectionIndex].data + rvaPointer);
}

void ImportRebuilder::enableOFTSupport()
{
	useOFT = true;
}

