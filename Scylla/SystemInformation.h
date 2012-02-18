#pragma once

#include <windows.h>

enum OPERATING_SYSTEM {
	UNKNOWN_OS,
	WIN_XP_32,
	WIN_XP_64,
	WIN_VISTA_32,
	WIN_VISTA_64,
	WIN_7_32,
	WIN_7_64,
	WIN_8_32,
	WIN_8_64
};

typedef void (WINAPI *def_GetNativeSystemInfo)(LPSYSTEM_INFO lpSystemInfo);

class SystemInformation
{
public:

	static OPERATING_SYSTEM currenOS;
	static bool getSystemInformation();
};
