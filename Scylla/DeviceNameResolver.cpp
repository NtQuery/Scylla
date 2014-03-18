
#include "DeviceNameResolver.h"
#include "NativeWinApi.h"

DeviceNameResolver::DeviceNameResolver()
{
    NativeWinApi::initialize();
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

	shortName[1] = TEXT(':');

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

    fixVirtualDevices();
}

bool DeviceNameResolver::resolveDeviceLongNameToShort(const TCHAR * sourcePath, TCHAR * targetPath)
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

void DeviceNameResolver::fixVirtualDevices()
{
    WCHAR longCopy[MAX_PATH] = {0};
    OBJECT_ATTRIBUTES oa = {0};
    UNICODE_STRING unicodeInput = {0};
    UNICODE_STRING unicodeOutput = {0};
    HANDLE hFile = 0;
    ULONG retLen = 0;
    HardDisk hardDisk;

    unicodeOutput.Length = MAX_PATH * 2 * sizeof(WCHAR);
    unicodeOutput.MaximumLength = unicodeOutput.Length;
    unicodeOutput.Buffer = (PWSTR)calloc(unicodeOutput.Length, 1);

    for (unsigned int i = 0; i < deviceNameList.size(); i++)
    {
        wcscpy_s(longCopy, deviceNameList[i].longName);

        NativeWinApi::RtlInitUnicodeString(&unicodeInput, longCopy);
        InitializeObjectAttributes(&oa, &unicodeInput, 0, 0, 0);

        if(NT_SUCCESS(NativeWinApi::NtOpenSymbolicLinkObject(&hFile, SYMBOLIC_LINK_QUERY, &oa)))
        {
            unicodeOutput.Length = MAX_PATH * 2 * sizeof(WCHAR);
            unicodeOutput.MaximumLength = unicodeOutput.Length;

            if (NT_SUCCESS(NativeWinApi::NtQuerySymbolicLinkObject(hFile, &unicodeOutput, &retLen)))
            {
                hardDisk.longNameLength = wcslen(unicodeOutput.Buffer);
                wcscpy_s(hardDisk.shortName, deviceNameList[i].shortName);
                wcscpy_s(hardDisk.longName, unicodeOutput.Buffer);
                deviceNameList.push_back(hardDisk);
            }  

            NativeWinApi::NtClose(hFile);
        }
    }

    free(unicodeOutput.Buffer);
}

