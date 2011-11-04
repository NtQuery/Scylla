#include "IATSearch.h"
#include "Logger.h"
#include "definitions.h"

//#define DEBUG_COMMENTS


bool IATSearch::searchImportAddressTableInProcess(DWORD_PTR startAddress, DWORD_PTR* addressIAT, DWORD* sizeIAT)
{
	DWORD_PTR addressInIAT = 0;

	addressInIAT = findAPIAddressInIAT(startAddress);

	if(!addressInIAT)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("searchImportAddressTableInProcess :: addressInIAT not found, startAddress ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"),startAddress);
#endif
		return false;
	}
	else
	{
		return findIATStartAndSize(addressInIAT, addressIAT,sizeIAT);
	}
}

DWORD_PTR IATSearch::findAPIAddressInIAT(DWORD_PTR startAddress)
{
	static const int MEMORY_READ_SIZE = 200;
	BYTE *dataBuffer = new BYTE[MEMORY_READ_SIZE];
	DWORD_PTR iatPointer = 0;
	int counter = 0;

	// to detect stolen api
	memoryAddress = 0;
	memorySize = 0;

	do 
	{
		counter++;

		if (!readMemoryFromProcess(startAddress,MEMORY_READ_SIZE,dataBuffer))
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog(TEXT("findAPIAddressInIAT :: error reading memory ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"), startAddress);
#endif
			return 0;
		}

		if (decomposeMemory(dataBuffer,MEMORY_READ_SIZE,startAddress))
		{
			iatPointer = findIATPointer();
			if (iatPointer)
			{
				if (isIATPointerValid(iatPointer))
				{
					delete[] dataBuffer;
					return iatPointer;
				}
			}
		}

		startAddress = findNextFunctionAddress();
		//printf("startAddress %08X\n",startAddress);
	} while (startAddress != 0 && counter != 8);


	delete[] dataBuffer;
	return 0;
}

DWORD_PTR IATSearch::findNextFunctionAddress()
{
#ifdef DEBUG_COMMENTS
	_DecodedInst inst;
#endif

	for (unsigned int i = 0; i < decomposerInstructionsCount; i++)
	{

		if (decomposerResult[i].flags != FLAG_NOT_DECODABLE)
		{
			if (META_GET_FC(decomposerResult[i].meta) == FC_CALL || META_GET_FC(decomposerResult[i].meta) == FC_UNC_BRANCH)
			{
				if (decomposerResult[i].size >= 5)
				{
					if (decomposerResult[i].ops[0].type == O_PC)
					{
#ifdef DEBUG_COMMENTS
						distorm_format(&decomposerCi, &decomposerResult[i], &inst);
						Logger::debugLog(TEXT("%S %S %d %d - target address: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"), inst.mnemonic.p, inst.operands.p,decomposerResult[i].ops[0].type,decomposerResult[i].size, INSTRUCTION_GET_TARGET(&decomposerResult[i]));
#endif
						return (DWORD_PTR)INSTRUCTION_GET_TARGET(&decomposerResult[i]);
					}
				}
			}
		}
	}

	return 0;
}

DWORD_PTR IATSearch::findIATPointer()
{
#ifdef DEBUG_COMMENTS
	_DecodedInst inst;
#endif

	for (unsigned int i = 0; i < decomposerInstructionsCount; i++)
	{

		if (decomposerResult[i].flags != FLAG_NOT_DECODABLE)
		{
			if (META_GET_FC(decomposerResult[i].meta) == FC_CALL || META_GET_FC(decomposerResult[i].meta) == FC_UNC_BRANCH)
			{
				if (decomposerResult[i].size >= 5)
				{
#ifdef _WIN64
					if (decomposerResult[i].flags & FLAG_RIP_RELATIVE)
					{
#ifdef DEBUG_COMMENTS
						distorm_format(&decomposerCi, &decomposerResult[i], &inst);
						Logger::debugLog(TEXT("%S %S %d %d - target address: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"), inst.mnemonic.p, inst.operands.p,decomposerResult[i].ops[0].type,decomposerResult[i].size,INSTRUCTION_GET_RIP_TARGET(&decomposerResult[i]));
#endif
						return INSTRUCTION_GET_RIP_TARGET(&decomposerResult[i]);
					}
#else
					if (decomposerResult[i].ops[0].type == O_DISP)
					{
						//jmp dword ptr || call dword ptr
#ifdef DEBUG_COMMENTS
						distorm_format(&decomposerCi, &decomposerResult[i], &inst);
						Logger::debugLog(TEXT("%S %S %d %d - target address: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"), inst.mnemonic.p, inst.operands.p,decomposerResult[i].ops[0].type,decomposerResult[i].size,decomposerResult[i].disp);
#endif
						return (DWORD_PTR)decomposerResult[i].disp;
					}
#endif
				}
			}
		}
	}

	return 0;
}

/*DWORD_PTR IATSearch::findAddressFromWORDString(char * stringBuffer)
{
	char * pAddress = 0;
	char * pTemp = 0;
	DWORD_PTR address = 0;

	//string split it e.g. DWORD [0x40f0fc], QWORD [RIP+0x40f0]
	pAddress = strchr(stringBuffer, 'x');

	if (pAddress)
	{
		pAddress++;

		pTemp = strchr(pAddress, ']');
		*pTemp = 0x00;

		address = strtoul(pAddress, 0, 16);

		//printf("findAddressFromWORDString :: %08X\n",address);

		if (address == ULONG_MAX)
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog("findAddressFromDWORDString :: strtoul ULONG_MAX\r\n");
#endif
			return 0;
		}
		else
		{
			return address;
		}
	}
	else
	{
		return 0;
	}
}*/

/*DWORD_PTR IATSearch::findAddressFromNormalCALLString(char * stringBuffer)
{
	char * pAddress = 0;
	DWORD_PTR address = 0;

	//e.g. CALL 0x7238
	pAddress = strchr(stringBuffer, 'x');

	if (pAddress)
	{
		pAddress++;

		address = strtoul(pAddress, 0, 16);

		//printf("findAddressFromNormalCALLString :: %08X\n",address);

		if (address == ULONG_MAX)
		{
#ifdef DEBUG_COMMENTS
			Logger::debugLog("findAddressFromNormalCALLString :: strtoul ULONG_MAX\r\n");
#endif
			return 0;
		}
		else
		{
			return address;
		}
	}
	else
	{
		return 0;
	}
}*/

bool IATSearch::isIATPointerValid(DWORD_PTR iatPointer)
{
	DWORD_PTR apiAddress = 0;

	if (!readMemoryFromProcess(iatPointer,sizeof(DWORD_PTR),&apiAddress))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("isIATPointerValid :: error reading memory\r\n");
#endif
		return false;
	}

	//printf("Win api ? %08X\n",apiAddress);

	if (isApiAddressValid(apiAddress) != 0)
	{
		return true;
	}
	else
	{
		//maybe redirected import?
		//if the address is 2 times inside a memory region it is possible a redirected api
		if (apiAddress > memoryAddress && apiAddress < (memoryAddress+memorySize))
		{
			return true;
		}
		else
		{
			getMemoryRegionFromAddress(apiAddress, &memoryAddress, &memorySize);
			return false;
		}
		
	}
}

bool IATSearch::findIATStartAndSize(DWORD_PTR address, DWORD_PTR * addressIAT, DWORD * sizeIAT)
{
	MEMORY_BASIC_INFORMATION memBasic = {0};
	BYTE *dataBuffer = 0;

	if (VirtualQueryEx(hProcess,(LPCVOID)address,&memBasic,sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("findIATStartAddress :: VirtualQueryEx error %u\r\n",GetLastError());
#endif
		return false;
	}

	//(sizeof(DWORD_PTR) * 3) added to prevent buffer overflow
	dataBuffer = new BYTE[memBasic.RegionSize + (sizeof(DWORD_PTR) * 3)];

	ZeroMemory(dataBuffer, memBasic.RegionSize + (sizeof(DWORD_PTR) * 3));

	if (!readMemoryFromProcess((DWORD_PTR)memBasic.BaseAddress, memBasic.RegionSize, dataBuffer))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("findIATStartAddress :: error reading memory\r\n");
#endif
		return false;
	}

	//printf("address %X memBasic.BaseAddress %X memBasic.RegionSize %X\n",address,memBasic.BaseAddress,memBasic.RegionSize);

	*addressIAT = findIATStartAddress((DWORD_PTR)memBasic.BaseAddress, address, dataBuffer);

	*sizeIAT = findIATSize((DWORD_PTR)memBasic.BaseAddress, *addressIAT,dataBuffer,(DWORD)memBasic.RegionSize);

	delete [] dataBuffer;

	return true;
}

DWORD_PTR IATSearch::findIATStartAddress(DWORD_PTR baseAddress, DWORD_PTR startAddress, BYTE * dataBuffer)
{
	DWORD_PTR *pIATAddress = 0;

	pIATAddress = (DWORD_PTR *)((startAddress - baseAddress) + (DWORD_PTR)dataBuffer);

	while((DWORD_PTR)pIATAddress != (DWORD_PTR)dataBuffer)
	{
		if ( (*pIATAddress < 0xFFFF) || !isAddressAccessable(*pIATAddress) )
		{
			if ( (*(pIATAddress - 1) < 0xFFFF) || !isAddressAccessable(*(pIATAddress - 1)) )
			{
				//IAT end

				if ((DWORD_PTR)(pIATAddress - 2) >= (DWORD_PTR)dataBuffer)
				{
					if (!isApiAddressValid(*(pIATAddress - 2)))
					{
						return (((DWORD_PTR)pIATAddress - (DWORD_PTR)dataBuffer) + baseAddress);
					}
				}
				else
				{
					return (((DWORD_PTR)pIATAddress - (DWORD_PTR)dataBuffer) + baseAddress);
				}
			}
		}

		pIATAddress--;
	}

	return baseAddress;
}

DWORD IATSearch::findIATSize(DWORD_PTR baseAddress, DWORD_PTR iatAddress, BYTE * dataBuffer, DWORD bufferSize)
{
	DWORD_PTR *pIATAddress = 0;

	pIATAddress = (DWORD_PTR *)((iatAddress - baseAddress) + (DWORD_PTR)dataBuffer);

#ifdef DEBUG_COMMENTS
	Logger::debugLog("findIATSize :: baseAddress %X iatAddress %X dataBuffer %X pIATAddress %X\r\n",baseAddress,iatAddress, dataBuffer,pIATAddress);
#endif

	while((DWORD_PTR)pIATAddress < ((DWORD_PTR)dataBuffer + bufferSize - 1))
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog("findIATSize :: %X %X %X\r\n",pIATAddress,*pIATAddress, *(pIATAddress + 1));
#endif
		if ( (*pIATAddress < 0xFFFF) || !isAddressAccessable(*pIATAddress) ) //normal is 0
		{
			if ( (*(pIATAddress + 1) < 0xFFFF) || !isAddressAccessable(*(pIATAddress + 1)) )
			{
				//IAT end
				if (!isApiAddressValid(*(pIATAddress + 2)))
				{
					return (DWORD)((DWORD_PTR)pIATAddress - (DWORD_PTR)dataBuffer - (iatAddress - baseAddress));
				}
			}
		}

		pIATAddress++;
	}

	return bufferSize;
}

bool IATSearch::isAddressAccessable(DWORD_PTR address)
{
	BYTE junk[3];
	SIZE_T numberOfBytesRead = 0;

	if (ReadProcessMemory(hProcess, (LPCVOID)address, junk, sizeof(junk), &numberOfBytesRead))
	{
		if (numberOfBytesRead == sizeof(junk))
		{
			if (junk[0] != 0x00)
			{
				return true;
			}
		}
	}

	return false;
}