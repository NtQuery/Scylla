#pragma once

#include "ApiReader.h"

class IATSearch : protected ApiReader
{
public:

	DWORD_PTR memoryAddress;
	SIZE_T memorySize;

	bool searchImportAddressTableInProcess(DWORD_PTR startAddress, DWORD_PTR* addressIAT, DWORD* sizeIAT);

private:

	DWORD_PTR findAPIAddressInIAT(DWORD_PTR startAddress);
	DWORD_PTR findNextFunctionAddress();
	DWORD_PTR findIATPointer();
	//DWORD_PTR findAddressFromWORDString(char * stringBuffer);
	//DWORD_PTR findAddressFromNormalCALLString(char * stringBuffer);
	bool isIATPointerValid(DWORD_PTR iatPointer);

	bool findIATStartAndSize(DWORD_PTR address, DWORD_PTR * addressIAT, DWORD * sizeIAT);

	DWORD_PTR findIATStartAddress( DWORD_PTR baseAddress, DWORD_PTR startAddress, BYTE * dataBuffer );
	DWORD findIATSize( DWORD_PTR baseAddress, DWORD_PTR iatAddress, BYTE * dataBuffer, DWORD bufferSize );

	bool isAddressAccessable(DWORD_PTR address);
};
