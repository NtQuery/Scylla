#include "Logger.h"

#include "MainGui.h"

WCHAR Logger::debugLogFile[MAX_PATH];
WCHAR Logger::logbuf[300];
char  Logger::logbufChar[300];

void Logger::getDebugLogFilePath()
{
	GetModuleFileName(0, debugLogFile, MAX_PATH);

	for(size_t i = wcslen(debugLogFile); i > 0; i--) 
	{
		if(debugLogFile[i] == L'\\') 
		{ 
			debugLogFile[i+1] = 0x00;
			break; 
		} 
	}

	wcscat_s(debugLogFile, _countof(debugLogFile), TEXT(DEBUG_LOG_FILENAME));
}


void Logger::debugLog(const WCHAR * format, ...)
{
	FILE * pFile;
	va_list va_alist;

	if (!format)
	{ 
		return;
	}

	ZeroMemory(logbuf, sizeof(logbuf));

	va_start (va_alist, format);
	_vsnwprintf_s(logbuf, _countof(logbuf), _countof(logbuf) - 1, format, va_alist);
	va_end (va_alist);

	if (_wfopen_s(&pFile,debugLogFile,L"a") == NULL)
	{
		fputws(logbuf,pFile);
		fclose (pFile);
	}
}

void  Logger::debugLog(const char * format, ...)
{
	FILE * pFile;
	va_list va_alist;

	if (!format)
	{ 
		return;
	}

	ZeroMemory(logbufChar, sizeof(logbufChar));

	va_start (va_alist, format);
	_vsnprintf_s(logbufChar, _countof(logbufChar), _countof(logbufChar) - 1, format, va_alist);
	va_end (va_alist);

	if (_wfopen_s(&pFile,debugLogFile,L"a") == NULL)
	{
		fputs(logbufChar,pFile);
		fclose (pFile);
	}
}

void Logger::printfDialog(const WCHAR * format, ...)
{
	va_list va_alist;

	if (!format)
	{ 
		return;
	}

	ZeroMemory(logbuf, sizeof(logbuf));

	va_start (va_alist, format);
	_vsnwprintf_s(logbuf, _countof(logbuf), _countof(logbuf) - 1, format, va_alist);
	va_end (va_alist);

	
	MainGui::addTextToOutputLog(logbuf);
	UpdateWindow(MainGui::hWndMainDlg);
}