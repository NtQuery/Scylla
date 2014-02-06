#pragma once
#include <vector>
#include "ProcessAccessHelp.h"
#include "PeParser.h"
#include "Logger.h"
#include "ApiReader.h"

enum IATReferenceType {
	IAT_REFERENCE_PTR_JMP,
	IAT_REFERENCE_PTR_CALL,
	IAT_REFERENCE_DIRECT_JMP,
	IAT_REFERENCE_DIRECT_CALL,
	IAT_REFERENCE_DIRECT_MOV,
	IAT_REFERENCE_DIRECT_PUSH,
	IAT_REFERENCE_DIRECT_LEA
};

class IATReference
{
public:
	DWORD_PTR addressVA; //Address of reference
	DWORD_PTR targetPointer; //Place inside IAT
	DWORD_PTR targetAddressInIat; //WIN API?
	BYTE instructionSize;
	IATReferenceType type;
};


class IATReferenceScan
{
public:

	IATReferenceScan()
	{
		apiReader = 0;
		IatAddressVA = 0;
		IatSize = 0;
		ImageBase = 0;
		ImageSize = 0;
		iatBackup = 0;
		ScanForDirectImports = false;
		ScanForNormalImports = true;
	}

	~IATReferenceScan()
	{
		iatReferenceList.clear();
		iatDirectImportList.clear();

		if (iatBackup)
		{
			free(iatBackup);
		}
	}

	bool ScanForDirectImports;
	bool ScanForNormalImports;
	bool JunkByteAfterInstruction;
	ApiReader * apiReader;

	void startScan(DWORD_PTR imageBase, DWORD imageSize, DWORD_PTR iatAddress, DWORD iatSize);
	//void patchNewIatBaseMemory(DWORD_PTR newIatBaseAddress);
	//void patchNewIatBaseFile(DWORD_PTR newIatBaseAddress);

	void patchNewIat(DWORD_PTR stdImagebase, DWORD_PTR newIatBaseAddress, PeParser * peParser);
	void patchDirectJumpTable( DWORD_PTR imageBase, DWORD directImportsJumpTableRVA, PeParser * peParser, BYTE * jmpTableMemory, DWORD newIatBase);
	void patchDirectImportsMemory(bool junkByteAfterInstruction);
	int numberOfFoundDirectImports();
	int numberOfFoundUniqueDirectImports();
	int numberOfDirectImportApisNotInIat();
	int getSizeInBytesOfJumpTableInSection();
	static FileLog directImportLog;
	void printDirectImportLog();
	void changeIatBaseOfDirectImports( DWORD newIatBaseAddressRVA );
	DWORD addAdditionalApisToList();
private:
	DWORD_PTR NewIatAddressRVA;

	DWORD_PTR IatAddressVA;
	DWORD IatSize;
	DWORD_PTR ImageBase;
	DWORD ImageSize;


	DWORD_PTR * iatBackup;

	std::vector<IATReference> iatReferenceList;
	std::vector<IATReference> iatDirectImportList;

	bool isPageExecutable( DWORD Protect );
	void scanMemoryPage( PVOID BaseAddress, SIZE_T RegionSize );
	void analyzeInstruction( _DInst * instruction );
	void findNormalIatReference( _DInst * instruction );
	void getIatEntryAddress( IATReference* ref );
	void findDirectIatReferenceCallJmp( _DInst * instruction );
	bool isAddressValidImageMemory( DWORD_PTR address );
	void patchReferenceInMemory( IATReference * ref );
	void patchReferenceInFile( IATReference* ref );
	void patchDirectImportInMemory( IATReference * iter );
	DWORD_PTR lookUpIatForPointer( DWORD_PTR addr );
	void findDirectIatReferenceMov( _DInst * instruction );
	void findDirectIatReferencePush( _DInst * instruction );
	void checkMemoryRangeAndAddToList( IATReference * ref, _DInst * instruction );
	void findDirectIatReferenceLea( _DInst * instruction );
	void patchDirectImportInDump32( int patchPreFixBytes, int instructionSize, DWORD patchBytes, BYTE * memory, DWORD memorySize, bool generateReloc, DWORD patchOffset, DWORD sectionRVA );
	void patchDirectImportInDump64( int patchPreFixBytes, int instructionSize, DWORD_PTR patchBytes, BYTE * memory, DWORD memorySize, bool generateReloc, DWORD patchOffset, DWORD sectionRVA );
	void patchDirectJumpTableEntry(DWORD_PTR targetIatPointer, DWORD_PTR stdImagebase, DWORD directImportsJumpTableRVA, PeParser * peParser, BYTE * jmpTableMemory, DWORD newIatBase );


};

/*
PE64
----------
000000013FF82D87 FF15 137C0A00 CALL QWORD [RIP+0xA7C13]
Result: 000000014002A9A0

000000013F65C952 FF25 F8EA0B00 JMP QWORD [RIP+0xBEAF8]
Result: 000000013F71B450

PE32
----------
0120FFA5 FF15 8C6D2601 CALL DWORD [0x01266D8C]

0120FF52 FF25 D4722601 JMP DWORD [0x012672D4]
*/

