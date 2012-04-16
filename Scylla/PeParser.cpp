
#include "PeParser.h"
#include "ProcessAccessHelp.h"

PeParser::PeParser()
{
	initClass();
}

PeParser::PeParser(const WCHAR * file, bool readSectionHeaders)
{
	initClass();

	filename = file;

	if (filename && wcslen(filename) > 3)
	{
		readPeHeaderFromFile(readSectionHeaders);

		if (readSectionHeaders)
		{
			if (isValidPeFile())
			{
				getSectionHeaders();
			}
		}
	}
}

PeParser::PeParser(const DWORD_PTR moduleBase, bool readSectionHeaders)
{
	initClass();

	moduleBaseAddress = moduleBase;

	if (moduleBaseAddress)
	{
		readPeHeaderFromProcess(readSectionHeaders);

		if (readSectionHeaders)
		{
			if (isValidPeFile())
			{
				getSectionHeaders();
			}
		}
	}

}

PeParser::~PeParser()
{
	if (headerMemory)
	{
		delete [] headerMemory;
	}
	if (fileMemory)
	{
		delete [] fileMemory;
	}

	listSectionHeaders.clear();
	listPeSection.clear();
}

void PeParser::initClass()
{
	fileMemory = 0;
	headerMemory = 0;

	pDosHeader = 0;
	pDosStub = 0;
	dosStubSize = 0;
	pNTHeader32 = 0;
	pNTHeader64 = 0;
	overlayData = 0;
	overlaySize = 0;

	filename = 0;
	fileSize = 0;
	moduleBaseAddress = 0;
	hFile = INVALID_HANDLE_VALUE;
}

bool PeParser::isPE64()
{
	if (isValidPeFile())
	{
		return (pNTHeader32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
	}
	else
	{
		return false;
	}
}

bool PeParser::isPE32()
{
	if (isValidPeFile())
	{
		return (pNTHeader32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC);
	}
	else
	{
		return false;
	}
}

bool PeParser::isTargetFileSamePeFormat()
{
#ifdef _WIN64
	return isPE64();
#else
	return isPE32();
#endif
}

bool PeParser::isValidPeFile()
{
	bool retValue = false;

	if (pDosHeader)
	{
		if (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE)
		{
			if (pNTHeader32)
			{
				if (pNTHeader32->Signature == IMAGE_NT_SIGNATURE)
				{
					retValue = true;
				}
			}
		}
	}

	return retValue;
}

bool PeParser::hasDirectory(const int directoryIndex)
{
	if (isPE32())
	{
		return (pNTHeader32->OptionalHeader.DataDirectory[directoryIndex].VirtualAddress != 0);
	}
	else if (isPE64())
	{
		return (pNTHeader64->OptionalHeader.DataDirectory[directoryIndex].VirtualAddress != 0);
	}
	else
	{
		return false;
	}
}

bool PeParser::hasExportDirectory()
{
	return hasDirectory(IMAGE_DIRECTORY_ENTRY_EXPORT);
}

bool PeParser::hasTLSDirectory()
{
	return hasDirectory(IMAGE_DIRECTORY_ENTRY_TLS);
}

bool PeParser::hasRelocationDirectory()
{
	return hasDirectory(IMAGE_DIRECTORY_ENTRY_BASERELOC);
}

DWORD PeParser::getEntryPoint()
{
	if (isPE32())
	{
		return pNTHeader32->OptionalHeader.AddressOfEntryPoint;
	}
	else if (isPE64())
	{
		return pNTHeader64->OptionalHeader.AddressOfEntryPoint;
	}
	else
	{
		return 0;
	}
}

bool PeParser::readPeHeaderFromProcess(bool readSectionHeaders)
{
	bool retValue = false;
	DWORD correctSize = 0;

	DWORD readSize = getInitialHeaderReadSize(readSectionHeaders);

	headerMemory = new BYTE[readSize];

	if (ProcessAccessHelp::readMemoryPartlyFromProcess(moduleBaseAddress, readSize, headerMemory))
	{
		retValue = true;

		getDosAndNtHeader(headerMemory, (LONG)readSize);

		if (isValidPeFile())
		{
			correctSize = calcCorrectPeHeaderSize(readSectionHeaders);

			if (readSize < correctSize)
			{
				readSize = correctSize;
				delete [] headerMemory;
				headerMemory = new BYTE[readSize];

				if (ProcessAccessHelp::readMemoryPartlyFromProcess(moduleBaseAddress, readSize, headerMemory))
				{
					getDosAndNtHeader(headerMemory, (LONG)readSize);
				}
			}
		}
	}

	return retValue;
}

bool PeParser::readPeHeaderFromFile(bool readSectionHeaders)
{
	bool retValue = false;
	DWORD correctSize = 0;
	DWORD numberOfBytesRead = 0;

	DWORD readSize = getInitialHeaderReadSize(readSectionHeaders);

	headerMemory = new BYTE[readSize];

	if (openFileHandle())
	{
		fileSize = (DWORD)ProcessAccessHelp::getFileSize(hFile);

		if (ReadFile(hFile, headerMemory, readSize, &numberOfBytesRead, 0))
		{
			retValue = true;

			getDosAndNtHeader(headerMemory, (LONG)readSize);

			if (isValidPeFile())
			{
				correctSize = calcCorrectPeHeaderSize(readSectionHeaders);

				if (readSize < correctSize)
				{
					readSize = correctSize;

					if (fileSize > 0)
					{
						if (fileSize < correctSize)
						{
							readSize = fileSize;
						}
					}

					
					delete [] headerMemory;
					headerMemory = new BYTE[readSize];

					SetFilePointer(hFile, 0, 0, FILE_BEGIN);

					if (ReadFile(hFile, headerMemory, readSize, &numberOfBytesRead, 0))
					{
						getDosAndNtHeader(headerMemory, (LONG)readSize);
					}
				}
			}
		}

		closeFileHandle();
	}

	return retValue;
}

bool PeParser::readPeSectionsFromFile()
{
	bool retValue = true;
	DWORD numberOfBytesRead = 0;
	DWORD readSize = 0;
	DWORD readOffset = 0;

	PeFileSection peFileSection;

	if (openFileHandle())
	{
		listPeSection.reserve(getNumberOfSections());

		for (WORD i = 0; i < getNumberOfSections(); i++)
		{
			readOffset = listSectionHeaders[i].PointerToRawData;
			readSize = listSectionHeaders[i].SizeOfRawData;

			peFileSection.normalSize = readSize;

			if (readSectionFromFile(readOffset, readSize, peFileSection))
			{
				listPeSection.push_back(peFileSection);
			}
			else
			{
				retValue = false;
			}
			
		}

		closeFileHandle();
	}
	else
	{
		retValue = false;
	}

	return retValue;
}

bool PeParser::getSectionHeaders()
{
	PIMAGE_SECTION_HEADER pSection = IMAGE_FIRST_SECTION(pNTHeader32);

	listSectionHeaders.clear();
	listSectionHeaders.reserve(getNumberOfSections());

	for (WORD i = 0; i < getNumberOfSections(); i++)
	{
		listSectionHeaders.push_back(*pSection);
		pSection++;
	}

	return true;
}

bool PeParser::getSectionNameUnicode(const int sectionIndex, WCHAR * output, const int outputLen)
{
	CHAR sectionNameA[IMAGE_SIZEOF_SHORT_NAME + 1] = {0};
	
	output[0] = 0;

	memcpy(sectionNameA, listSectionHeaders[sectionIndex].Name, IMAGE_SIZEOF_SHORT_NAME); //not null terminated

	return (swprintf_s(output, outputLen, L"%S", sectionNameA) != -1);
}

WORD PeParser::getNumberOfSections()
{
	return pNTHeader32->FileHeader.NumberOfSections;
}

std::vector<IMAGE_SECTION_HEADER> & PeParser::getSectionHeaderList()
{
	return listSectionHeaders;
}

void PeParser::getDosAndNtHeader(BYTE * memory, LONG size)
{
	pDosHeader = (PIMAGE_DOS_HEADER)memory;

	pNTHeader32 = 0;
	pNTHeader64 = 0;
	dosStubSize = 0;
	pDosStub = 0;

	if (pDosHeader->e_lfanew > 0 && pDosHeader->e_lfanew < size) //malformed PE
	{
		pNTHeader32 = (PIMAGE_NT_HEADERS32)((DWORD_PTR)pDosHeader + pDosHeader->e_lfanew);
		pNTHeader64 = (PIMAGE_NT_HEADERS64)((DWORD_PTR)pDosHeader + pDosHeader->e_lfanew);

		if (pDosHeader->e_lfanew > sizeof(IMAGE_DOS_HEADER))
		{
			dosStubSize = pDosHeader->e_lfanew - sizeof(IMAGE_DOS_HEADER);
			pDosStub = (BYTE *)((DWORD_PTR)pDosHeader + sizeof(IMAGE_DOS_HEADER));
		}
	}
}

DWORD PeParser::calcCorrectPeHeaderSize(bool readSectionHeaders)
{
	DWORD correctSize = pDosHeader->e_lfanew + 50; //extra buffer

	if (readSectionHeaders)
	{
		correctSize += getNumberOfSections() * sizeof(IMAGE_SECTION_HEADER);
	}

	if (isPE32())
	{
		correctSize += sizeof(IMAGE_NT_HEADERS32);
	}
	else if(isPE64())
	{
		correctSize += sizeof(IMAGE_NT_HEADERS64);
	}
	else
	{
		correctSize = 0; //not a valid PE
	}

	return correctSize;
}

DWORD PeParser::getInitialHeaderReadSize(bool readSectionHeaders)
{
	DWORD readSize = sizeof(IMAGE_DOS_HEADER) + 200 + sizeof(IMAGE_NT_HEADERS64);

	if (readSectionHeaders)
	{
		readSize += (10 * sizeof(IMAGE_SECTION_HEADER));
	}

	return readSize;
}

DWORD PeParser::getSectionHeaderBasedFileSize()
{
	DWORD lastRawOffset = 0, lastRawSize = 0;

	//this is needed if the sections aren't sorted by their RawOffset (e.g. Petite)
	for (WORD i = 0; i < getNumberOfSections(); i++)
	{
		if (listSectionHeaders[i].PointerToRawData > lastRawOffset)
		{
			lastRawOffset = listSectionHeaders[i].PointerToRawData;
			lastRawSize = listSectionHeaders[i].SizeOfRawData;
		}
	}

	return (lastRawSize + lastRawOffset);
}

bool PeParser::openFileHandle()
{
	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (filename)
		{
			hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
		}
		else
		{
			hFile = INVALID_HANDLE_VALUE;
		}
	}

	return (hFile != INVALID_HANDLE_VALUE);
}

bool PeParser::openWriteFileHandle( const WCHAR * newFile )
{
	if (newFile)
	{
		hFile = CreateFile(newFile, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	}
	else
	{
		hFile = INVALID_HANDLE_VALUE;
	}

	return (hFile != INVALID_HANDLE_VALUE);
}


void PeParser::closeFileHandle()
{
	if (hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
	}
}

bool PeParser::readSectionFromFile(DWORD readOffset, DWORD readSize, PeFileSection & peFileSection)
{
	const DWORD maxReadSize = 100;
	BYTE data[maxReadSize];
	DWORD bytesRead = 0;
	bool retValue = true;
	DWORD valuesFound = 0;
	DWORD currentOffset = 0;

	peFileSection.data = 0;
	peFileSection.dataSize = 0;

	if (!readOffset || !readSize)
	{
		return true; //section without data is valid
	}

	if (readSize <= maxReadSize)
	{
		peFileSection.dataSize = readSize;
		peFileSection.normalSize = readSize;

		return readPeSectionFromFile(readOffset, peFileSection);
	}

	currentOffset = readOffset + readSize - maxReadSize;

	while(currentOffset >= readOffset) //start from the end
	{
		SetFilePointer(hFile, currentOffset, 0, FILE_BEGIN);

		if (!ReadFile(hFile, data, sizeof(data), &bytesRead, 0))
		{
			retValue = false;
			break;
		}

		valuesFound = isMemoryNotNull(data, sizeof(data));
		if (valuesFound)
		{
			//found some real code

			currentOffset += valuesFound;

			if (readOffset < currentOffset)
			{
				//real size
				peFileSection.dataSize = currentOffset - readOffset;
			}

			break;
		}

		currentOffset -= maxReadSize;
	}

	if (peFileSection.dataSize)
	{
		readPeSectionFromFile(readOffset, peFileSection);
	}

	return retValue;
}

DWORD PeParser::isMemoryNotNull( BYTE * data, int dataSize )
{
	for (int i = (dataSize - 1); i >= 0; i--)
	{
		if (data[i] != 0)
		{
			return i + 1;
		}
	}

	return 0;
}

bool PeParser::savePeFileToDisk( const WCHAR * newFile )
{
	bool retValue = true;
	DWORD dwFileOffset = 0, dwWriteSize = 0;

	if (getNumberOfSections() != listSectionHeaders.size() || getNumberOfSections() != listPeSection.size())
	{
		return false;
	}

	if (openWriteFileHandle(newFile))
	{
		//Dos header
		dwWriteSize = sizeof(IMAGE_DOS_HEADER);
		if (!ProcessAccessHelp::writeMemoryToFile(hFile, dwFileOffset, dwWriteSize, pDosHeader))
		{
			retValue = false;
		}
		dwFileOffset += dwWriteSize;


		if (dosStubSize && pDosStub)
		{
			//Dos Stub
			dwWriteSize = dosStubSize;
			if (!ProcessAccessHelp::writeMemoryToFile(hFile, dwFileOffset, dwWriteSize, pDosStub))
			{
				retValue = false;
			}
			dwFileOffset += dwWriteSize;
		}


		//Pe Header
		if (isPE32())
		{
			dwWriteSize = sizeof(IMAGE_NT_HEADERS32);
		}
		else
		{
			dwWriteSize = sizeof(IMAGE_NT_HEADERS64);
		}

		if (!ProcessAccessHelp::writeMemoryToFile(hFile, dwFileOffset, dwWriteSize, pNTHeader32))
		{
			retValue = false;
		}
		dwFileOffset += dwWriteSize;

		//section headers
		dwWriteSize = sizeof(IMAGE_SECTION_HEADER);

		for (WORD i = 0; i < getNumberOfSections(); i++)
		{
			if (!ProcessAccessHelp::writeMemoryToFile(hFile, dwFileOffset, dwWriteSize, &listSectionHeaders[i]))
			{
				retValue = false;
				break;
			}
			dwFileOffset += dwWriteSize;
		}

		for (WORD i = 0; i < getNumberOfSections(); i++)
		{
			if (!listSectionHeaders[i].PointerToRawData)
				continue;

			dwWriteSize = listSectionHeaders[i].PointerToRawData - dwFileOffset; //padding

			if (dwWriteSize)
			{
				if (!writeZeroMemoryToFile(hFile, dwFileOffset, dwWriteSize))
				{
					retValue = false;
					break;
				}
				dwFileOffset += dwWriteSize;
			}

			dwWriteSize = listPeSection[i].dataSize;

			if (dwWriteSize)
			{
				if (!ProcessAccessHelp::writeMemoryToFile(hFile, dwFileOffset, dwWriteSize, listPeSection[i].data))
				{
					retValue = false;
					break;
				}
				dwFileOffset += dwWriteSize;

				if (listPeSection[i].dataSize < listSectionHeaders[i].SizeOfRawData) //padding
				{
					dwWriteSize = listSectionHeaders[i].SizeOfRawData - listPeSection[i].dataSize;

					if (!writeZeroMemoryToFile(hFile, dwFileOffset, dwWriteSize))
					{
						retValue = false;
						break;
					}
					dwFileOffset += dwWriteSize;
				}
			}

		}

		closeFileHandle();
	}

	return retValue;
}

bool PeParser::writeZeroMemoryToFile(HANDLE hFile, DWORD fileOffset, DWORD size)
{
	bool retValue = false;
	PVOID zeromemory = calloc(size, 1);

	if (zeromemory)
	{
		retValue = ProcessAccessHelp::writeMemoryToFile(hFile, fileOffset, size, zeromemory);
		free(zeromemory);
	}

	return retValue;
}

void PeParser::removeDosStub()
{
	if (pDosHeader)
	{
		dosStubSize = 0;
		pDosStub = 0; //must not delete []
		pDosHeader->e_lfanew = sizeof(IMAGE_DOS_HEADER);
	}
}

bool PeParser::readPeSectionFromFile(DWORD readOffset, PeFileSection & peFileSection)
{
	DWORD bytesRead = 0;

	peFileSection.data = new BYTE[peFileSection.dataSize];

	SetFilePointer(hFile, readOffset, 0, FILE_BEGIN);
	if (!ReadFile(hFile, peFileSection.data, peFileSection.dataSize, &bytesRead, 0))
	{
		return false;
	}
	else
	{
		return true;
	}
}



