#include "Logger.h"

#include <shlwapi.h>
//#include <fstream>
#include <cstdio>
#include <atlbase.h> 
#include <atlconv.h>

void Logger::log(const WCHAR * format, ...)
{
	static WCHAR buf[300];

	if(!format)
		return;

	va_list va_alist;
	va_start (va_alist, format);
	_vsnwprintf_s(buf, _countof(buf) - 1, format, va_alist);
	va_end (va_alist);

	write(buf);
}

void Logger::log(const char * format, ...)
{
	static char buf[300];

	if(!format)
		return;

	va_list va_alist;
	va_start (va_alist, format);
	_vsnprintf_s(buf, _countof(buf) - 1, format, va_alist);
	va_end (va_alist);

	write(buf);
}

void Logger::write(const CHAR * str)
{
	write(ATL::CA2W(str));
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

	FILE * pFile = 0;
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
	
	FILE * pFile = 0;
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

void ListboxLog::write(const WCHAR* str)
{	
	LRESULT index = SendMessageW(window, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str));
	SendMessage(window, LB_SETCURSEL, index, 0);
	UpdateWindow(window);
}
