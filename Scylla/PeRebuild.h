#pragma once

#include <Windows.h>
#include <cstdlib>

enum ReBaseErr
{
	RB_OK = 0,
	RB_INVALIDPE,
	RB_NORELOCATIONINFO,
	RB_INVALIDRVA,
	RB_INVALIDNEWBASE,
	RB_ACCESSVIOLATION
};

/*****************************************************************************
  Improved Realign DLL version 1.5 by yoda
*****************************************************************************/

class PeRebuild
{
public:

	bool truncateFile(WCHAR * szFilePath, DWORD dwNewFsize);
	DWORD realignPE(LPVOID AddressOfMapFile,DWORD dwFsize);
	DWORD wipeReloc(void* pMap, DWORD dwFsize);
	bool validatePE(void* pPEImage, DWORD dwFileSize);
	ReBaseErr reBasePEImage(void* pPE, DWORD_PTR dwNewBase);

	bool updatePeHeaderChecksum(LPVOID AddressOfMapFile, DWORD dwFsize);

	LPVOID createFileMappingViewFull(const WCHAR * filePath);
	void closeAllMappingHandles();

private:

	// constants
	static const size_t MAX_SEC_NUM = 30;

	static const DWORD ScanStartDS = 0x40;
	static const int MinSectionTerm = 5;
	static const int FileAlignmentConstant = 0x200;

	// variables
	DWORD_PTR            dwMapBase;
	LPVOID               pMap;
	DWORD				 dwTmpNum,dwSectionBase;
	WORD                 wTmpNum;
	CHAR *	             pCH;
	WORD *			     pW;
	DWORD *				 pDW;
	LPVOID               pSections[MAX_SEC_NUM];

	//my vars
	HANDLE hFileToMap;
	HANDLE hMappedFile;
	LPVOID addrMappedDll;


	DWORD validAlignment(DWORD BadSize);
	DWORD validAlignmentNew(DWORD badAddress);
	bool isRoundedTo(DWORD_PTR dwTarNum, DWORD_PTR dwRoundNum);

	void cleanSectionPointer();
	bool validatePeHeaders( PIMAGE_DOS_HEADER pDosh );
};
