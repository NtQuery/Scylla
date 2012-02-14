#include "SystemInformation.h"

OPERATING_SYSTEM SystemInformation::currenOS = UNKNOWN_OS;

bool SystemInformation::getSystemInformation()
{
	OSVERSIONINFOEX osvi = {0};
	SYSTEM_INFO si = {0};
	def_GetNativeSystemInfo _GetNativeSystemInfo = 0;

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (!GetVersionEx((OSVERSIONINFO*) &osvi))
	{
		return false;
	}

	if ((osvi.dwMajorVersion < 5) || ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)))
	{
		return false;
	}

	_GetNativeSystemInfo = (def_GetNativeSystemInfo)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetNativeSystemInfo");
	if (_GetNativeSystemInfo)
	{
		_GetNativeSystemInfo(&si);
	}
	else
	{
		GetSystemInfo(&si);
	}

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 && osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
	{
		currenOS = WIN_VISTA_64;
	}
	else if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL && osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
	{
		currenOS = WIN_VISTA_32;
	}
	else if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 && osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
	{
		currenOS = WIN_7_64;
	}
	else if(si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL && osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
	{
		currenOS = WIN_7_32;
	}
	else if(si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 && osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
	{
		currenOS = WIN_XP_64;
	}
	else if(osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
	{
		currenOS = WIN_XP_32;
	}
	else
	{
		currenOS = UNKNOWN_OS;
	}

	return (currenOS != UNKNOWN_OS);
}