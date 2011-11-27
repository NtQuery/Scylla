#include "PeDump.h"
#include "ProcessAccessHelp.h"

#include "Logger.h"
#include "definitions.h"

bool PeDump::useHeaderFromDisk = true;
bool PeDump::appendOverlayData = true;

//#define DEBUG_COMMENTS

bool PeDump::fillPeHeaderStructs(bool fromDisk)
{
	DWORD dwSize = ProcessAccessHelp::PE_HEADER_BYTES_COUNT;

	if (dwSize > sizeOfImage)
	{
		dwSize = (DWORD)sizeOfImage;
	}

	headerData = new BYTE[dwSize];

	if (!headerData)
		return false;

	if (fromDisk)
	{
		//from disk
		if (!ProcessAccessHelp::readHeaderFromFile(headerData, dwSize, fullpath))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(L"fillPeHeaderStructs -> ProcessAccessHelp::readHeaderFromFile failed - %X %s\r\n", dwSize, fullpath);
#endif
			return false;
		}
	}
	else
	{
		//from memory
		if (!ProcessAccessHelp::readMemoryFromProcess(imageBase, dwSize, headerData))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("fillPeHeaderStructs -> ProcessAccessHelp::readMemoryFromProcess failed - ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" %X ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"),imageBase, dwSize, headerData);
#endif
			return false;
		}
	}

	pDOSHeader = (PIMAGE_DOS_HEADER)headerData;
	pNTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)headerData + (DWORD_PTR)pDOSHeader->e_lfanew);
	pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);

	return true;
}

bool PeDump::validateHeaders()
{
	if ((pDOSHeader != 0) && (pDOSHeader->e_magic == IMAGE_DOS_SIGNATURE) && (pNTHeader->Signature == IMAGE_NT_SIGNATURE))
	{
#ifdef _WIN64
		if (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
#else
		if (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
#endif
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool PeDump::dumpCompleteProcessToDisk(const WCHAR * dumpFilePath)
{
	if (!fillPeHeaderStructs(useHeaderFromDisk))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("dumpCompleteProcessToDisk -> fillPeHeaderStructs failed\r\n");
#endif
		return false;
	}

	if (!validateHeaders())
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("dumpCompleteProcessToDisk -> validateHeaders failed\r\n");
#endif
		return false;
	}

	dumpData = new BYTE[sizeOfImage];

	if (dumpData)
	{
		if (!ProcessAccessHelp::readMemoryFromProcess(imageBase,sizeOfImage,dumpData))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog("dumpCompleteProcessToDisk -> readMemoryFromProcess failed\r\n");
#endif
			return false;
		}
		else
		{

			fixDump(dumpData);

			if (saveDumpToDisk(dumpFilePath, dumpData, (DWORD)sizeOfImage))
			{

				if (appendOverlayData)
				{
					appendOverlayDataToDump(dumpFilePath);
				}

				//printf("dump success\n");
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("dumpCompleteProcessToDisk -> new BYTE[sizeOfImage] failed %X\r\n",sizeOfImage);
#endif
		return false;
	}
}

bool PeDump::appendOverlayDataToDump(const WCHAR *dumpFilePath)
{
	DWORD_PTR offset = 0;
	DWORD size = 0;

	if (getOverlayData(fullpath,&offset,&size))
	{
		if (offset == 0)
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog("appendOverlayDataToDump :: No overlay exists\r\n");
#endif
			return true;
		}
		else
		{
			if (copyFileDataFromOffset(fullpath, dumpFilePath, offset, size))
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog("appendOverlayDataToDump :: appending overlay success\r\n");
#endif
				return true;
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog("appendOverlayDataToDump :: appending overlay failed\r\n");
#endif
				return false;
			}
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("appendOverlayDataToDump :: getOverlayData failed\r\n");
#endif
		return false;
	}
}

bool PeDump::copyFileDataFromOffset(const WCHAR * sourceFile, const WCHAR * destFile, DWORD_PTR fileOffset, DWORD dwSize)
{
	HANDLE hSourceFile, hDestFile;
	BYTE * dataBuffer = 0;
	bool retValue = false;

	hSourceFile = CreateFile(sourceFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if(hSourceFile == INVALID_HANDLE_VALUE)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("copyFileDataFromOffset :: failed to open source file\r\n");
#endif
		return false;
	}

	hDestFile = CreateFile(destFile, GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);

	if(hSourceFile == INVALID_HANDLE_VALUE)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("copyFileDataFromOffset :: failed to open destination file\r\n");
#endif
		CloseHandle(hSourceFile);
		return false;
	}

	dataBuffer = new BYTE[dwSize];

	if (ProcessAccessHelp::readMemoryFromFile(hSourceFile, (LONG)fileOffset, dwSize, dataBuffer))
	{
		if (ProcessAccessHelp::writeMemoryToFileEnd(hDestFile,dwSize,dataBuffer))
		{
			retValue = true;
		}
		else
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog("copyFileDataFromOffset :: writeMemoryToFileEnd failed\r\n");
#endif
			retValue = false;
		}
	}
	else
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("copyFileDataFromOffset :: readMemoryFromFile failed to read from source file\r\n");
#endif
		retValue = false;
	}

	delete [] dataBuffer;

	CloseHandle(hSourceFile);
	CloseHandle(hDestFile);

	return retValue;
}

void PeDump::fixDump(BYTE * dumpBuffer)
{
	int counter = 0;
	PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)dumpBuffer;
	PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)((DWORD_PTR)dumpBuffer + pDos->e_lfanew);
	PIMAGE_SECTION_HEADER pSec = IMAGE_FIRST_SECTION(pNt);

	fixNtHeaderForDump(pNt, pNTHeader);

	do 
	{
		fixSectionHeaderForDump(pSec, pSectionHeader);

		pSectionHeader++;
		pSec++;

		counter++;
	} while (counter < pNt->FileHeader.NumberOfSections);


}

void PeDump::fixBadNtHeaderValues(PIMAGE_NT_HEADERS pNtHead)
{
	//maybe imagebase in process is not real imagebase
	pNtHead->OptionalHeader.ImageBase = imageBase;
	pNtHead->OptionalHeader.AddressOfEntryPoint = (DWORD)(entryPoint - imageBase);
	pNtHead->OptionalHeader.SizeOfImage = sizeOfImage;
}

void PeDump::fixSectionHeaderForDump(PIMAGE_SECTION_HEADER oldSecHead, PIMAGE_SECTION_HEADER newSecHead)
{
	memcpy_s(oldSecHead->Name, IMAGE_SIZEOF_SHORT_NAME, newSecHead->Name, IMAGE_SIZEOF_SHORT_NAME);

	oldSecHead->Characteristics = newSecHead->Characteristics;

	oldSecHead->Misc.VirtualSize = newSecHead->Misc.VirtualSize;
	oldSecHead->VirtualAddress = newSecHead->VirtualAddress;

	oldSecHead->SizeOfRawData = newSecHead->Misc.VirtualSize;
	oldSecHead->PointerToRawData = newSecHead->VirtualAddress;
}

void PeDump::fixNtHeaderForDump(PIMAGE_NT_HEADERS oldNtHead, PIMAGE_NT_HEADERS newNtHead)
{
	//some special
	fixBadNtHeaderValues(newNtHead);

	//fix FileHeader
	oldNtHead->FileHeader.NumberOfSections = newNtHead->FileHeader.NumberOfSections;

	//fix OptionalHeader
	oldNtHead->OptionalHeader.ImageBase = newNtHead->OptionalHeader.ImageBase;
	oldNtHead->OptionalHeader.SizeOfImage = newNtHead->OptionalHeader.SizeOfImage;
	oldNtHead->OptionalHeader.BaseOfCode = newNtHead->OptionalHeader.BaseOfCode;
	oldNtHead->OptionalHeader.AddressOfEntryPoint = newNtHead->OptionalHeader.AddressOfEntryPoint;
	oldNtHead->OptionalHeader.SectionAlignment = newNtHead->OptionalHeader.SectionAlignment;
	oldNtHead->OptionalHeader.FileAlignment = newNtHead->OptionalHeader.SectionAlignment;

	//deleted in x64 PE
#ifndef _WIN64
	oldNtHead->OptionalHeader.BaseOfData = newNtHead->OptionalHeader.BaseOfData;
#endif
}

bool PeDump::saveDumpToDisk(const WCHAR * dumpFilePath, BYTE *dumpBuffer, DWORD dumpSize)
{
	DWORD lpNumberOfBytesWritten = 0;
	bool retValue = false;

	HANDLE hFile = CreateFile(dumpFilePath, GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

	if(hFile == INVALID_HANDLE_VALUE)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("saveDumpToDisk :: INVALID_HANDLE_VALUE %u\r\n",GetLastError());
#endif
		retValue = false;
	}
	else
	{
		if (WriteFile(hFile, dumpBuffer, dumpSize, &lpNumberOfBytesWritten, 0))
		{
			if (lpNumberOfBytesWritten != dumpSize)
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog("saveDumpToDisk :: lpNumberOfBytesWritten != dumpSize %d %d\r\n",lpNumberOfBytesWritten,dumpSize);
#endif
				retValue = false;
			}
			else
			{
				retValue = true;
			}
		}
		else
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog("saveDumpToDisk :: WriteFile failed %u\r\n",GetLastError());
#endif
			retValue = false;
		}

		CloseHandle(hFile);
	}

	return retValue;
}

bool PeDump::getOverlayData(const WCHAR * filepath, DWORD_PTR * overlayFileOffset, DWORD * overlaySize)
{
	LONGLONG fileSize = 0;
	DWORD dwSize = 0;
	DWORD bufferSize = 1000;
	BYTE *buffer = 0;
	bool returnValue = 0;
	PIMAGE_DOS_HEADER pDOSh = 0;
	PIMAGE_NT_HEADERS pNTh = 0;
	PIMAGE_SECTION_HEADER pSech = 0;
	int counter = 0;
	DWORD calcSize = 0;

	HANDLE hFile = CreateFile(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if( hFile == INVALID_HANDLE_VALUE )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("getOverlayData :: INVALID_HANDLE_VALUE %u\r\n",GetLastError());
#endif
		returnValue = false;
	}
	else
	{
		fileSize = ProcessAccessHelp::getFileSize(hFile);

		if (fileSize > 0)
		{
			if (fileSize > bufferSize)
			{
				dwSize = bufferSize;
			}
			else
			{
				dwSize = (DWORD)(fileSize - 1);
			}

			buffer = new BYTE[dwSize];

			if (ProcessAccessHelp::readMemoryFromFile(hFile, 0, dwSize, buffer))
			{
				pDOSh = (PIMAGE_DOS_HEADER)buffer;
				pNTh = (PIMAGE_NT_HEADERS)((DWORD_PTR)buffer + pDOSh->e_lfanew);

				//first section
				pSech = IMAGE_FIRST_SECTION(pNTh);
				counter = 1;

				//get last section
				while(counter < pNTh->FileHeader.NumberOfSections)
				{
					counter++;
					pSech++;
				}

				//printf("PointerToRawData %X\nSizeOfRawData %X\nfile size %X\n",pSech->PointerToRawData,pSech->SizeOfRawData,pSech->PointerToRawData+pSech->SizeOfRawData);

				calcSize = pSech->PointerToRawData + pSech->SizeOfRawData;

				if (calcSize < fileSize)
				{
					//overlay found
					*overlayFileOffset = calcSize;
					*overlaySize = (DWORD)(fileSize - calcSize);
				}
				else
				{
					*overlayFileOffset = 0;
					*overlaySize = 0;
				}

				returnValue = true;
			}
			else
			{
				returnValue = false;
			}

			delete [] buffer;
		}
		else
		{
			returnValue = false;
		}

		CloseHandle(hFile);
	}

	return returnValue;
}