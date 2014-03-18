#include <Windows.h>


#pragma once

#include <Windows.h>
#include <vector>
#include <tchar.h>

class HardDisk {
public:
	TCHAR shortName[3];
	TCHAR longName[MAX_PATH];
	size_t longNameLength;
};

class DeviceNameResolver
{
public:
	DeviceNameResolver();
	~DeviceNameResolver();
	bool resolveDeviceLongNameToShort(const TCHAR * sourcePath, TCHAR * targetPath);
private:
	std::vector<HardDisk> deviceNameList;

	void initDeviceNameList();
    void fixVirtualDevices();
};

