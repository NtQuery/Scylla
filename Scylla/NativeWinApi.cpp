#include "NativeWinApi.h"

def_NtCreateThreadEx NativeWinApi::NtCreateThreadEx = 0;
def_NtDuplicateObject NativeWinApi::NtDuplicateObject = 0;
def_NtOpenProcess NativeWinApi::NtOpenProcess = 0;
def_NtOpenThread NativeWinApi::NtOpenThread = 0;
def_NtQueryObject NativeWinApi::NtQueryObject = 0;
def_NtQueryInformationFile NativeWinApi::NtQueryInformationFile = 0;
def_NtQueryInformationProcess NativeWinApi::NtQueryInformationProcess = 0;
def_NtQueryInformationThread NativeWinApi::NtQueryInformationThread = 0;
def_NtQuerySystemInformation NativeWinApi::NtQuerySystemInformation = 0;
def_NtQueryVirtualMemory NativeWinApi::NtQueryVirtualMemory = 0;
def_NtResumeProcess NativeWinApi::NtResumeProcess = 0;
def_NtResumeThread NativeWinApi::NtResumeThread = 0;
def_NtSetInformationThread NativeWinApi::NtSetInformationThread = 0;
def_NtSuspendProcess NativeWinApi::NtSuspendProcess = 0;
def_NtTerminateProcess NativeWinApi::NtTerminateProcess = 0;

def_RtlNtStatusToDosError NativeWinApi::RtlNtStatusToDosError = 0;

void NativeWinApi::initialize()
{
	HMODULE hModuleNtdll = GetModuleHandle(L"ntdll.dll");

	if (!hModuleNtdll)
	{
		return;
	}

	NtCreateThreadEx = (def_NtCreateThreadEx)GetProcAddress(hModuleNtdll, "NtCreateThreadEx");
	NtDuplicateObject = (def_NtDuplicateObject)GetProcAddress(hModuleNtdll, "NtDuplicateObject");
	NtOpenProcess = (def_NtOpenProcess)GetProcAddress(hModuleNtdll, "NtOpenProcess");
	NtOpenThread = (def_NtOpenThread)GetProcAddress(hModuleNtdll, "NtOpenThread");
	NtQueryObject = (def_NtQueryObject)GetProcAddress(hModuleNtdll, "NtQueryObject");
	NtQueryInformationFile = (def_NtQueryInformationFile)GetProcAddress(hModuleNtdll, "NtQueryInformationFile");
	NtQueryInformationProcess = (def_NtQueryInformationProcess)GetProcAddress(hModuleNtdll, "NtQueryInformationProcess");
	NtQueryInformationThread = (def_NtQueryInformationThread)GetProcAddress(hModuleNtdll, "NtQueryInformationThread");
	NtQuerySystemInformation = (def_NtQuerySystemInformation)GetProcAddress(hModuleNtdll, "NtQuerySystemInformation");
	NtQueryVirtualMemory = (def_NtQueryVirtualMemory)GetProcAddress(hModuleNtdll, "NtQueryVirtualMemory");
	NtResumeProcess = (def_NtResumeProcess)GetProcAddress(hModuleNtdll, "NtResumeProcess");
	NtResumeThread = (def_NtResumeThread)GetProcAddress(hModuleNtdll, "NtResumeThread");
	NtSetInformationThread = (def_NtSetInformationThread)GetProcAddress(hModuleNtdll, "NtSetInformationThread");
	NtSuspendProcess = (def_NtSuspendProcess)GetProcAddress(hModuleNtdll, "NtSuspendProcess");
	NtTerminateProcess = (def_NtTerminateProcess)GetProcAddress(hModuleNtdll, "NtTerminateProcess");

	RtlNtStatusToDosError = (def_RtlNtStatusToDosError)GetProcAddress(hModuleNtdll, "RtlNtStatusToDosError");

}


PPEB NativeWinApi::getCurrentProcessEnvironmentBlock()
{
	return getProcessEnvironmentBlockAddress(GetCurrentProcess());
}

PPEB NativeWinApi::getProcessEnvironmentBlockAddress(HANDLE processHandle)
{
	ULONG lReturnLength = 0;
	PROCESS_BASIC_INFORMATION processBasicInformation;

	if ((NtQueryInformationProcess(processHandle,ProcessBasicInformation,&processBasicInformation,sizeof(PROCESS_BASIC_INFORMATION),&lReturnLength) >= 0) && (lReturnLength == sizeof(PROCESS_BASIC_INFORMATION)))
	{
		//printf("NtQueryInformationProcess success %d\n",sizeof(PROCESS_BASIC_INFORMATION));

		return processBasicInformation.PebBaseAddress;
	}
	else
	{
		//printf("NtQueryInformationProcess failed %d vs %d\n",lReturnLength,sizeof(PROCESS_BASIC_INFORMATION));
		return 0;
	}
}