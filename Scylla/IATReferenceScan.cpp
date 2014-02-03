#include "IATReferenceScan.h"
#include "Scylla.h"
#include "Architecture.h"

//#define DEBUG_COMMENTS


int IATReferenceScan::numberOfFoundDirectImports()
{
	return (int)iatDirectImportList.size();
}

void IATReferenceScan::startScan(DWORD_PTR imageBase, DWORD imageSize, DWORD_PTR iatAddress, DWORD iatSize)
{
	MEMORY_BASIC_INFORMATION memBasic = {0};

	IatAddressVA = iatAddress;
	IatSize = iatSize;
	ImageBase = imageBase;
	ImageSize = imageSize;

	iatReferenceList.clear();
	iatDirectImportList.clear();

	iatReferenceList.reserve(200);
	iatDirectImportList.reserve(50);

	DWORD_PTR section = imageBase;

	do
	{
		if (!VirtualQueryEx(ProcessAccessHelp::hProcess, (LPCVOID)section, &memBasic, sizeof(MEMORY_BASIC_INFORMATION)))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"VirtualQueryEx failed %d", GetLastError());
#endif

			break;
		}
		else
		{
			if (isPageExecutable(memBasic.Protect))
			{
				//do read and scan
				scanMemoryPage(memBasic.BaseAddress, memBasic.RegionSize);
			}
		}

		section = (DWORD_PTR)((SIZE_T)section + memBasic.RegionSize);

	} while (section < (imageBase + imageSize));


}

//void IATReferenceScan::patchNewIatBaseMemory(DWORD_PTR newIatBaseAddress)
//{
//	NewIatAddressVA = newIatBaseAddress;
//
//	for (std::vector<IATReference>::iterator iter = iatReferenceList.begin(); iter != iatReferenceList.end(); iter++)
//	{
//		patchReferenceInMemory(&(*iter));
//	}
//}
//
//void IATReferenceScan::patchNewIatBaseFile(DWORD_PTR newIatBaseAddress)
//{
//	NewIatAddressVA = newIatBaseAddress;
//
//	for (std::vector<IATReference>::iterator iter = iatReferenceList.begin(); iter != iatReferenceList.end(); iter++)
//	{
//		patchReferenceInFile(&(*iter));
//	}
//}

void IATReferenceScan::patchDirectImportsMemory()
{
	for (std::vector<IATReference>::iterator iter = iatDirectImportList.begin(); iter != iatDirectImportList.end(); iter++)
	{
		patchDirectImportInMemory(&(*iter));
	}
}


bool IATReferenceScan::isPageExecutable( DWORD Protect )
{
	if (Protect & PAGE_NOCACHE) Protect ^= PAGE_NOCACHE;

	if (Protect & PAGE_WRITECOMBINE) Protect ^= PAGE_WRITECOMBINE;

	switch(Protect)
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

void IATReferenceScan::scanMemoryPage( PVOID BaseAddress, SIZE_T RegionSize )
{
	BYTE * dataBuffer = (BYTE *)calloc(RegionSize, 1);
	BYTE * currentPos = dataBuffer;
	int currentSize = (int)RegionSize;
	DWORD_PTR currentOffset = (DWORD_PTR)BaseAddress;
	_DecodeResult res;
	unsigned int instructionsCount = 0, next = 0;

	if (!dataBuffer)
		return;

	if (ProcessAccessHelp::readMemoryFromProcess((DWORD_PTR)BaseAddress, RegionSize, (LPVOID)dataBuffer))
	{
		while (1)
		{
			ZeroMemory(&ProcessAccessHelp::decomposerCi, sizeof(_CodeInfo));
			ProcessAccessHelp::decomposerCi.code = currentPos;
			ProcessAccessHelp::decomposerCi.codeLen = currentSize;
			ProcessAccessHelp::decomposerCi.dt = ProcessAccessHelp::dt;
			ProcessAccessHelp::decomposerCi.codeOffset = currentOffset;

			instructionsCount = 0;

			res = distorm_decompose(&ProcessAccessHelp::decomposerCi, ProcessAccessHelp::decomposerResult, sizeof(ProcessAccessHelp::decomposerResult)/sizeof(ProcessAccessHelp::decomposerResult[0]), &instructionsCount);

			if (res == DECRES_INPUTERR)
			{
				break;
			}

			for (unsigned int i = 0; i < instructionsCount; i++) 
			{
				if (ProcessAccessHelp::decomposerResult[i].flags != FLAG_NOT_DECODABLE)
				{
					analyzeInstruction(&ProcessAccessHelp::decomposerResult[i]);
				}
			}

			if (res == DECRES_SUCCESS) break; // All instructions were decoded.
			else if (instructionsCount == 0) break;

			next = (unsigned long)(ProcessAccessHelp::decomposerResult[instructionsCount-1].addr - ProcessAccessHelp::decomposerResult[0].addr);

			if (ProcessAccessHelp::decomposerResult[instructionsCount-1].flags != FLAG_NOT_DECODABLE)
			{
				next += ProcessAccessHelp::decomposerResult[instructionsCount-1].size;
			}

			currentPos += next;
			currentOffset += next;
			currentSize -= next;
		}
	}

	free(dataBuffer);
}

void IATReferenceScan::analyzeInstruction( _DInst * instruction )
{
	if (ScanForNormalImports)
	{
		findNormalIatReference(instruction);
	}
	
	if (ScanForDirectImports)
	{
		findDirectIatReferenceCallJmp(instruction);
		findDirectIatReferenceMov(instruction);
	}
}

void IATReferenceScan::findNormalIatReference( _DInst * instruction )
{
#ifdef DEBUG_COMMENTS
	_DecodedInst inst;
#endif

	IATReference ref;


	if (META_GET_FC(instruction->meta) == FC_CALL || META_GET_FC(instruction->meta) == FC_UNC_BRANCH)
	{
		if (instruction->size >= 5)
		{
			if (META_GET_FC(instruction->meta) == FC_CALL)
			{
				ref.type = IAT_REFERENCE_PTR_CALL;
			}
			else
			{
				ref.type = IAT_REFERENCE_PTR_JMP;
			}
			ref.addressVA = (DWORD_PTR)instruction->addr;

#ifdef _WIN64
			if (instruction->flags & FLAG_RIP_RELATIVE)
			{

#ifdef DEBUG_COMMENTS
				distorm_format(&ProcessAccessHelp::decomposerCi, instruction, &inst);
				Scylla::debugLog.log(PRINTF_DWORD_PTR_FULL L" " PRINTF_DWORD_PTR_FULL L" %S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL, (DWORD_PTR)instruction->addr, ImageBase, inst.mnemonic.p, inst.operands.p, instruction->ops[0].type, instruction->size, INSTRUCTION_GET_RIP_TARGET(instruction));
#endif

				if (INSTRUCTION_GET_RIP_TARGET(instruction) >= IatAddressVA && INSTRUCTION_GET_RIP_TARGET(instruction) < (IatAddressVA + IatSize))
				{
					ref.targetPointer = INSTRUCTION_GET_RIP_TARGET(instruction);

					getIatEntryAddress(&ref);

					//Scylla::debugLog.log(L"iat entry "PRINTF_DWORD_PTR_FULL,ref.targetAddressInIat);

					iatReferenceList.push_back(ref);
				}
			}
#else

			if (instruction->ops[0].type == O_DISP)
			{
				//jmp dword ptr || call dword ptr
#ifdef DEBUG_COMMENTS
				distorm_format(&ProcessAccessHelp::decomposerCi, instruction, &inst);
				Scylla::debugLog.log(PRINTF_DWORD_PTR_FULL L" " PRINTF_DWORD_PTR_FULL L" %S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL, (DWORD_PTR)instruction->addr, ImageBase, inst.mnemonic.p, inst.operands.p, instruction->ops[0].type, instruction->size, instruction->disp);
#endif
				
				if (instruction->disp >= IatAddressVA && instruction->disp < (IatAddressVA + IatSize))
				{
					ref.targetPointer = (DWORD_PTR)instruction->disp;
					
					getIatEntryAddress(&ref);

					//Scylla::debugLog.log(L"iat entry "PRINTF_DWORD_PTR_FULL,ref.targetAddressInIat);

					iatReferenceList.push_back(ref);
				}
			}
#endif
		}
	}
}

void IATReferenceScan::getIatEntryAddress( IATReference * ref )
{
	if (!ProcessAccessHelp::readMemoryFromProcess(ref->targetPointer, sizeof(DWORD_PTR), &ref->targetAddressInIat))
	{
		ref->targetAddressInIat = 0;
	}
}

void IATReferenceScan::findDirectIatReferenceCallJmp( _DInst * instruction )
{
#ifdef DEBUG_COMMENTS
	_DecodedInst inst;
#endif

	IATReference ref;
	ref.targetPointer = 0;
	

	if (META_GET_FC(instruction->meta) == FC_CALL || META_GET_FC(instruction->meta) == FC_UNC_BRANCH)
	{
		if ((instruction->size >= 5) && (instruction->ops[0].type == O_PC)) //CALL/JMP 0x00000000
		{
			if (META_GET_FC(instruction->meta) == FC_CALL)
			{
				ref.type = IAT_REFERENCE_DIRECT_CALL;
			}
			else
			{
				ref.type = IAT_REFERENCE_DIRECT_JMP;
			}
			ref.addressVA = (DWORD_PTR)instruction->addr;
			ref.targetAddressInIat = (DWORD_PTR)INSTRUCTION_GET_TARGET(instruction);

			if ((ref.targetAddressInIat < ImageBase) || (ref.targetAddressInIat > (ImageBase+ImageSize))) //call/JMP outside pe image
			{
				if (isAddressValidImageMemory(ref.targetAddressInIat))
				{
#ifdef DEBUG_COMMENTS
					distorm_format(&ProcessAccessHelp::decomposerCi, instruction, &inst);
					Scylla::debugLog.log(PRINTF_DWORD_PTR_FULL L" " PRINTF_DWORD_PTR_FULL L" %S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL,(DWORD_PTR)instruction->addr, ImageBase, inst.mnemonic.p, inst.operands.p, instruction->ops[0].type, instruction->size, INSTRUCTION_GET_TARGET(instruction));
#endif

					iatDirectImportList.push_back(ref);
				}
			}
		}
	}
}

bool IATReferenceScan::isAddressValidImageMemory( DWORD_PTR address )
{
	MEMORY_BASIC_INFORMATION memBasic = {0};

	if (!VirtualQueryEx(ProcessAccessHelp::hProcess, (LPCVOID)address, &memBasic, sizeof(MEMORY_BASIC_INFORMATION)))
	{
		return false;
	}

	return (memBasic.Type == MEM_IMAGE && isPageExecutable(memBasic.Protect));
}

void IATReferenceScan::patchReferenceInMemory( IATReference * ref )
{
	DWORD_PTR newIatAddressPointer = ref->targetPointer - IatAddressVA + NewIatAddressRVA;

	DWORD patchBytes = 0;

#ifdef _WIN64
	patchBytes = (DWORD)(newIatAddressPointer - ref->addressVA - 6);
#else
	patchBytes = newIatAddressPointer;
#endif
	ProcessAccessHelp::writeMemoryToProcess(ref->addressVA + 2, sizeof(DWORD), &patchBytes);
}

void IATReferenceScan::patchDirectImportInMemory( IATReference * ref )
{
	DWORD patchBytes = 0;
	BYTE patchPreBytes[2];

	ref->targetPointer = lookUpIatForPointer(ref->targetAddressInIat);
	if (ref->targetPointer)
	{
		patchPreBytes[0] = 0xFF;

		if (ref->type == IAT_REFERENCE_DIRECT_CALL) //FF15
		{
			patchPreBytes[1] = 0x15;
		}
		else if (ref->type == IAT_REFERENCE_DIRECT_JMP) //FF25
		{
			patchPreBytes[1] = 0x25;
		}
		else
		{
			return;
		}

		ProcessAccessHelp::writeMemoryToProcess(ref->addressVA, 2, patchPreBytes);

#ifdef _WIN64
		patchBytes = (DWORD)(ref->targetPointer - ref->addressVA - 6);
#else
		patchBytes = ref->targetPointer;
#endif
		ProcessAccessHelp::writeMemoryToProcess(ref->addressVA + 2, sizeof(DWORD), &patchBytes);
	}
}

DWORD_PTR IATReferenceScan::lookUpIatForPointer( DWORD_PTR addr )
{
	if (!iatBackup)
	{
		iatBackup = (DWORD_PTR *)calloc(IatSize + sizeof(DWORD_PTR), 1);
		if (!iatBackup)
		{
			return 0;
		}
		if (!ProcessAccessHelp::readMemoryFromProcess(IatAddressVA, IatSize, iatBackup))
		{
			free(iatBackup);
			iatBackup = 0;
			return 0;
		}
	}

	for (int i = 0; i < ((int)IatSize / (int)sizeof(DWORD_PTR));i++)
	{
		if (iatBackup[i] == addr)
		{
			return (DWORD_PTR)&iatBackup[i] - (DWORD_PTR)iatBackup + IatAddressVA;
		}
	}

	return 0;
}

void IATReferenceScan::patchNewIat(DWORD_PTR stdImagebase, DWORD_PTR newIatBaseAddress, PeParser * peParser)
{
	NewIatAddressRVA = newIatBaseAddress;
	DWORD patchBytes = 0;

	for (std::vector<IATReference>::iterator iter = iatReferenceList.begin(); iter != iatReferenceList.end(); iter++)
	{
		IATReference * ref = &(*iter);

		DWORD_PTR newIatAddressPointer = (ref->targetPointer - IatAddressVA) + NewIatAddressRVA + stdImagebase;

#ifdef _WIN64
		patchBytes = (DWORD)(newIatAddressPointer - (ref->addressVA - ImageBase + stdImagebase) - 6);
#else
		patchBytes = newIatAddressPointer;
#endif
		DWORD_PTR patchOffset = peParser->convertRVAToOffsetRelative(ref->addressVA - ImageBase);
		int index = peParser->convertRVAToOffsetVectorIndex(ref->addressVA - ImageBase);
		BYTE * memory = peParser->getSectionMemoryByIndex(index);
		DWORD memorySize = peParser->getSectionMemorySizeByIndex(index);


		if (memorySize < (DWORD)(patchOffset + 6))
		{
			Scylla::debugLog.log(L"Error - Cannot fix IAT reference RVA: " PRINTF_DWORD_PTR_FULL, ref->addressVA - ImageBase);
		}
		else
		{
			memory += patchOffset + 2;		

			*((DWORD *)memory) = patchBytes;
		}
		//Scylla::debugLog.log(L"address %X old %X new %X",ref->addressVA, ref->targetPointer, newIatAddressPointer);

	}
}

void IATReferenceScan::findDirectIatReferenceMov( _DInst * instruction )
{
#ifdef DEBUG_COMMENTS
	_DecodedInst inst;
#endif

	IATReference ref;
	ref.targetPointer = 0;
	ref.type = IAT_REFERENCE_DIRECT_MOV;

	if (instruction->size >= 5 && instruction->opcode == I_MOV)
	{
		//MOV REGISTER, 0xFFFFFFFF
		if (instruction->ops[0].type == O_REG && instruction->ops[1].type == O_IMM)
		{
			ref.targetAddressInIat = (DWORD_PTR)instruction->imm.qword;
			ref.addressVA = (DWORD_PTR)instruction->addr;

			if (ref.targetAddressInIat > 0x000FFFFF && ref.targetAddressInIat != (DWORD_PTR)-1)
			{
				if ((ref.targetAddressInIat < ImageBase) || (ref.targetAddressInIat > (ImageBase+ImageSize)))
				{
					if (isAddressValidImageMemory(ref.targetAddressInIat))
					{
#ifdef DEBUG_COMMENTS
						distorm_format(&ProcessAccessHelp::decomposerCi, instruction, &inst);
						Scylla::debugLog.log(PRINTF_DWORD_PTR_FULL L" " PRINTF_DWORD_PTR_FULL L" %S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL,(DWORD_PTR)instruction->addr, ImageBase, inst.mnemonic.p, inst.operands.p, instruction->ops[0].type, instruction->size, INSTRUCTION_GET_TARGET(instruction));
#endif
						iatDirectImportList.push_back(ref);
					}
				}
			}
		}
	}
}
