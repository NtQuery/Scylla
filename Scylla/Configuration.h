#pragma once

#include <windows.h>

class Configuration
{
public:

	enum Type {
		String,
		Decimal,
		Hexadecimal,
		Boolean
	};

	static const size_t CONFIG_NAME_LENGTH = 100;
	static const size_t CONFIG_STRING_LENGTH = 100;

	Configuration(const WCHAR* name = L"", Type type = String);

	const WCHAR* getName() const;
	Type getType() const;

	DWORD_PTR getNumeric() const;
	void setNumeric(DWORD_PTR value);

	const WCHAR* getString() const;
	void setString(const WCHAR* str);

	bool getBool() const;
	void setBool(bool flag);

	// Redundant (we have getBool and setBool), but easier on the eye
	bool isTrue() const;
	void setTrue();
	void setFalse();

private:

	WCHAR name[CONFIG_NAME_LENGTH];
	Type type;

	DWORD_PTR valueNumeric;
	WCHAR valueString[CONFIG_STRING_LENGTH];
};
