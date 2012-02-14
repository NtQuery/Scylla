#include "Logger.h"

#include <shlwapi.h>
#include <atlconv.h>
#include <fstream>

WCHAR Logger::logbuf[300];
char  Logger::logbufChar[300];

void Logger::log(const WCHAR * format, ...)
{
	if (!format)
	{ 
		return;
	}

	//ZeroMemory(logbuf, sizeof(logbuf));

	va_list va_alist;
	va_start (va_alist, format);
	_vsnwprintf_s(logbuf, _countof(logbuf), _countof(logbuf) - 1, format, va_alist);
	va_end (va_alist);

	write(logbuf);
}

void Logger::log(const char * format, ...)
{
	if (!format)
	{ 
		return;
	}

	//ZeroMemory(logbufChar, sizeof(logbufChar));

	va_list va_alist;
	va_start (va_alist, format);
	_vsnprintf_s(logbufChar, _countof(logbufChar), _countof(logbufChar) - 1, format, va_alist);
	va_end (va_alist);

	write(logbufChar);
}

void Logger::write(const CHAR * str)
{
	size_t len = strlen(str) + 1;
	WCHAR * buf = new WCHAR[len];

	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, buf, len, str, _TRUNCATE);

	write(buf);

	delete[] buf;
}

FileLog::FileLog(const WCHAR * fileName)
{
	GetModuleFileName(0, this->filePath, _countof(this->filePath));
	PathRemoveFileSpec(this->filePath);
	PathAppend(this->filePath, fileName);
}

void FileLog::write(const CHAR * str)
{
	/*
	std::wofstream file(filePath, std::wofstream::app);
	if(!file.fail())
	{
		file << str << std::endl;
	}
	*/

	FILE * pFile;
	if (_wfopen_s(&pFile, filePath, L"a") == 0)
	{
		fputs(str, pFile);
		fputs("\r\n", pFile);
		fclose(pFile);
	}
}

void FileLog::write(const WCHAR * str)
{
	/*
	std::wofstream file(filePath, std::wofstream::app);
	if(!file.fail())
	{
		file << str << std::endl;
	}
	*/
	
	FILE * pFile;
	if (_wfopen_s(&pFile, filePath, L"a") == 0)
	{
		fputws(str, pFile);
		fputws(L"\r\n", pFile);
		fclose(pFile);
	}
}

void ListboxLog::setWindow(HWND window)
{
	this->window = window;
}

void ListboxLog::write(const WCHAR * str)
{	
	LRESULT index = SendMessageW(window, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str));
	SendMessage(window, LB_SETCURSEL, index, 0);
	UpdateWindow(window);
}
