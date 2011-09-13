#include <Windows.h>
#include <stdlib.h>

typedef enum _ReBaseErr
{
	RB_OK = 0,
	RB_INVALIDPE,
	RB_NORELOCATIONINFO,
	RB_INVALIDRVA,
	RB_INVALIDNEWBASE,
	RB_ACCESSVIOLATION
} ReBaseErr;

class PeRebuild {

/*****************************************************************************
  Improved Realign DLL version 1.5 by yoda
*****************************************************************************/
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
#define MAX_SEC_NUM 30

	const static DWORD ScanStartDS = 0x40;
	const static int MinSectionTerm = 5;
	const static int FileAlignmentConstant = 0x200;

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