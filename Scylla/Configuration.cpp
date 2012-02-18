#include "Configuration.h"

Configuration::Configuration(const WCHAR* name, Type type)
{
	wcscpy_s(this->name, name);
	this->type = type;
	valueNumeric = 0;
	valueString[0] = L'\0';
}

const WCHAR* Configuration::getName() const
{
	return name;
}

Configuration::Type Configuration::getType() const
{
	return type;
}

DWORD_PTR Configuration::getNumeric() const
{
	return valueNumeric;
}

void Configuration::setNumeric(DWORD_PTR value)
{
	valueNumeric = value;
}

const WCHAR* Configuration::getString() const
{
	return valueString;
}

void Configuration::setString(const WCHAR* str)
{
	wcsncpy_s(valueString, str, _countof(valueString));
}

bool Configuration::getBool() const
{
	return getNumeric() == 1;
}

void Configuration::setBool(bool flag)
{
	setNumeric(flag ? 1 : 0);
}

bool Configuration::isTrue() const
{
	return getBool();
}

void Configuration::setTrue()
{
	setBool(true);
}

void Configuration::setFalse()
{
	setBool(false);
}
