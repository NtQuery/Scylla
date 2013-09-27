#include "IATSearch.h"
#include "Scylla.h"
#include "Architecture.h"


#define DEBUG_COMMENTS

bool IATSearch::searchImportAddressTableInProcess( DWORD_PTR startAddress, DWORD_PTR* addressIAT, DWORD* sizeIAT, bool advanced )
{
	DWORD_PTR addressInIAT = 0;

	*addressIAT = 0;
	*sizeIAT = 0;

	if (advanced)
	{
		return findIATAdvanced(startAddress, addressIAT, sizeIAT);
	}
	
	addressInIAT = findAPIAddressInIAT(startAddress);
	

	if(!addressInIAT)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"searchImportAddressTableInProcess :: addressInIAT not found, startAddress " PRINTF_DWORD_PTR_FULL, startAddress);
#endif
		return false;
	}
	else
	{
		return findIATStartAndSize(addressInIAT, addressIAT,sizeIAT);
	}
}

bool IATSearch::findIATAdvanced( DWORD_PTR startAddress, DWORD_PTR* addressIAT, DWORD* sizeIAT )
{
	BYTE *dataBuffer;
	DWORD_PTR baseAddress;
	SIZE_T memorySize;

	findExecutableMemoryPagesByStartAddress(startAddress, &baseAddress, &memorySize);

	if (memorySize == 0)
		return false;

	dataBuffer = new BYTE[memorySize];

	if (!readMemoryFromProcess((DWORD_PTR)baseAddress, memorySize,dataBuffer))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"findAPIAddressInIAT2 :: error reading memory");
#endif
		return false;
	}

	std::set<DWORD_PTR> iatPointers;
	DWORD_PTR next;
	BYTE * tempBuf = dataBuffer;
	while(decomposeMemory(tempBuf, memorySize, (DWORD_PTR)baseAddress) && decomposerInstructionsCount != 0)
	{
		findIATPointers(iatPointers);

		next = (DWORD_PTR)(decomposerResult[decomposerInstructionsCount - 1].addr - baseAddress);
		next += decomposerResult[decomposerInstructionsCount - 1].size;
		// Advance ptr and recalc offset.
		tempBuf += next;

		if (memorySize <= next)
		{
			break;
		}
		memorySize -= next;
		baseAddress += next;
	}

	if (iatPointers.size() == 0)
		return false;

	filterIATPointersList(iatPointers);

	*addressIAT = *(iatPointers.begin());
	*sizeIAT = *(--iatPointers.end()) - *(iatPointers.begin()) + sizeof(DWORD_PTR);

	Scylla::windowLog.log(L"IAT Search Advanced: Found %d (0x%X) possible IAT entries.", iatPointers.size(), iatPointers.size());
	Scylla::windowLog.log(L"IAT Search Advanced: Possible IAT first " PRINTF_DWORD_PTR_FULL L" last " PRINTF_DWORD_PTR_FULL L" entry.", *(iatPointers.begin()), *(--iatPointers.end()));

	delete [] dataBuffer;

	return true;
}

DWORD_PTR IATSearch::findAPIAddressInIAT(DWORD_PTR startAddress)
{
	const size_t MEMORY_READ_SIZE = 200;
	BYTE dataBuffer[MEMORY_READ_SIZE];

	DWORD_PTR iatPointer = 0;
	int counter = 0;

	// to detect stolen api
	memoryAddress = 0;
	memorySize = 0;

	do 
	{
		counter++;

		if (!readMemoryFromProcess(startAddress, sizeof(dataBuffer), dataBuffer))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"findAPIAddressInIAT :: error reading memory " PRINTF_DWORD_PTR_FULL, startAddress);
#endif
			return 0;
		}

		if (decomposeMemory(dataBuffer, sizeof(dataBuffer), startAddress))
		{
			iatPointer = findIATPointer();
			if (iatPointer)
			{
				if (isIATPointerValid(iatPointer))
				{
					return iatPointer;
				}
			}
		}

		startAddress = findNextFunctionAddress();
		//printf("startAddress %08X\n",startAddress);
	} while (startAddress != 0 && counter != 8);

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
						Scylla::debugLog.log(L"%S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL, inst.mnemonic.p, inst.operands.p, decomposerResult[i].ops[0].type, decomposerResult[i].size, INSTRUCTION_GET_TARGET(&decomposerResult[i]));
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
						Scylla::debugLog.log(L"%S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL, inst.mnemonic.p, inst.operands.p, decomposerResult[i].ops[0].type, decomposerResult[i].size, INSTRUCTION_GET_RIP_TARGET(&decomposerResult[i]));
#endif
						return INSTRUCTION_GET_RIP_TARGET(&decomposerResult[i]);
					}
#else
					if (decomposerResult[i].ops[0].type == O_DISP)
					{
						//jmp dword ptr || call dword ptr
#ifdef DEBUG_COMMENTS
						distorm_format(&decomposerCi, &decomposerResult[i], &inst);
						Scylla::debugLog.log(L"%S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL, inst.mnemonic.p, inst.operands.p, decomposerResult[i].ops[0].type, decomposerResult[i].size, decomposerResult[i].disp);
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

bool IATSearch::isIATPointerValid(DWORD_PTR iatPointer)
{
	DWORD_PTR apiAddress = 0;

	if (!readMemoryFromProcess(iatPointer,sizeof(DWORD_PTR),&apiAddress))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"isIATPointerValid :: error reading memory");
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

bool IATSearch::isPageExecutable(DWORD value)
{
	if (value & PAGE_NOCACHE) value ^= PAGE_NOCACHE;

	if (value & PAGE_WRITECOMBINE) value ^= PAGE_WRITECOMBINE;

	switch(value)
	{
	case PAGE_EXECUTE:
		{
			return true;
		}
	case PAGE_EXECUTE_READ:
		{
			return true;
		}
	case PAGE_EXECUTE_READWRITE:
		{
			return true;
		}
	case PAGE_EXECUTE_WRITECOPY:
		{
			return true;
		}
	default:
		return false;
	}
}

bool IATSearch::findIATStartAndSize(DWORD_PTR address, DWORD_PTR * addressIAT, DWORD * sizeIAT)
{
	MEMORY_BASIC_INFORMATION memBasic = {0};
	BYTE *dataBuffer = 0;

	if (VirtualQueryEx(hProcess,(LPCVOID)address, &memBasic, sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"findIATStartAddress :: VirtualQueryEx error %u", GetLastError());
#endif
		return false;
	}


	//(sizeof(DWORD_PTR) * 3) added to prevent buffer overflow
	dataBuffer = new BYTE[memBasic.RegionSize + (sizeof(DWORD_PTR) * 3)];

	ZeroMemory(dataBuffer, memBasic.RegionSize + (sizeof(DWORD_PTR) * 3));

	if (!readMemoryFromProcess((DWORD_PTR)memBasic.BaseAddress, memBasic.RegionSize, dataBuffer))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"findIATStartAddress :: error reading memory");
#endif
		return false;
	}

	//printf("address %X memBasic.BaseAddress %X memBasic.RegionSize %X\n",address,memBasic.BaseAddress,memBasic.RegionSize);

	*addressIAT = findIATStartAddress((DWORD_PTR)memBasic.BaseAddress, address, dataBuffer);

	*sizeIAT = findIATSize((DWORD_PTR)memBasic.BaseAddress, *addressIAT, dataBuffer, (DWORD)memBasic.RegionSize);

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
	Scylla::debugLog.log(L"findIATSize :: baseAddress %X iatAddress %X dataBuffer %X pIATAddress %X", baseAddress, iatAddress, dataBuffer, pIATAddress);
#endif

	while((DWORD_PTR)pIATAddress < ((DWORD_PTR)dataBuffer + bufferSize - 1))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"findIATSize :: %X %X %X", pIATAddress, *pIATAddress, *(pIATAddress + 1));
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

void IATSearch::findIATPointers(std::set<DWORD_PTR> & iatPointers)
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
						Scylla::debugLog.log(L"%S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL, inst.mnemonic.p, inst.operands.p, decomposerResult[i].ops[0].type, decomposerResult[i].size, INSTRUCTION_GET_RIP_TARGET(&decomposerResult[i]));
#endif
						iatPointers.insert(INSTRUCTION_GET_RIP_TARGET(&decomposerResult[i]));
					}
#else
					if (decomposerResult[i].ops[0].type == O_DISP)
					{
						//jmp dword ptr || call dword ptr
#ifdef DEBUG_COMMENTS
						distorm_format(&decomposerCi, &decomposerResult[i], &inst);
						Scylla::debugLog.log(L"%S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL, inst.mnemonic.p, inst.operands.p, decomposerResult[i].ops[0].type, decomposerResult[i].size, decomposerResult[i].disp);
#endif
						iatPointers.insert((DWORD_PTR)decomposerResult[i].disp);
					}
#endif
				}
			}
		}
	}


}

void IATSearch::findExecutableMemoryPagesByStartAddress( DWORD_PTR startAddress, DWORD_PTR* baseAddress, SIZE_T* memorySize )
{
	MEMORY_BASIC_INFORMATION memBasic = {0};
	DWORD_PTR tempAddress;

	*memorySize = 0;
	*baseAddress = 0;

	if (VirtualQueryEx(hProcess,(LPCVOID)startAddress, &memBasic, sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"findIATStartAddress :: VirtualQueryEx error %u", GetLastError());
#endif
		return;
	}

	//search down
	do
	{
		*memorySize = memBasic.RegionSize;
		*baseAddress = (DWORD_PTR)memBasic.BaseAddress;
		tempAddress = (DWORD_PTR)memBasic.BaseAddress - 1;

		if (VirtualQueryEx(hProcess, (LPCVOID)tempAddress, &memBasic, sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
		{
			break;
		}
	} while (isPageExecutable(memBasic.Protect));

	tempAddress = *baseAddress;
	memBasic.RegionSize = *memorySize;
	*memorySize = 0;
	//search up
	do
	{
		tempAddress += memBasic.RegionSize;
		*memorySize += memBasic.RegionSize;

		if (VirtualQueryEx(hProcess, (LPCVOID)tempAddress, &memBasic, sizeof(MEMORY_BASIC_INFORMATION)) != sizeof(MEMORY_BASIC_INFORMATION))
		{
			break;
		}
	} while (isPageExecutable(memBasic.Protect));
}

void IATSearch::filterIATPointersList( std::set<DWORD_PTR> & iatPointers )
{
	std::set<DWORD_PTR>::iterator iter;
	iter = iatPointers.begin();
	std::advance(iter, iatPointers.size() / 2); //start in the middle, important!

	DWORD_PTR lastPointer = *iter;
	iter++;

	for (; iter != iatPointers.end(); iter++)
	{
		if ((*iter - lastPointer) > 0x100) //check difference
		{
			iatPointers.erase(iter, iatPointers.end());
			break;
		}
		else
		{
			lastPointer = *iter;
		}
	}


	bool erased = true;

	while(erased)
	{
		iter = iatPointers.begin();
		lastPointer = *iter;
		iter++;

		for (; iter != iatPointers.end(); iter++)
		{
			if ((*iter - lastPointer) > 0x100) //check difference
			{
				iter--;
				iatPointers.erase(iter);
				erased = true;
				break;
			}
			else
			{
				erased = false;
				lastPointer = *iter;
			}
		}
	}

}
