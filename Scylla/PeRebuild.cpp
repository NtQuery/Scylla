#include "PeRebuild.h"

#include <imagehlp.h>
#pragma comment(lib,"imagehlp.lib")

#include "ProcessAccessHelp.h"

#include "Logger.h"
#include "ConfigurationHolder.h"

//#define DEBUG_COMMENTS

bool PeRebuild::truncateFile(WCHAR * szFilePath, DWORD dwNewFsize)
{
	bool retValue = true;
	HANDLE hFile = CreateFile(szFilePath,GENERIC_WRITE, FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	if (SetFilePointer(hFile, dwNewFsize, 0, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		if (GetLastError() == NO_ERROR)
		{
			retValue = true;
		}
		else
		{
			retValue = false;
		}
	}
	else
	{
		retValue = true;
	}

	if (!SetEndOfFile(hFile))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("SetEndOfFile failed error %d\r\n", GetLastError());
#endif
		retValue = false;
	}

	CloseHandle(hFile);

	return retValue;
}

DWORD PeRebuild::validAlignment(DWORD BadSize)
{
	div_t DivRes;

	DivRes = div(BadSize, FileAlignmentConstant);
	if (DivRes.rem == 0)
		return BadSize;
	return ((DivRes.quot+1) * FileAlignmentConstant);
}

DWORD PeRebuild::validAlignmentNew(DWORD badAddress)
{
	DWORD moduloResult = badAddress % FileAlignmentConstant;

	if (moduloResult)
	{
		return (FileAlignmentConstant - moduloResult);
	}
	else
	{
		return 0;
	}
}

bool PeRebuild::isRoundedTo(DWORD_PTR dwTarNum, DWORD_PTR dwRoundNum)
{
#ifdef _WIN64
	lldiv_t d;

	d = div((__int64)dwTarNum, (__int64)dwRoundNum);
#else
	ldiv_t d;

	d = div((long)dwTarNum, (long)dwRoundNum);
#endif

	return (d.rem == 0);
}

void PeRebuild::cleanSectionPointer()
{
	if (pSections)
	{
		for (int j = 0; j < MAX_SEC_NUM; j++)
		{
			if (pSections[j])
			{
				free(pSections[j]);
				pSections[j] = 0;
			}
		}
	}
}

DWORD PeRebuild::realignPE(LPVOID AddressOfMapFile, DWORD dwFsize)
{
	PIMAGE_DOS_HEADER pDosh = 0;
	PIMAGE_NT_HEADERS pPeh = 0;
	PIMAGE_SECTION_HEADER pSectionh = 0;
	int i = 0;
	DWORD extraAlign = 0;

	ZeroMemory(&pSections, sizeof(pSections));

	// get the other parameters
	pMap = AddressOfMapFile;
	dwMapBase = (DWORD_PTR)pMap;

	if (dwFsize == 0 || pMap == NULL)
		return 1;

	// access the PE Header and check whether it's a valid one
	pDosh = (PIMAGE_DOS_HEADER)(pMap);
	pPeh = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDosh+pDosh->e_lfanew);

	if (!validatePeHeaders(pDosh))
	{
		return 0;
	}

	if (pPeh->FileHeader.NumberOfSections > MAX_SEC_NUM)
	{
		return 3;
	}

	__try
	{
		/* START */
		pPeh->OptionalHeader.FileAlignment = FileAlignmentConstant;

		/* Realign the PE Header */
		// get the size of all headers
		dwTmpNum = FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader) + pPeh->FileHeader.SizeOfOptionalHeader + (pPeh->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));


		// kill room between the "win32 pls" message and the PE signature
		// find the end of the message
		pW = (WORD*)(dwMapBase + ScanStartDS);
		while (*pW != 0 || (!isRoundedTo((DWORD_PTR)pW, 0x10)))
		{
			pW = (WORD*)((DWORD_PTR)pW + 1);
		}

		wTmpNum = (WORD)((DWORD_PTR)pW - dwMapBase);
		if (wTmpNum < pDosh->e_lfanew)
		{
			CopyMemory((LPVOID)pW,(VOID*)pPeh,dwTmpNum); // copy the Header to the right place
			pDosh->e_lfanew = wTmpNum;
		}

		dwSectionBase = validAlignment(dwTmpNum + pDosh->e_lfanew);
		pPeh = (PIMAGE_NT_HEADERS)(dwMapBase + pDosh->e_lfanew); // because the NT header moved
		// correct the SizeOfHeaders
		pPeh->OptionalHeader.SizeOfHeaders = dwSectionBase;

		/* Realign all sections */
		// make a copy of all sections
		// this is needed if the sections aren't sorted by their RawOffset (e.g. Petite)
		pSectionh = IMAGE_FIRST_SECTION(pPeh);

		for (i=0; i<pPeh->FileHeader.NumberOfSections; i++)
		{
			if (pSectionh->SizeOfRawData == 0 || pSectionh->PointerToRawData == 0)
			{
				++pSectionh;
				continue;
			}
			// get a valid size
			dwTmpNum = pSectionh->SizeOfRawData;
			if ((pSectionh->SizeOfRawData + pSectionh->PointerToRawData) > dwFsize)
			{
				dwTmpNum = dwFsize - pSectionh->PointerToRawData;
			}

			//dwTmpNum -= 1;

			// copy the section into some memory
			// limit max section size to 300 MB = 300000 KB = 300000000 B
			if (dwTmpNum > 300000000)
			{
				dwTmpNum = 300000000;
			}

			//because of validAlignment we need some extra space, max 0x200 extra
			extraAlign = validAlignmentNew(dwTmpNum);

			pSections[i] = malloc(dwTmpNum + extraAlign);
			ZeroMemory(pSections[i], dwTmpNum + extraAlign);

			if (pSections[i] == NULL) // fatal error !!!
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog("realignPE :: malloc failed with dwTmpNum %08X %08X\r\n", dwTmpNum, extraAlign);
#endif

				cleanSectionPointer();

				return 4;
			}
			CopyMemory(pSections[i],(LPVOID)(pSectionh->PointerToRawData+dwMapBase),dwTmpNum);
			++pSectionh;
		}

		// start realigning the sections
		pSectionh = IMAGE_FIRST_SECTION(pPeh);

		for (i=0;i<pPeh->FileHeader.NumberOfSections;i++)
		{
			// some anti crash code :P
			if (pSectionh->SizeOfRawData == 0 || pSectionh->PointerToRawData == 0)
			{
				++pSectionh;
				if (pSectionh->PointerToRawData == 0)
				{
					continue;
				}
				pSectionh->PointerToRawData = dwSectionBase;
				continue;
			}
			// let pCH point to the end of the current section
			if ((pSectionh->PointerToRawData+pSectionh->SizeOfRawData) <= dwFsize)
			{
				pCH = (char*)(dwMapBase+pSectionh->PointerToRawData+pSectionh->SizeOfRawData-1);
			}
			else
			{
				pCH = (char*)(dwMapBase+dwFsize-1);
			}
			// look for the end of this section
			while (*pCH == 0)
			{
				--pCH;
			}
			// calculate the new RawSize
			dwTmpNum = (DWORD)(((DWORD_PTR)pCH - dwMapBase) + MinSectionTerm - pSectionh->PointerToRawData);
			if (dwTmpNum < pSectionh->SizeOfRawData)
			{
				pSectionh->SizeOfRawData = dwTmpNum;
			}
			else // the new size is too BIG
			{
				dwTmpNum = pSectionh->SizeOfRawData;
			}
			// copy the section to the new place
			if (i != pPeh->FileHeader.NumberOfSections-1)
			{
				dwTmpNum = 	validAlignment(dwTmpNum);
			}

			CopyMemory((LPVOID)(dwMapBase+dwSectionBase), pSections[i], dwTmpNum);
			// set the RawOffset
			pSectionh->PointerToRawData = dwSectionBase;
			// get the RawOffset for the next section
			dwSectionBase = dwTmpNum+dwSectionBase; // the last section doesn't need to be aligned
			// go to the next section
			++pSectionh;
		}

		// delete bound import directories because it is destroyed if present
		pPeh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].VirtualAddress = 0;
		pPeh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT].Size = 0;

		// clean up
		cleanSectionPointer();
	}
	__except(1)
	{
		// clean up
		cleanSectionPointer();

#ifdef DEBUG_COMMENTS
		Logger::debugLog("realignPE :: Exception occured\r\n");
#endif
		return 0;
	}

	if (ConfigurationHolder::getConfigObject(UPDATE_HEADER_CHECKSUM)->isTrue())
	{
		updatePeHeaderChecksum(AddressOfMapFile, dwSectionBase);
	}
	
	return dwSectionBase; // return the new filesize
}


// returns:
//  -1  - access violation
//  -2  - no relocation found
//  -3  - no own section
//  -4  - dll characteristics found
//  -5  - invalid PE file
//  else the new raw size
DWORD PeRebuild::wipeReloc(void* pMap, DWORD dwFsize)
{
	PIMAGE_DOS_HEADER       pDosH;
	PIMAGE_NT_HEADERS       pNTH;
	PIMAGE_SECTION_HEADER   pSecH;
	PIMAGE_SECTION_HEADER   pSH, pSH2;
	DWORD                   dwRelocRVA, i;
	BOOL                    bOwnSec           = FALSE;
	DWORD                   dwNewFsize;

	__try  // =)
	{
		// get pe header pointers
		pDosH = (PIMAGE_DOS_HEADER)pMap;

		if (pDosH->e_magic != IMAGE_DOS_SIGNATURE)
			return -5;

		pNTH  = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDosH + pDosH->e_lfanew);

		if (pNTH->Signature != IMAGE_NT_SIGNATURE)
			return -5;

		pSecH = IMAGE_FIRST_SECTION(pNTH);

		// has PE dll characteristics ?
		if (pNTH->FileHeader.Characteristics & IMAGE_FILE_DLL)
			return -4;

		// is there a reloc section ?
		dwRelocRVA = pNTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;

		if (!dwRelocRVA)
			return -2;

		// check whether the relocation has an own section
		pSH = pSecH;
		for (i=0; i < pNTH->FileHeader.NumberOfSections; i++)
		{
			if (pSH->VirtualAddress == dwRelocRVA)
			{
				bOwnSec = TRUE;
				break; // pSH -> reloc section header and i == section index
			}
			++pSH;
		}
		if (!bOwnSec)
			return -3;

		if (i+1 == pNTH->FileHeader.NumberOfSections)
		{
			//--- relocation is the last section ---
			// truncate at the start of the reloc section
			dwNewFsize = pSH->PointerToRawData;
		}
		else
		{
			//--- relocation isn't the last section ---
			dwNewFsize = dwFsize - pSH->SizeOfRawData;

			//-> copy the section(s) after the relocation to the start of the relocation
			pSH2 = pSH;
			++pSH2; // pSH2 -> pointer to first section after relocation
			memcpy(
				(void*)(pSH->PointerToRawData + (DWORD)pMap),
				(const void*)(pSH2->PointerToRawData + (DWORD)pMap),
				dwFsize - pSH2->PointerToRawData);

			//-> fix the section headers
			// (pSH -> reloc section header)
			// (pSH2 -> first section after reloc section)
			for (++i; i < pNTH->FileHeader.NumberOfSections; i++)
			{
				// apply important values
				pSH->SizeOfRawData    = pSH2->SizeOfRawData;
				pSH->VirtualAddress   = pSH2->VirtualAddress;
				pSH->Misc.VirtualSize = pSH2->Misc.VirtualSize;

				// apply section name
				memcpy(
					(void*)(pSH->Name),
					(const void*)(pSH2->Name),
					sizeof(pSH2->Name));
				++pSH;
				++pSH2;
			}
		}

		// dec section number
		--pNTH->FileHeader.NumberOfSections;

		// kill reloc directory entry
		pNTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = 0;
		pNTH->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size           = 0;

		// fix virtual parts of the PE Header (a must for win2k)
		pSH2 = pSH = pSecH;
		++pSH2;
		for (i=0; i < (DWORD)pNTH->FileHeader.NumberOfSections-1; i++)
		{
			pSH->Misc.VirtualSize = pSH2->VirtualAddress - pSH->VirtualAddress;
			++pSH;
			++pSH2;
		}
		// (pSH -> pointer to last section)
		if (pSH->Misc.PhysicalAddress)
			pNTH->OptionalHeader.SizeOfImage = pSH->VirtualAddress + pSH->Misc.VirtualSize;
		else // WATCOM is always a bit special >:-)
			pNTH->OptionalHeader.SizeOfImage = pSH->VirtualAddress + pSH->SizeOfRawData;
	}
	__except(1)
	{
		// an access violation occurred :(
		return -1;
	}

	return dwNewFsize;
}

bool PeRebuild::validatePE(void* pPEImage, DWORD dwFileSize)
{
	PIMAGE_NT_HEADERS       pNTh;
	PIMAGE_SECTION_HEADER   pSech,pSH, pSH2, pLastSH;
	UINT                    i;
	DWORD                   dwHeaderSize;

	// get PE base information
	pNTh = ImageNtHeader(pPEImage);

	if (!pNTh)
		return FALSE;


	pSech = IMAGE_FIRST_SECTION(pNTh);

	// FIX:
	// ... the SizeOfHeaders
	pSH = pSech;
	dwHeaderSize = 0xFFFFFFFF;
	for(i=0; i < pNTh->FileHeader.NumberOfSections; i++)
	{
		if (pSH->PointerToRawData && pSH->PointerToRawData < dwHeaderSize)
		{
			dwHeaderSize = pSH->PointerToRawData;
		}
		++pSH;
	}
	pNTh->OptionalHeader.SizeOfHeaders = dwHeaderSize;

	// ...Virtual Sizes
	pSH2 = pSH = pSech;
	++pSH2;
	for (i=0; i < (DWORD)pNTh->FileHeader.NumberOfSections-1; i++)
	{
		pSH->Misc.VirtualSize = pSH2->VirtualAddress - pSH->VirtualAddress;
		++pSH;
		++pSH2;
	}

	// (pSH -> pointer to last section)
	pLastSH = pSH;

	// ...RawSize of the last section
	pLastSH->SizeOfRawData = dwFileSize - pLastSH->PointerToRawData;

	// ...SizeOfImage
	if (pLastSH->Misc.PhysicalAddress)
	{
		pNTh->OptionalHeader.SizeOfImage = pLastSH->VirtualAddress + pLastSH->Misc.VirtualSize;
	}
	else // WATCOM is always a bit special >:-)
	{
		pNTh->OptionalHeader.SizeOfImage = pLastSH->VirtualAddress + pLastSH->SizeOfRawData;
	}

	return true;
}

ReBaseErr PeRebuild::reBasePEImage(void* pPE, DWORD_PTR dwNewBase)
{
	PIMAGE_NT_HEADERS    pNT;
	PIMAGE_RELOCATION    pR;
	ReBaseErr            ret;
	DWORD_PTR            dwDelta;
	DWORD                *pdwAddr, dwRva, dwType;
	UINT                 iItems, i;
	WORD                 *pW;

	// dwNewBase valid ?
	if (dwNewBase & 0xFFFF)
	{
		ret = RB_INVALIDNEWBASE;
		goto Exit; // ERR
	}

	//
	// get relocation dir ptr
	//
	pNT = ImageNtHeader(pPE);
	if (!pNT)
	{
		ret = RB_INVALIDPE;
		goto Exit; // ERR
	}
	// new base = old base ?
	if (pNT->OptionalHeader.ImageBase == dwNewBase)
	{
		ret = RB_OK;
		goto Exit; // OK
	}
	if (!pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress)
	{
		ret = RB_NORELOCATIONINFO;
		goto Exit; // ERR
	}

	pR = (PIMAGE_RELOCATION)ImageRvaToVa(
		pNT,
		pPE,
		pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
		NULL);

	if (!pR)
	{
		ret = RB_INVALIDRVA;
		goto Exit; // ERR
	}

	//
	// add delta to relocation items
	//
	dwDelta = dwNewBase - pNT->OptionalHeader.ImageBase;
	__try
	{
		do
		{
			// get number of items
			if (pR->SymbolTableIndex)
				iItems = (pR->SymbolTableIndex - 8) / 2;
			else
				break; // no items in this block

			// trace/list block items...
			pW = (WORD*)((DWORD_PTR)pR + 8);

			for (i = 0; i < iItems; i++)
			{
				dwRva  = (*pW & 0xFFF) + pR->VirtualAddress;
				dwType = *pW >> 12;
				if (dwType != 0) // fully compatible ???
				{
					// add delta
					pdwAddr = (PDWORD)ImageRvaToVa(
						pNT,
						pPE,
						dwRva,
						NULL);
					if (!pdwAddr)
					{
						ret = RB_INVALIDRVA;
						goto Exit; // ERR
					}
					*pdwAddr += dwDelta;
				}
				// next item
				++pW;
			}

			pR = (PIMAGE_RELOCATION)pW; // pR -> next block header
		} while ( *(DWORD*)pW );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ret = RB_ACCESSVIOLATION;
		goto Exit; // ERR
	}

	// apply new base to header
	pNT->OptionalHeader.ImageBase = dwNewBase;

	ret = RB_OK; // OK

Exit:
	return ret;
}


bool PeRebuild::updatePeHeaderChecksum(LPVOID AddressOfMapFile, DWORD dwFsize)
{
	PIMAGE_NT_HEADERS32 pNTHeader32 = 0;
	PIMAGE_NT_HEADERS64 pNTHeader64 = 0;
	DWORD headerSum = 0;
	DWORD checkSum = 0;

	pNTHeader32 = (PIMAGE_NT_HEADERS32)CheckSumMappedFile(AddressOfMapFile, dwFsize, &headerSum, &checkSum);

	if (!pNTHeader32)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("updatePeHeaderChecksum :: CheckSumMappedFile failed error %X\r\n", GetLastError());
#endif
		return false;
	}

#ifdef DEBUG_COMMENTS
	Logger::debugLog("Old checksum %08X new checksum %08X\r\n",headerSum,checkSum);
#endif

	if (pNTHeader32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		pNTHeader64 = (PIMAGE_NT_HEADERS64)pNTHeader32;
		pNTHeader64->OptionalHeader.CheckSum = checkSum;
	}
	else
	{
		pNTHeader32->OptionalHeader.CheckSum = checkSum;
	}

	return true;
}

LPVOID PeRebuild::createFileMappingViewFull(const WCHAR * filePath)
{
	hFileToMap = CreateFile(filePath, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if( hFileToMap == INVALID_HANDLE_VALUE )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("createFileMappingView :: INVALID_HANDLE_VALUE %u\r\n",GetLastError());
#endif
		hMappedFile = 0;
		hFileToMap = 0;
		addrMappedDll = 0;
		return NULL;
	}

	hMappedFile = CreateFileMapping(hFileToMap, 0, PAGE_READWRITE, 0, 0, NULL);

	if( hMappedFile == NULL )
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("createFileMappingViewFull :: hMappedFile == NULL\r\n");
#endif
		CloseHandle(hFileToMap);
		hMappedFile = 0;
		hFileToMap = 0;
		addrMappedDll = 0;
		return NULL;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("createFileMappingView :: GetLastError() == ERROR_ALREADY_EXISTS\r\n");
#endif
		CloseHandle(hFileToMap);
		hMappedFile = 0;
		hFileToMap = 0;
		addrMappedDll = 0;
		return NULL;
	}

	addrMappedDll = MapViewOfFile(hMappedFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);

	if(!addrMappedDll)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("createFileMappingView :: addrMappedDll == NULL\r\n");
#endif
		CloseHandle(hFileToMap);
		CloseHandle(hMappedFile);
		hMappedFile = 0;
		hFileToMap = 0;
		return NULL;
	}

	return addrMappedDll;
}

void PeRebuild::closeAllMappingHandles()
{
	if (addrMappedDll)
	{
		if (!FlushViewOfFile(addrMappedDll, 0)) 
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog("closeAllMappingHandles :: Could not flush memory to disk (%d)\r\n", GetLastError());
#endif
		}

		UnmapViewOfFile(addrMappedDll);
		addrMappedDll = 0;
	}
	if (hMappedFile)
	{
		CloseHandle(hMappedFile);
		hMappedFile = 0;
	}
	if (hFileToMap)
	{
		CloseHandle(hFileToMap);
		hFileToMap = 0;
	}
}

bool PeRebuild::validatePeHeaders( PIMAGE_DOS_HEADER pDosh )
{
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDosh + pDosh->e_lfanew);

	if ((pDosh != 0) && (pDosh->e_magic == IMAGE_DOS_SIGNATURE) && (pNTHeader->Signature == IMAGE_NT_SIGNATURE))
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
