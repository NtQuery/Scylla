#pragma once

class StringConversion
{
public:

	static const char* ToASCII(const wchar_t* str, char* buf, size_t bufsize);
	static const wchar_t* ToUTF16(const char* str, wchar_t* buf, size_t bufsize);
};
