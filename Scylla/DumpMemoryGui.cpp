#include "DumpMemoryGui.h"

#include "Architecture.h"
#include "ProcessAccessHelp.h"
#include <Psapi.h>

WCHAR DumpMemoryGui::protectionString[100];
const WCHAR DumpMemoryGui::MemoryUndefined[] = L"UNDEF";
const WCHAR DumpMemoryGui::MemoryUnknown[] = L"UNKNOWN";
const WCHAR * DumpMemoryGui::MemoryStateValues[] = {L"COMMIT",L"FREE",L"RESERVE"};
const WCHAR * DumpMemoryGui::MemoryTypeValues[] = {L"IMAGE",L"MAPPED",L"PRIVATE"};
const WCHAR * DumpMemoryGui::MemoryProtectionValues[] = {L"EXECUTE",L"EXECUTE_READ",L"EXECUTE_READWRITE",L"EXECUTE_WRITECOPY",L"NOACCESS",L"READONLY",L"READWRITE",L"WRITECOPY",L"GUARD",L"NOCACHE",L"WRITECOMBINE"};


DumpMemoryGui::DumpMemoryGui()
{
	dumpedMemory = 0;
	dumpedMemorySize = 0;
	deviceNameResolver = new DeviceNameResolver();
}
DumpMemoryGui::~DumpMemoryGui()
{
	if (dumpedMemory)
	{
		delete [] dumpedMemory;
	}

	if (deviceNameResolver)
	{
		delete deviceNameResolver;
	}
}
BOOL DumpMemoryGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange(); // attach controls
	DlgResize_Init(true, true);

	addColumnsToMemoryList(ListMemorySelect);
	displayMemoryList(ListMemorySelect);

	forceDump = false;
	DoDataExchange(DDX_LOAD);

	EditMemoryAddress.SetValue(ProcessAccessHelp::targetImageBase);
	EditMemorySize.SetValue((DWORD)ProcessAccessHelp::targetSizeOfImage);

	CenterWindow();
	return TRUE;
}

void DumpMemoryGui::addColumnsToMemoryList(CListViewCtrl& list)
{
	list.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	list.InsertColumn(COL_ADDRESS, L"Address", LVCFMT_CENTER);
	list.InsertColumn(COL_SIZE, L"Size", LVCFMT_CENTER);
	list.InsertColumn(COL_FILENAME, L"File", LVCFMT_LEFT);
	list.InsertColumn(COL_PESECTION, L"PE Section", LVCFMT_LEFT);

	list.InsertColumn(COL_TYPE, L"Type", LVCFMT_CENTER);
	list.InsertColumn(COL_PROTECTION, L"Protection", LVCFMT_CENTER);
	list.InsertColumn(COL_STATE, L"State", LVCFMT_CENTER);

	list.InsertColumn(COL_MAPPED_FILE, L"Mapped File", LVCFMT_LEFT);
}

void DumpMemoryGui::displayMemoryList(CListViewCtrl& list)
{
	int count = 0;
	WCHAR temp[20];
	list.DeleteAllItems();


	getMemoryList();

	std::vector<Memory>::const_iterator iter;

	for( iter = memoryList.begin(); iter != memoryList.end(); iter++ , count++)
	{
		swprintf_s(temp, PRINTF_DWORD_PTR_FULL, iter->address);
		list.InsertItem(count,temp);

		swprintf_s(temp, L"%08X", iter->size);
		list.SetItemText(count, COL_SIZE, temp);

		list.SetItemText(count, COL_FILENAME, iter->filename);
		list.SetItemText(count, COL_PESECTION, iter->peSection);

		if (iter->state == MEM_FREE)
		{
			list.SetItemText(count, COL_TYPE, MemoryUndefined);
		}
		else
		{
			list.SetItemText(count, COL_TYPE, getMemoryTypeString(iter->type));
		}

		if ( (iter->state == MEM_RESERVE) || (iter->state == MEM_FREE) )
		{
			list.SetItemText(count, COL_PROTECTION, MemoryUndefined);
		}
		else
		{
			list.SetItemText(count, COL_PROTECTION, getMemoryProtectionString(iter->protect));
		}
		
		list.SetItemText(count, COL_STATE, getMemoryStateString(iter->state));

		list.SetItemText(count, COL_MAPPED_FILE, iter->mappedFilename);

		list.SetItemData(count, (DWORD_PTR)&(*iter));
	}

	list.SetColumnWidth(COL_ADDRESS, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_SIZE, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_FILENAME, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_PESECTION, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_TYPE, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_PROTECTION, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_STATE, LVSCW_AUTOSIZE_USEHEADER);
	list.SetColumnWidth(COL_MAPPED_FILE, LVSCW_AUTOSIZE_USEHEADER);
}

const WCHAR * DumpMemoryGui::getMemoryTypeString(DWORD value)
{
	switch(value)
	{
	case MEM_IMAGE:
		return MemoryTypeValues[TYPE_IMAGE];
	case MEM_MAPPED:
		return MemoryTypeValues[TYPE_MAPPED];
	case MEM_PRIVATE:
		return MemoryTypeValues[TYPE_PRIVATE];
	default:
		return MemoryUnknown;
	}
}
const WCHAR * DumpMemoryGui::getMemoryStateString(DWORD value)
{
	switch(value)
	{
	case MEM_COMMIT:
		return MemoryStateValues[STATE_COMMIT];
	case MEM_FREE:
		return MemoryStateValues[STATE_FREE];
	case MEM_RESERVE:
		return MemoryStateValues[STATE_RESERVE];
	default:
		return MemoryUnknown;
	}
}

WCHAR * DumpMemoryGui::getMemoryProtectionString(DWORD value)
{
	protectionString[0] = 0;

	if (value & PAGE_GUARD)
	{
		wcscpy_s(protectionString, MemoryProtectionValues[PROT_GUARD]);
		wcscat_s(protectionString, L" | ");
		value ^= PAGE_GUARD;
	}
	if (value & PAGE_NOCACHE)
	{
		wcscpy_s(protectionString, MemoryProtectionValues[PROT_NOCACHE]);
		wcscat_s(protectionString, L" | ");
		value ^= PAGE_NOCACHE;
	}
	if (value & PAGE_WRITECOMBINE)
	{
		wcscpy_s(protectionString, MemoryProtectionValues[PROT_WRITECOMBINE]);
		wcscat_s(protectionString, L" | ");
		value ^= PAGE_WRITECOMBINE;
	}

	switch(value)
	{
	case PAGE_EXECUTE:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_EXECUTE]);
			break;
		}
	case PAGE_EXECUTE_READ:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_EXECUTE_READ]);
			break;
		}
	case PAGE_EXECUTE_READWRITE:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_EXECUTE_READWRITE]);
			break;
		}
	case PAGE_EXECUTE_WRITECOPY:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_EXECUTE_WRITECOPY]);
			break;
		}
	case PAGE_NOACCESS:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_NOACCESS]);
			break;
		}
	case PAGE_READONLY:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_READONLY]);
			break;
		}
	case PAGE_READWRITE:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_READWRITE]);
			break;
		}
	case PAGE_WRITECOPY:
		{
			wcscat_s(protectionString, MemoryProtectionValues[PROT_WRITECOPY]);
			break;
		}
	default:
		{
			wcscat_s(protectionString, MemoryUnknown);
		}
	}

	return protectionString;
}


LRESULT DumpMemoryGui::OnListMemoryColumnClicked(NMHDR* pnmh)
{
	NMLISTVIEW* list = (NMLISTVIEW*)pnmh;
	int column = list->iSubItem;

	if(column == prevColumn)
	{
		ascending = !ascending;
	}
	else
	{
		prevColumn = column;
		ascending = true;
	}

	// lo-byte: column, hi-byte: sort-order
	ListMemorySelect.SortItems(&listviewCompareFunc, MAKEWORD(column, ascending));

	return 0;
}
LRESULT DumpMemoryGui::OnListMemoryClick(NMHDR* pnmh)
{
	int index = ListMemorySelect.GetSelectionMark();
	if (index != -1)
	{
		selectedMemory = (Memory *)ListMemorySelect.GetItemData(index);
		if (selectedMemory)
		{
			updateAddressAndSize(selectedMemory);
		}
		
	}
	return 0;
}
void DumpMemoryGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DoDataExchange(DDX_SAVE);

	if (EditMemoryAddress.GetValue() == 0 || EditMemorySize.GetValue() == 0)
	{
		wndCtl.MessageBoxW(L"Textbox is empty!",L"Error",MB_ICONERROR);
	}
	else
	{
		if (dumpMemory())
		{
			EndDialog(1);
		}
		else
		{
			wndCtl.MessageBoxW(L"Reading memory from process failed",L"Error",MB_ICONERROR);
		}		
	}	
}
void DumpMemoryGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void DumpMemoryGui::updateAddressAndSize( Memory * selectedMemory )
{
	EditMemoryAddress.SetValue(selectedMemory->address);
	EditMemorySize.SetValue(selectedMemory->size);
}

int DumpMemoryGui::listviewCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const Memory * module1 = (Memory *)lParam1;
	const Memory * module2 = (Memory *)lParam2;

	int column = LOBYTE(lParamSort);
	bool ascending = (HIBYTE(lParamSort) == TRUE);

	int diff = 0;

	switch(column)
	{
	case COL_ADDRESS:
		diff = module1->address < module2->address ? -1 : 1;
		break;
	case COL_SIZE:
		diff = module1->size < module2->size ? -1 : 1;
		break;
	case COL_FILENAME:
		diff = _wcsicmp(module1->filename, module2->filename);
		break;
	case COL_PESECTION:
		diff = _wcsicmp(module1->peSection, module2->peSection);
		break;
	case COL_TYPE:
		diff = module1->type < module2->type ? -1 : 1;
		break;
	case COL_PROTECTION:
		diff = module1->protect < module2->protect ? -1 : 1;
		break;
	case COL_STATE:
		diff = _wcsicmp(getMemoryStateString(module1->state), getMemoryStateString(module2->state));
		//diff = module1->state < module2->state ? -1 : 1;
		break;
	case COL_MAPPED_FILE:
		diff = _wcsicmp(module1->mappedFilename, module2->mappedFilename);
		break;
	}

	return ascending ? diff : -diff;
}

void DumpMemoryGui::getMemoryList()
{
	DWORD count = 0;
	DWORD_PTR address = 0;
	MEMORY_BASIC_INFORMATION memBasic = {0};
	Memory memory;
	HMODULE * hMods = 0;
	DWORD cbNeeded = 0;
	bool notEnough = true;
	WCHAR target[MAX_PATH];

	count = 100;
	hMods = new HMODULE[count];

	if (memoryList.empty())
	{
		memoryList.reserve(20);
	}
	else
	{
		memoryList.clear();
	}

	memory.filename[0] = 0;
	memory.peSection[0] = 0;
	memory.mappedFilename[0] = 0;

	while(VirtualQueryEx(ProcessAccessHelp::hProcess,(LPCVOID)address,&memBasic,sizeof(memBasic)))
	{
		memory.address = (DWORD_PTR)memBasic.BaseAddress;
		memory.type = memBasic.Type;
		memory.state = memBasic.State;
		memory.size = (DWORD)memBasic.RegionSize;
		memory.protect = memBasic.Protect;
		

		if (memory.type == MEM_MAPPED)
		{
			if (!getMappedFilename(&memory))
			{
				memory.mappedFilename[0] = 0;
			}
		}

		memoryList.push_back(memory);

		memory.mappedFilename[0] = 0;

		address += memBasic.RegionSize;
	}

	do 
	{
		if (!EnumProcessModules(ProcessAccessHelp::hProcess, hMods, count * sizeof(HMODULE), &cbNeeded))
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getMemoryList :: EnumProcessModules failed count %d", count);
#endif
			delete [] hMods;
			return;
		}

		if ( (count * sizeof(HMODULE)) < cbNeeded )
		{
			delete [] hMods;
			count += 100;
			hMods = new HMODULE[count];
		}
		else
		{
			notEnough = false;
		}
	} while (notEnough);

	for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
	{
		if (GetModuleFileNameExW(ProcessAccessHelp::hProcess, hMods[i], target, _countof(target)))
		{
			setModuleName((DWORD_PTR)hMods[i],target);
			setAllSectionNames((DWORD_PTR)hMods[i],target);
		}
		else
		{
#ifdef DEBUG_COMMENTS
			Scylla::debugLog.log(L"getMemoryList :: GetModuleFileNameExW failed 0x%X", GetLastError());
#endif
		}
	}

	delete [] hMods;


}

void DumpMemoryGui::setSectionName(DWORD_PTR sectionAddress, DWORD sectionSize, const WCHAR * sectionName)
{
	bool found = false;
	std::vector<Memory>::const_iterator iter;


	for( iter = memoryList.begin(); iter != memoryList.end(); iter++)
	{
		if (!found)
		{
			if ( (iter->address <= sectionAddress) && (sectionAddress < (iter->address + iter->size)) )
			{
				if (wcslen(iter->peSection) == 0)
				{
					wcscpy_s((WCHAR *)iter->peSection, IMAGE_SIZEOF_SHORT_NAME * 4, sectionName);
				}
				else
				{
					wcscat_s((WCHAR *)iter->peSection, IMAGE_SIZEOF_SHORT_NAME * 4, L"|");
					wcscat_s((WCHAR *)iter->peSection, IMAGE_SIZEOF_SHORT_NAME * 4, sectionName);
				}
				
				found = true;
			}
		}
		else
		{
			if ((sectionSize+sectionAddress) < iter->address)
			{
				break;
			}
			if (wcslen(iter->peSection) == 0)
			{
				wcscpy_s((WCHAR *)iter->peSection, IMAGE_SIZEOF_SHORT_NAME * 4, sectionName);
			}
			else
			{
				wcscat_s((WCHAR *)iter->peSection, IMAGE_SIZEOF_SHORT_NAME * 4, L"|");
				wcscat_s((WCHAR *)iter->peSection, IMAGE_SIZEOF_SHORT_NAME * 4, sectionName);
			}
		}

	}
}

void DumpMemoryGui::setModuleName(DWORD_PTR moduleBase, const WCHAR * moduleName)
{
	bool found = false;
	std::vector<Memory>::const_iterator iter;

	//get filename
	const WCHAR* slash = wcsrchr(moduleName, L'\\');
	if(slash)
	{
		moduleName = slash+1;
	}


	for( iter = memoryList.begin(); iter != memoryList.end(); iter++)
	{
		if (iter->address == moduleBase)
		{
			found = true;
		}

		if (found)
		{
			if (iter->type == MEM_IMAGE)
			{
				wcscpy_s((WCHAR *)iter->filename, MAX_PATH, moduleName);
			}
			else
			{
				break;
			}
		}
	}
}

void DumpMemoryGui::setAllSectionNames( DWORD_PTR moduleBase, WCHAR * moduleName )
{
	WORD numSections = 0;
	PIMAGE_DOS_HEADER pDos = 0;
	PIMAGE_NT_HEADERS pNT = 0;
	PIMAGE_SECTION_HEADER pSec = 0;
	DWORD size = sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS) + 200;
	DWORD correctSize = 0;
	WCHAR sectionNameW[IMAGE_SIZEOF_SHORT_NAME + 1] = {0};
	CHAR sectionNameA[IMAGE_SIZEOF_SHORT_NAME + 1] = {0};

	BYTE * buffer = new BYTE[size];

	if (ProcessAccessHelp::readMemoryFromProcess(moduleBase,size,buffer))
	{
		pDos = (PIMAGE_DOS_HEADER)buffer;

		if (pDos->e_magic == IMAGE_DOS_SIGNATURE)
		{
			pNT = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDos + pDos->e_lfanew);
			if (pNT->Signature == IMAGE_NT_SIGNATURE)
			{
				numSections = pNT->FileHeader.NumberOfSections;
				correctSize = (numSections*sizeof(IMAGE_SECTION_HEADER)) + sizeof(IMAGE_NT_HEADERS) + pDos->e_lfanew;

				if (size < correctSize)
				{
					size = correctSize;
					delete [] buffer;
					buffer = new BYTE[size];
					if (!ProcessAccessHelp::readMemoryFromProcess(moduleBase,size,buffer))
					{
						delete [] buffer;
						return;
					}

					pDos = (PIMAGE_DOS_HEADER)buffer;
					pNT = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDos + pDos->e_lfanew);
				}

				pSec = IMAGE_FIRST_SECTION(pNT);

				for (WORD i = 0; i < numSections; i++)
				{
					ZeroMemory(sectionNameA, sizeof(sectionNameA));
					memcpy(sectionNameA,pSec->Name,8);
					swprintf_s(sectionNameW,L"%S",sectionNameA);

					setSectionName(moduleBase + pSec->VirtualAddress, pSec->Misc.VirtualSize,sectionNameW);
					pSec++;
				}

			}
		}
	}

	delete [] buffer;
}

bool DumpMemoryGui::dumpMemory()
{
	DWORD_PTR address = EditMemoryAddress.GetValue();
	dumpedMemorySize = EditMemorySize.GetValue();

	swprintf_s(dumpFilename,TEXT("MEM_")TEXT(PRINTF_DWORD_PTR_FULL_S)TEXT("_")TEXT("%08X"),address,dumpedMemorySize);

	dumpedMemory = new BYTE[dumpedMemorySize];

	if (dumpedMemory)
	{
		if (forceDump)
		{
			return ProcessAccessHelp::readMemoryPartlyFromProcess(address,dumpedMemorySize,dumpedMemory);
		}
		else
		{
			return ProcessAccessHelp::readMemoryFromProcess(address,dumpedMemorySize,dumpedMemory);
		}
		
	}
	else
	{
		return false;
	}
}

bool DumpMemoryGui::getMappedFilename( Memory* memory )
{
	WCHAR filename[MAX_PATH] = {0};

	//TODO replace with Nt direct syscall
	if (GetMappedFileNameW(ProcessAccessHelp::hProcess, (LPVOID)memory->address, filename, _countof(filename)) > 0)
	{
		return deviceNameResolver->resolveDeviceLongNameToShort(filename,memory->mappedFilename);
	}

	return false;
}
