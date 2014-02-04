#include "IATReferenceScan.h"
#include "Scylla.h"
#include "Architecture.h"

//#define DEBUG_COMMENTS


FileLog IATReferenceScan::directImportLog(L"Scylla_direct_imports.log");

int IATReferenceScan::numberOfFoundDirectImports()
{
	return (int)iatDirectImportList.size();
}

int IATReferenceScan::getSizeInBytesOfJumpTableInSection()
{
	return ((int)iatDirectImportList.size() * 6); //for x86 and x64 the same size
}

void IATReferenceScan::startScan(DWORD_PTR imageBase, DWORD imageSize, DWORD_PTR iatAddress, DWORD iatSize)
{
	MEMORY_BASIC_INFORMATION memBasic = {0};

	IatAddressVA = iatAddress;
	IatSize = iatSize;
	ImageBase = imageBase;
	ImageSize = imageSize;

	if (ScanForNormalImports)
	{
		iatReferenceList.clear();
		iatReferenceList.reserve(200);
	}
	if (ScanForDirectImports)
	{
		iatDirectImportList.clear();
		iatDirectImportList.reserve(50);
	}



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

void IATReferenceScan::patchDirectImportsMemory( bool junkByteAfterInstruction )
{
	JunkByteAfterInstruction = junkByteAfterInstruction;
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
		findDirectIatReferenceMov(instruction);
		
#ifndef _WIN64
		findDirectIatReferenceCallJmp(instruction);
		findDirectIatReferenceLea(instruction);
		findDirectIatReferencePush(instruction);
#endif
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
			ref.instructionSize = instruction->size;

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
			ref.instructionSize = instruction->size;

			checkMemoryRangeAndAddToList(&ref, instruction);
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

		if (!JunkByteAfterInstruction)
		{
			ref->addressVA -= 1;
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
	IATReference ref;
	ref.targetPointer = 0;
	ref.type = IAT_REFERENCE_DIRECT_MOV;

	if (instruction->opcode == I_MOV)
	{
#ifdef _WIN64
		if (instruction->size >= 7) //MOV REGISTER, 0xFFFFFFFFFFFFFFFF
#else
		if (instruction->size >= 5) //MOV REGISTER, 0xFFFFFFFF
#endif
		{
			if (instruction->ops[0].type == O_REG && instruction->ops[1].type == O_IMM)
			{
				ref.targetAddressInIat = (DWORD_PTR)instruction->imm.qword;
				ref.addressVA = (DWORD_PTR)instruction->addr;
				ref.instructionSize = instruction->size;

				checkMemoryRangeAndAddToList(&ref, instruction);
			}
		}
	}
}

void IATReferenceScan::printDirectImportLog()
{
	IATReferenceScan::directImportLog.log(L"------------------------------------------------------------");
	IATReferenceScan::directImportLog.log(L"ImageBase " PRINTF_DWORD_PTR_FULL L" ImageSize %08X IATAddress " PRINTF_DWORD_PTR_FULL L" IATSize 0x%X", ImageBase, ImageSize, IatAddressVA, IatSize);
	int count = 0;
	bool isSuspect = false;

	for (std::vector<IATReference>::iterator iter = iatDirectImportList.begin(); iter != iatDirectImportList.end(); iter++)
	{
		IATReference * ref = &(*iter);
		
		ApiInfo * apiInfo = apiReader->getApiByVirtualAddress(ref->targetAddressInIat, &isSuspect);

		ref->targetPointer = lookUpIatForPointer(ref->targetAddressInIat);

		count++;
		WCHAR * type = L"U";

		if (ref->type == IAT_REFERENCE_DIRECT_CALL)
		{
			type = L"CALL";
		}
		else if (ref->type == IAT_REFERENCE_DIRECT_JMP)
		{
			type = L"JMP";
		}
		else if (ref->type == IAT_REFERENCE_DIRECT_MOV)
		{
			type = L"MOV";
		}
		else if (ref->type == IAT_REFERENCE_DIRECT_PUSH)
		{
			type = L"PUSH";
		}
		else if (ref->type == IAT_REFERENCE_DIRECT_LEA)
		{
			type = L"LEA";
		}

		IATReferenceScan::directImportLog.log(L"%04d AddrVA " PRINTF_DWORD_PTR_FULL L" Type %s Value " PRINTF_DWORD_PTR_FULL L" IatRefPointer " PRINTF_DWORD_PTR_FULL L" Api %s %S", count, ref->addressVA, type, ref->targetAddressInIat, ref->targetPointer,apiInfo->module->getFilename(), apiInfo->name);

	}

	IATReferenceScan::directImportLog.log(L"------------------------------------------------------------");
}


void IATReferenceScan::findDirectIatReferencePush( _DInst * instruction )
{
	IATReference ref;
	ref.targetPointer = 0;
	ref.type = IAT_REFERENCE_DIRECT_PUSH;

	if (instruction->size >= 5 && instruction->opcode == I_PUSH)
	{
		ref.targetAddressInIat = (DWORD_PTR)instruction->imm.qword;
		ref.addressVA = (DWORD_PTR)instruction->addr;
		ref.instructionSize = instruction->size;

		checkMemoryRangeAndAddToList(&ref, instruction);
	}
}

void IATReferenceScan::findDirectIatReferenceLea( _DInst * instruction )
{
	IATReference ref;
	ref.targetPointer = 0;
	ref.type = IAT_REFERENCE_DIRECT_LEA;

	if (instruction->size >= 5 && instruction->opcode == I_LEA)
	{
		if (instruction->ops[0].type == O_REG && instruction->ops[1].type == O_DISP) //LEA EDX, [0xb58bb8]
		{
			ref.targetAddressInIat = (DWORD_PTR)instruction->disp;
			ref.addressVA = (DWORD_PTR)instruction->addr;
			ref.instructionSize = instruction->size;

			checkMemoryRangeAndAddToList(&ref, instruction);
		}
	}
}

void IATReferenceScan::checkMemoryRangeAndAddToList( IATReference * ref, _DInst * instruction )
{
#ifdef DEBUG_COMMENTS
	_DecodedInst inst;
#endif

	if (ref->targetAddressInIat > 0x000FFFFF && ref->targetAddressInIat != (DWORD_PTR)-1)
	{
		if ((ref->targetAddressInIat < ImageBase) || (ref->targetAddressInIat > (ImageBase+ImageSize))) //outside pe image
		{
			//if (isAddressValidImageMemory(ref->targetAddressInIat))
			{
				bool isSuspect = false;
				if (apiReader->getApiByVirtualAddress(ref->targetAddressInIat, &isSuspect) != 0)
				{
#ifdef DEBUG_COMMENTS
					distorm_format(&ProcessAccessHelp::decomposerCi, instruction, &inst);
					Scylla::debugLog.log(PRINTF_DWORD_PTR_FULL L" " PRINTF_DWORD_PTR_FULL L" %S %S %d %d - target address: " PRINTF_DWORD_PTR_FULL,(DWORD_PTR)instruction->addr, ImageBase, inst.mnemonic.p, inst.operands.p, instruction->ops[0].type, instruction->size, ref->targetAddressInIat);
#endif
					iatDirectImportList.push_back(*ref);
				}
			}
		}
	}
}

void IATReferenceScan::patchDirectJumpTable( DWORD_PTR stdImagebase, DWORD directImportsJumpTableRVA, PeParser * peParser, BYTE * jmpTableMemory, DWORD newIatBase )
{
	DWORD patchBytes = 0;
	for (std::vector<IATReference>::iterator iter = iatDirectImportList.begin(); iter != iatDirectImportList.end(); iter++)
	{
		IATReference * ref = &(*iter);

		DWORD_PTR refTargetPointer = ref->targetPointer;
		if (newIatBase) //create new iat in section
		{
			refTargetPointer = (ref->targetPointer - IatAddressVA) + newIatBase + ImageBase;
		}
		//create jump table in section
		DWORD_PTR newIatAddressPointer = refTargetPointer - ImageBase + stdImagebase;

#ifdef _WIN64
		patchBytes = (DWORD)(newIatAddressPointer - (ref->addressVA - ImageBase + stdImagebase) - 6);
#else
		patchBytes = newIatAddressPointer; //dont forget relocation here
		directImportLog.log(L"Relocation direct imports fix: Base RVA %08X Offset %04X Type IMAGE_REL_BASED_HIGHLOW", (directImportsJumpTableRVA + 2) & 0xFFFFF000, (directImportsJumpTableRVA + 2) & 0x00000FFF);
#endif
		jmpTableMemory[0] = 0xFF;
		jmpTableMemory[1] = 0x25;
		*((DWORD *)&jmpTableMemory[2]) = patchBytes;

		//patch dump
		DWORD_PTR patchOffset = peParser->convertRVAToOffsetRelative(ref->addressVA - ImageBase);
		int index = peParser->convertRVAToOffsetVectorIndex(ref->addressVA - ImageBase);
		BYTE * memory = peParser->getSectionMemoryByIndex(index);
		DWORD memorySize = peParser->getSectionMemorySizeByIndex(index);
		DWORD sectionRVA = peParser->getSectionAddressRVAByIndex(index);

		if (ref->type == IAT_REFERENCE_DIRECT_CALL || ref->type == IAT_REFERENCE_DIRECT_JMP)
		{
			if (ref->instructionSize == 5)
			{
				patchBytes = directImportsJumpTableRVA - (ref->addressVA - ImageBase) - 5;
				patchDirectImportInDump32(1, 5, patchBytes, memory, memorySize, false, patchOffset, sectionRVA);
			}
		}
		else if (ref->type == IAT_REFERENCE_DIRECT_PUSH || ref->type == IAT_REFERENCE_DIRECT_MOV)
		{
			if (ref->instructionSize == 5)
			{
				patchBytes = directImportsJumpTableRVA + stdImagebase;
				patchDirectImportInDump32(1, 5, patchBytes, memory, memorySize, true, patchOffset, sectionRVA);				
			}
		}
		else if (ref->type == IAT_REFERENCE_DIRECT_LEA)
		{
			if (ref->instructionSize == 6)
			{
				patchBytes = directImportsJumpTableRVA + stdImagebase;
				patchDirectImportInDump32(2, 6, patchBytes, memory, memorySize, true, patchOffset, sectionRVA);
			}
		}


		jmpTableMemory += 6;
		directImportsJumpTableRVA += 6;
	}

}

void IATReferenceScan::patchDirectImportInDump32( int patchPreFixBytes, int instructionSize, DWORD patchBytes, BYTE * memory, DWORD memorySize, bool generateReloc, DWORD patchOffset, DWORD sectionRVA )
{

	if (memorySize < (DWORD)(patchOffset + instructionSize))
	{
		Scylla::debugLog.log(L"Error - Cannot fix direct import reference RVA: %X", sectionRVA + patchOffset);
	}
	else
	{
		memory += patchOffset + patchPreFixBytes;
		if (generateReloc)
		{
			directImportLog.log(L"Relocation direct imports fix: Base RVA %08X Offset %04X Type IMAGE_REL_BASED_HIGHLOW", (sectionRVA + patchOffset + patchPreFixBytes) & 0xFFFFF000, (sectionRVA + patchOffset+ patchPreFixBytes) & 0x00000FFF);
		}

		*((DWORD *)memory) = patchBytes;
	}
}


