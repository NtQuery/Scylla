
#include "PeParser.h"
#include "ProcessAccessHelp.h"

PeParser::PeParser()
{
	fileMemory = 0;
	headerMemory = 0;
	pDosHeader = 0;
	pNTHeader32 = 0;
	pNTHeader64 = 0;
	filename = 0;
	moduleBaseAddress = 0;
}

PeParser::PeParser(const WCHAR * file, bool readSectionHeaders)
{
	fileMemory = 0;
	headerMemory = 0;
	pDosHeader = 0;
	pNTHeader32 = 0;
	pNTHeader64 = 0;
	moduleBaseAddress = 0;

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
	fileMemory = 0;
	headerMemory = 0;
	pDosHeader = 0;
	pNTHeader32 = 0;
	pNTHeader64 = 0;
	filename = 0;

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

	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (hFile != INVALID_HANDLE_VALUE)
	{
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

		CloseHandle(hFile);
	}

	return retValue;
}

bool PeParser::readFileToMemory()
{
	bool retValue = false;
	DWORD numberOfBytesRead = 0;
	LARGE_INTEGER largeInt = {0};
	const DWORD MaxFileSize = 500 * 1024 * 1024; // GB * MB * KB * B -> 500 MB

	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		if (GetFileSizeEx(hFile, &largeInt))
		{
			if (largeInt.QuadPart > MaxFileSize)
			{
				//TODO handle big files
				retValue = false;
			}
			else
			{
				fileMemory = new BYTE[largeInt.LowPart];

				if (ReadFile(hFile,fileMemory,largeInt.LowPart,&numberOfBytesRead,0))
				{
					retValue = true;
				}
				else
				{
					delete [] fileMemory;
					fileMemory = 0;
				}
			}
		}

		CloseHandle(hFile);
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

	if (pDosHeader->e_lfanew > 0 && pDosHeader->e_lfanew < size) //malformed PE
	{
		pNTHeader32 = (PIMAGE_NT_HEADERS32)((DWORD_PTR)pDosHeader + pDosHeader->e_lfanew);
		pNTHeader64 = (PIMAGE_NT_HEADERS64)((DWORD_PTR)pDosHeader + pDosHeader->e_lfanew);
	}
	else
	{
		pNTHeader32 = 0;
		pNTHeader64 = 0;
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

