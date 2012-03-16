
#include "DeviceNameResolver.h"

DeviceNameResolver::DeviceNameResolver()
{
	initDeviceNameList();
}

DeviceNameResolver::~DeviceNameResolver()
{
	deviceNameList.clear();
}

void DeviceNameResolver::initDeviceNameList()
{
	TCHAR shortName[3] = {0};
	TCHAR longName[MAX_PATH] = {0};
	HardDisk hardDisk;

	shortName[1] = L':';

	deviceNameList.reserve(3);

	for ( TCHAR shortD = TEXT('a'); shortD < TEXT('z'); shortD++ )
	{
		shortName[0] = shortD;
		if (QueryDosDevice( shortName, longName, MAX_PATH ) > 0)
		{
			hardDisk.shortName[0] = _totupper(shortD);
			hardDisk.shortName[1] = TEXT(':');
			hardDisk.shortName[2] = 0;

			hardDisk.longNameLength = _tcslen(longName);

			
			_tcscpy_s(hardDisk.longName, longName);
			deviceNameList.push_back(hardDisk);
		}
	}
}

bool DeviceNameResolver::resolveDeviceLongNameToShort( WCHAR * sourcePath, WCHAR * targetPath )
{
	for (unsigned int i = 0; i < deviceNameList.size(); i++)
	{
		if (!_tcsnicmp(deviceNameList[i].longName, sourcePath, deviceNameList[i].longNameLength))
		{
			_tcscpy_s(targetPath, MAX_PATH, deviceNameList[i].shortName);
			_tcscat_s(targetPath, MAX_PATH, sourcePath + deviceNameList[i].longNameLength);
			return true;
		}
	}

	return false;
}