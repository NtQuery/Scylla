#pragma once

#include <windows.h>

#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#define STATUS_SUCCESS   ((NTSTATUS)0x00000000L)
#define DUPLICATE_SAME_ATTRIBUTES   0x00000004
#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

typedef LONG KPRIORITY;

typedef enum _SYSTEM_INFORMATION_CLASS {

	SystemBasicInformation, 
	SystemProcessorInformation, 
	SystemPerformanceInformation, 
	SystemTimeOfDayInformation, 
	SystemPathInformation, 
	SystemProcessInformation, 
	SystemCallCountInformation, 
	SystemDeviceInformation, 
	SystemProcessorPerformanceInformation, 
	SystemFlagsInformation, 
	SystemCallTimeInformation, 
	SystemModuleInformation, 
	SystemLocksInformation, 
	SystemStackTraceInformation, 
	SystemPagedPoolInformation, 
	SystemNonPagedPoolInformation, 
	SystemHandleInformation, 
	SystemObjectInformation, 
	SystemPageFileInformation, 
	SystemVdmInstemulInformation, 
	SystemVdmBopInformation, 
	SystemFileCacheInformation, 
	SystemPoolTagInformation, 
	SystemInterruptInformation, 
	SystemDpcBehaviorInformation, 
	SystemFullMemoryInformation, 
	SystemLoadGdiDriverInformation, 
	SystemUnloadGdiDriverInformation, 
	SystemTimeAdjustmentInformation, 
	SystemSummaryMemoryInformation, 
	SystemNextEventIdInformation, 
	SystemEventIdsInformation, 
	SystemCrashDumpInformation, 
	SystemExceptionInformation, 
	SystemCrashDumpStateInformation, 
	SystemKernelDebuggerInformation, 
	SystemContextSwitchInformation, 
	SystemRegistryQuotaInformation, 
	SystemExtendServiceTableInformation, 
	SystemPrioritySeperation, 
	SystemPlugPlayBusInformation, 
	SystemDockInformation, 
	SystemPowerInformation2, 
	SystemProcessorSpeedInformation, 
	SystemCurrentTimeZoneInformation, 
	SystemLookasideInformation

} SYSTEM_INFORMATION_CLASS;

typedef struct _IO_STATUS_BLOCK {
	union {
		NTSTATUS Status;
		PVOID Pointer;
	};
	ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _FILE_NAME_INFORMATION { // Information Classes 9 and 21
	ULONG FileNameLength;
	WCHAR FileName[1];
} FILE_NAME_INFORMATION;

typedef enum _FILE_INFORMATION_CLASS {
	FileNameInformation=9,            
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS; 

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _CLIENT_ID{
	HANDLE UniqueProcess;
	HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

#define InitializeObjectAttributes(p,n,a,r,s) \
{ \
	(p)->Length = sizeof(OBJECT_ATTRIBUTES); \
	(p)->ObjectName = n; \
	(p)->Attributes = a; \
	(p)->RootDirectory = r; \
	(p)->SecurityDescriptor = s; \
	(p)->SecurityQualityOfService = NULL; \
}

typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	PVOID RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

typedef enum _OBJECT_INFORMATION_CLASS {
	ObjectBasicInformation,
	ObjectNameInformation,
	ObjectTypeInformation,
	ObjectAllInformation,
	ObjectDataInformation
} OBJECT_INFORMATION_CLASS, *POBJECT_INFORMATION_CLASS;

typedef enum _THREADINFOCLASS {
	ThreadBasicInformation,
	ThreadTimes,
	ThreadPriority,
	ThreadBasePriority,
	ThreadAffinityMask,
	ThreadImpersonationToken,
	ThreadDescriptorTableEntry,
	ThreadEnableAlignmentFaultFixup,
	ThreadEventPair_Reusable,
	ThreadQuerySetWin32StartAddress,
	ThreadZeroTlsCell,
	ThreadPerformanceCount,
	ThreadAmILastThread,
	ThreadIdealProcessor,
	ThreadPriorityBoost,
	ThreadSetTlsArrayAddress,
	ThreadIsIoPending,
	ThreadHideFromDebugger,
	ThreadBreakOnTermination,
	MaxThreadInfoClass
} THREADINFOCLASS;

//
// Memory Information Classes for NtQueryVirtualMemory
//
typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation,
	MemoryWorkingSetList,
	MemorySectionName,
	MemoryBasicVlmInformation
} MEMORY_INFORMATION_CLASS;

typedef enum _PROCESSINFOCLASS {
	ProcessBasicInformation,
	ProcessQuotaLimits,
	ProcessIoCounters,
	ProcessVmCounters,
	ProcessTimes,
	ProcessBasePriority,
	ProcessRaisePriority,
	ProcessDebugPort,
	ProcessExceptionPort,
	ProcessAccessToken,
	ProcessLdtInformation,
	ProcessLdtSize,
	ProcessDefaultHardErrorMode,
	ProcessIoPortHandlers,
	ProcessPooledUsageAndLimits,
	ProcessWorkingSetWatch,
	ProcessUserModeIOPL,
	ProcessEnableAlignmentFaultFixup,
	ProcessPriorityClass,
	ProcessWx86Information,
	ProcessHandleCount,
	ProcessAffinityMask,
	ProcessPriorityBoost,
	ProcessDeviceMap,
	ProcessSessionInformation,
	ProcessForegroundInformation,
	ProcessWow64Information,
	ProcessImageFileName,
	ProcessLUIDDeviceMapsEnabled,
	ProcessBreakOnTermination,
	ProcessDebugObjectHandle,
	ProcessDebugFlags,
	ProcessHandleTracing,
	ProcessIoPriority,
	ProcessExecuteFlags,
	ProcessResourceManagement,
	ProcessCookie,
	ProcessImageInformation, 
	MaxProcessInfoClass
} PROCESSINFOCLASS;

typedef struct _PEB_LDR_DATA {
	BYTE       Reserved1[8];
	PVOID      Reserved2[3];
	LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
	BYTE           Reserved1[16];
	PVOID          Reserved2[10];
	UNICODE_STRING ImagePathName;
	UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _PEB {
	BYTE                          Reserved1[2];
	BYTE                          BeingDebugged;
	BYTE                          Reserved2[1];
	PVOID                         Reserved3[2];
	PPEB_LDR_DATA                 Ldr;
	PRTL_USER_PROCESS_PARAMETERS  ProcessParameters;
	BYTE                          Reserved4[104];
	PVOID                         Reserved5[52];
	PVOID						  PostProcessInitRoutine;
	BYTE                          Reserved6[128];
	PVOID                         Reserved7[1];
	ULONG                         SessionId;
} PEB, *PPEB;

typedef struct _PROCESS_BASIC_INFORMATION {
	PVOID Reserved1;
	PPEB PebBaseAddress;
	PVOID Reserved2[2];
	ULONG_PTR UniqueProcessId;
	PVOID Reserved3;
} PROCESS_BASIC_INFORMATION;


typedef struct _MEMORY_WORKING_SET_LIST
{
	ULONG	NumberOfPages;
	ULONG	WorkingSetList[1];
} MEMORY_WORKING_SET_LIST, *PMEMORY_WORKING_SET_LIST;

typedef struct _MEMORY_SECTION_NAME
{
	UNICODE_STRING	SectionFileName;
} MEMORY_SECTION_NAME, *PMEMORY_SECTION_NAME;

typedef struct _SYSTEM_SESSION_PROCESS_INFORMATION
{
    ULONG SessionId;
    ULONG SizeOfBuf;
    PVOID Buffer;
} SYSTEM_SESSION_PROCESS_INFORMATION, *PSYSTEM_SESSION_PROCESS_INFORMATION;

typedef struct _SYSTEM_THREAD_INFORMATION
{
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER CreateTime;
    ULONG WaitTime;
    PVOID StartAddress;
    CLIENT_ID ClientId;
    KPRIORITY Priority;
    LONG BasePriority;
    ULONG ContextSwitches;
    ULONG ThreadState;
    ULONG WaitReason;
} SYSTEM_THREAD_INFORMATION, *PSYSTEM_THREAD_INFORMATION;

typedef struct _SYSTEM_EXTENDED_THREAD_INFORMATION
{
    SYSTEM_THREAD_INFORMATION ThreadInfo;
    PVOID StackBase;
    PVOID StackLimit;
    PVOID Win32StartAddress;
    PVOID TebAddress; /* This is only filled in on Vista and above */
    ULONG_PTR Reserved2;
    ULONG_PTR Reserved3;
    ULONG_PTR Reserved4;
} SYSTEM_EXTENDED_THREAD_INFORMATION, *PSYSTEM_EXTENDED_THREAD_INFORMATION;

typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    HANDLE InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG_PTR PageDirectoryBase;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER ReadOperationCount;
    LARGE_INTEGER WriteOperationCount;
    LARGE_INTEGER OtherOperationCount;
    LARGE_INTEGER ReadTransferCount;
    LARGE_INTEGER WriteTransferCount;
    LARGE_INTEGER OtherTransferCount;
    SYSTEM_THREAD_INFORMATION Threads[1];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;


///////////////////////////////////////////////////////////////////////////////////////
//Evolution of Process Environment Block (PEB) http://blog.rewolf.pl/blog/?p=573
//March 2, 2013 / ReWolf posted in programming, reverse engineering, source code, x64 /

#pragma pack(push)
#pragma pack(1)

template <class T>
struct LIST_ENTRY_T
{
    T Flink;
    T Blink;
};

template <class T>
struct UNICODE_STRING_T
{
    union
    {
        struct
        {
            WORD Length;
            WORD MaximumLength;
        };
        T dummy;
    };
    T _Buffer;
};

template <class T, class NGF, int A>
struct _PEB_T
{
    union
    {
        struct
        {
            BYTE InheritedAddressSpace;
            BYTE ReadImageFileExecOptions;
            BYTE BeingDebugged;
            BYTE _SYSTEM_DEPENDENT_01;
        };
        T dummy01;
    };
    T Mutant;
    T ImageBaseAddress;
    T Ldr;
    T ProcessParameters;
    T SubSystemData;
    T ProcessHeap;
    T FastPebLock;
    T _SYSTEM_DEPENDENT_02;
    T _SYSTEM_DEPENDENT_03;
    T _SYSTEM_DEPENDENT_04;
    union
    {
        T KernelCallbackTable;
        T UserSharedInfoPtr;
    };
    DWORD SystemReserved;
    DWORD _SYSTEM_DEPENDENT_05;
    T _SYSTEM_DEPENDENT_06;
    T TlsExpansionCounter;
    T TlsBitmap;
    DWORD TlsBitmapBits[2];
    T ReadOnlySharedMemoryBase;
    T _SYSTEM_DEPENDENT_07;
    T ReadOnlyStaticServerData;
    T AnsiCodePageData;
    T OemCodePageData;
    T UnicodeCaseTableData;
    DWORD NumberOfProcessors;
    union
    {
        DWORD NtGlobalFlag;
        NGF dummy02;
    };
    LARGE_INTEGER CriticalSectionTimeout;
    T HeapSegmentReserve;
    T HeapSegmentCommit;
    T HeapDeCommitTotalFreeThreshold;
    T HeapDeCommitFreeBlockThreshold;
    DWORD NumberOfHeaps;
    DWORD MaximumNumberOfHeaps;
    T ProcessHeaps;
};

typedef _PEB_T<DWORD, DWORD64, 34> PEB32;
typedef _PEB_T<DWORD64, DWORD, 30> PEB64;

#ifdef _WIN64
typedef PEB64 PEB_CURRENT;
#else
typedef PEB32 PEB_CURRENT;
#endif

#pragma pack(pop)


typedef NTSTATUS (WINAPI *def_NtTerminateProcess)(HANDLE ProcessHandle, NTSTATUS ExitStatus);
typedef NTSTATUS (WINAPI *def_NtQueryObject)(HANDLE Handle,OBJECT_INFORMATION_CLASS ObjectInformationClass,PVOID ObjectInformation,ULONG ObjectInformationLength,PULONG ReturnLength);
typedef NTSTATUS (WINAPI *def_NtDuplicateObject)(HANDLE SourceProcessHandle, HANDLE SourceHandle, HANDLE TargetProcessHandle, PHANDLE TargetHandle, ACCESS_MASK DesiredAccess, BOOLEAN InheritHandle, ULONG Options ); 
typedef NTSTATUS (WINAPI *def_NtQueryInformationFile)(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock, PVOID FileInformation, ULONG Length, FILE_INFORMATION_CLASS FileInformationClass);
typedef NTSTATUS (WINAPI *def_NtQueryInformationThread)(HANDLE ThreadHandle,THREADINFOCLASS ThreadInformationClass,PVOID ThreadInformation,ULONG ThreadInformationLength,PULONG ReturnLength);
typedef NTSTATUS (WINAPI *def_NtQueryInformationProcess)(HANDLE ProcessHandle,PROCESSINFOCLASS ProcessInformationClass,PVOID ProcessInformation,ULONG ProcessInformationLength,PULONG ReturnLength);
typedef NTSTATUS (WINAPI *def_NtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS SystemInformationClass,PVOID SystemInformation,ULONG SystemInformationLength, PULONG ReturnLength);
typedef NTSTATUS (WINAPI *def_NtQueryVirtualMemory)(HANDLE ProcessHandle, PVOID BaseAddress, MEMORY_INFORMATION_CLASS MemoryInformationClass, PVOID Buffer, ULONG Length, PULONG ResultLength); 
typedef NTSTATUS (WINAPI *def_NtOpenProcess)(PHANDLE ProcessHandle, ACCESS_MASK AccessMask, PVOID ObjectAttributes, PCLIENT_ID ClientId );
typedef NTSTATUS (WINAPI *def_NtOpenThread)(PHANDLE ThreadHandle,ACCESS_MASK DesiredAccess,POBJECT_ATTRIBUTES ObjectAttributes,PCLIENT_ID ClientId);
typedef NTSTATUS (WINAPI *def_NtResumeThread)(HANDLE ThreadHandle, PULONG SuspendCount);
typedef NTSTATUS (WINAPI *def_NtSetInformationThread)(HANDLE ThreadHandle,THREADINFOCLASS ThreadInformationClass,PVOID ThreadInformation,ULONG ThreadInformationLength);
typedef NTSTATUS (WINAPI *def_NtCreateThreadEx)(PHANDLE hThread,ACCESS_MASK DesiredAccess,LPVOID ObjectAttributes,HANDLE ProcessHandle,LPTHREAD_START_ROUTINE lpStartAddress,LPVOID lpParameter,int CreateFlags,ULONG StackZeroBits,LPVOID SizeOfStackCommit,LPVOID SizeOfStackReserve,LPVOID lpBytesBuffer);
typedef NTSTATUS (WINAPI *def_NtSuspendProcess)(HANDLE ProcessHandle);
typedef NTSTATUS (WINAPI *def_NtResumeProcess)(HANDLE ProcessHandle);

typedef ULONG (WINAPI *def_RtlNtStatusToDosError)(NTSTATUS Status);

//Flags from waliedassar
#define NtCreateThreadExFlagCreateSuspended  0x1
#define NtCreateThreadExFlagSuppressDllMains 0x2
#define NtCreateThreadExFlagHideFromDebugger 0x4

class NativeWinApi {
public:
	
	static def_NtCreateThreadEx NtCreateThreadEx;
	static def_NtDuplicateObject NtDuplicateObject;
	static def_NtOpenProcess NtOpenProcess;
	static def_NtOpenThread NtOpenThread;
	static def_NtQueryObject NtQueryObject;
	static def_NtQueryInformationFile NtQueryInformationFile;
	static def_NtQueryInformationProcess NtQueryInformationProcess;
	static def_NtQueryInformationThread NtQueryInformationThread;
	static def_NtQuerySystemInformation NtQuerySystemInformation;
	static def_NtQueryVirtualMemory NtQueryVirtualMemory;
	static def_NtResumeProcess NtResumeProcess;
	static def_NtResumeThread NtResumeThread;
	static def_NtSetInformationThread NtSetInformationThread;
	static def_NtSuspendProcess NtSuspendProcess;
	static def_NtTerminateProcess NtTerminateProcess;


	static def_RtlNtStatusToDosError RtlNtStatusToDosError;

	static void initialize();

	static PPEB getCurrentProcessEnvironmentBlock();
	static PPEB getProcessEnvironmentBlockAddress(HANDLE processHandle);
};
