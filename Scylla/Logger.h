
#pragma once

#include <windows.h>

#define DEBUG_LOG_FILENAME "Scylla_debug.log"

class Logger {
public:
	static void debugLog(const WCHAR * format, ...);
	static void debugLog(const CHAR * format, ...);
	static void printfDialog(const WCHAR * format, ...);

	static void getDebugLogFilePath();

private:
	static WCHAR debugLogFile[MAX_PATH];
	static WCHAR logbuf[300];
	static char logbufChar[300];
};