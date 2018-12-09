#include "ImportsHandling.h"

#include "Thunks.h"
#include "Architecture.h"

#include <atlmisc.h>
#include <atlcrack.h>
#include "multitree.h" // CMultiSelectTreeViewCtrl

#include "resource.h"

//#define DEBUG_COMMENTS

void ImportThunk::invalidate()
{
	ordinal = 0;
	hint = 0;
	valid = false;
	suspect = false;
	moduleName[0] = 0;
	name[0] = 0;
}

bool ImportModuleThunk::isValid() const
{
	std::map<DWORD_PTR, ImportThunk>::const_iterator iterator = thunkList.begin();
	while (iterator != thunkList.end())
	{
		if (iterator->second.valid == false)
		{
			return false;
		}
		iterator++;
	}

	return true;
}

DWORD_PTR ImportModuleThunk::getFirstThunk() const
{
	if (thunkList.size() > 0)
	{
		const std::map<DWORD_PTR, ImportThunk>::const_iterator iterator = thunkList.begin();
		return iterator->first;
	}
	else
	{
		return 0;
	}
}

ImportsHandling::ImportsHandling(CMultiSelectTreeViewCtrl& TreeImports) : TreeImports(TreeImports)
{
	hIconCheck.LoadIcon(IDI_ICON_CHECK, 16, 16);
	hIconWarning.LoadIcon(IDI_ICON_WARNING, 16, 16);
	hIconError.LoadIcon(IDI_ICON_ERROR, 16, 16);

	CDCHandle dc = CWindow(::GetDesktopWindow()).GetDC();
	int bits = dc.GetDeviceCaps(BITSPIXEL);

	const UINT FLAGS = bits > 16 ? ILC_COLOR32 : (ILC_COLOR24 | ILC_MASK);

	TreeIcons.Create(16, 16, FLAGS, 3, 1);
	TreeIcons.AddIcon(hIconCheck);
	TreeIcons.AddIcon(hIconWarning);
	TreeIcons.AddIcon(hIconError);

	m_thunkCount = m_invalidThunkCount = m_suspectThunkCount = 0;
}

ImportsHandling::~ImportsHandling()
{
	TreeIcons.Destroy();
}

bool ImportsHandling::isModule(CTreeItem item) 
{
	return (0 != getModuleThunk(item));
}

bool ImportsHandling::isImport(CTreeItem item)
{
	return (0 != getImportThunk(item));
}

ImportModuleThunk * ImportsHandling::getModuleThunk(CTreeItem item)
{
	std::unordered_map<HTREEITEM, TreeItemData>::const_iterator it;
	it = itemData.find(item);
	if(it != itemData.end())
	{
		const TreeItemData * data = &it->second;
		if(data->isModule)
		{
			return data->module;
		}
	}
	return NULL;
}

ImportThunk * ImportsHandling::getImportThunk(CTreeItem item)
{
	std::unordered_map<HTREEITEM, TreeItemData>::const_iterator it;
	TreeItemData * data = getItemData(item);
	if(data && !data->isModule)
	{
		return data->import;
	}
	return NULL;
}

void ImportsHandling::setItemData(CTreeItem item, const TreeItemData * data)
{
	itemData[item] = *data;
}

ImportsHandling::TreeItemData * ImportsHandling::getItemData(CTreeItem item)
{
	std::unordered_map<HTREEITEM, TreeItemData>::iterator it;
	it = itemData.find(item);
	if(it != itemData.end())
	{
		return &it->second;
	}
	return NULL;
}

void ImportsHandling::updateCounts()
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	m_thunkCount = m_invalidThunkCount = m_suspectThunkCount = 0;

	it_module = moduleList.begin();
	while (it_module != moduleList.end())
	{
		ImportModuleThunk &moduleThunk = it_module->second;

		it_import = moduleThunk.thunkList.begin();
		while (it_import != moduleThunk.thunkList.end())
		{
			ImportThunk &importThunk = it_import->second;

			m_thunkCount++;
			if(!importThunk.valid)
				m_invalidThunkCount++;
			else if(importThunk.suspect)
				m_suspectThunkCount++;

			it_import++;
		}

		it_module++;
	}
}

/*bool ImportsHandling::addImport(const WCHAR * moduleName, const CHAR * name, DWORD_PTR va, DWORD_PTR rva, WORD ordinal, bool valid, bool suspect)
{
	ImportThunk import;
	ImportModuleThunk  * module = 0;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;

	if (moduleList.size() > 1)
	{
		iterator1 = moduleList.begin();
		while (iterator1 != moduleList.end())
		{
			if (rva >= iterator1->second.firstThunk)
			{
				iterator1++;
				if (iterator1 == moduleList.end())
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
				else if (rva < iterator1->second.firstThunk)
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
			}
		}
	}
	else
	{
		iterator1 = moduleList.begin();
		module = &(iterator1->second);
	}

	if (!module)
	{
		Scylla::debugLog.log(L"ImportsHandling::addFunction module not found rva " PRINTF_DWORD_PTR_FULL, rva);
		return false;
	}

	//TODO
	import.suspect = true;
	import.valid = false;
	import.va = va;
	import.rva = rva;
	import.ordinal = ordinal;

	wcscpy_s(import.moduleName, MAX_PATH, moduleName);
	strcpy_s(import.name, MAX_PATH, name);

	module->thunkList.insert(std::pair<DWORD_PTR,ImportThunk>(import.rva, import));

	return true;
}
*/

/*
bool ImportsHandling::addModule(const WCHAR * moduleName, DWORD_PTR firstThunk)
{
	ImportModuleThunk module;

	module.firstThunk = firstThunk;
	wcscpy_s(module.moduleName, MAX_PATH, moduleName);

	moduleList.insert(std::pair<DWORD_PTR,ImportModuleThunk>(firstThunk,module));

	return true;
}
*/

void ImportsHandling::displayAllImports()
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	TreeImports.DeleteAllItems();
	itemData.clear();
	TreeImports.SetImageList(TreeIcons);

	it_module = moduleList.begin();
	while (it_module != moduleList.end())
	{
		ImportModuleThunk &moduleThunk = it_module->second;

		moduleThunk.key = moduleThunk.firstThunk; // This belongs elsewhere...
		moduleThunk.hTreeItem = addDllToTreeView(TreeImports, &moduleThunk);

		it_import = moduleThunk.thunkList.begin();
		while (it_import != moduleThunk.thunkList.end())
		{
			ImportThunk &importThunk = it_import->second;

			importThunk.key = importThunk.rva; // This belongs elsewhere...
			importThunk.hTreeItem = addApiToTreeView(TreeImports, moduleThunk.hTreeItem, &importThunk);

			it_import++;
		}

		it_module++;
	}

	updateCounts();
}

void ImportsHandling::clearAllImports()
{
	TreeImports.DeleteAllItems();
	itemData.clear();
	moduleList.clear();
	updateCounts();
}

CTreeItem ImportsHandling::addDllToTreeView(CMultiSelectTreeViewCtrl& idTreeView, ImportModuleThunk * moduleThunk)
{
	CTreeItem item = idTreeView.InsertItem(L"", NULL, TVI_ROOT);

	item.SetData(itemData.size());

	TreeItemData data;
	data.isModule = true;
	data.module = moduleThunk;

	setItemData(item, &data);

	updateModuleInTreeView(moduleThunk, item);
	return item;
}

CTreeItem ImportsHandling::addApiToTreeView(CMultiSelectTreeViewCtrl& idTreeView, CTreeItem parentDll, ImportThunk * importThunk)
{
	CTreeItem item = idTreeView.InsertItem(L"", parentDll, TVI_LAST);

	item.SetData(itemData.size());

	TreeItemData data;
	data.isModule = false;
	data.import = importThunk;

	setItemData(item, &data);

	updateImportInTreeView(importThunk, item);
	return item;
}

void ImportsHandling::selectImports(bool invalid, bool suspect)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	TreeImports.SelectAllItems(FALSE); //remove selection

	it_module = moduleList.begin();
	while (it_module != moduleList.end())
	{
		ImportModuleThunk &moduleThunk = it_module->second;

		it_import = moduleThunk.thunkList.begin();
		while (it_import != moduleThunk.thunkList.end())
		{
			ImportThunk &importThunk = it_import->second;

			if ((invalid && !importThunk.valid) || (suspect && importThunk.suspect))
			{
				TreeImports.SelectItem(importThunk.hTreeItem, TRUE);
				importThunk.hTreeItem.EnsureVisible();
			}

			it_import++;
		}

		it_module++;
	}
}

bool ImportsHandling::invalidateImport(CTreeItem item)
{
	ImportThunk * import = getImportThunk(item);
	if(import)
	{
		CTreeItem parent = item.GetParent();
		if(!parent.IsNull())
		{
			const ImportModuleThunk * module = getModuleThunk(parent);
			if(module)
			{
				import->invalidate();

				updateImportInTreeView(import, import->hTreeItem);
				updateModuleInTreeView(module, module->hTreeItem);

				updateCounts();
				return true;
			}
		}
	}
	return false;
}

bool ImportsHandling::invalidateModule(CTreeItem item)
{
	ImportModuleThunk * module = getModuleThunk(item);
	if(module)
	{
		std::map<DWORD_PTR, ImportThunk>::iterator it_import;

		it_import = module->thunkList.begin();
		while(it_import != module->thunkList.end())
		{
			ImportThunk * import = &it_import->second;
			import->invalidate();
			updateImportInTreeView(import, import->hTreeItem);
			it_import++;
		}

		updateModuleInTreeView(module, module->hTreeItem);

		updateCounts();
		return true;
	}
	return false;
}

bool ImportsHandling::setImport(CTreeItem item, const WCHAR * moduleName, const CHAR * apiName, WORD ordinal, WORD hint, bool valid, bool suspect)
{
	ImportThunk * import = getImportThunk(item);
	if(import)
	{
		CTreeItem parent = item.GetParent();
		if(!parent.IsNull())
		{
			ImportModuleThunk * module = getModuleThunk(parent);
			if(module)
			{

				wcscpy_s(import->moduleName, moduleName);
				strcpy_s(import->name, apiName);
				import->ordinal = ordinal;
				//import->apiAddressVA = api->va; //??
				import->hint = hint;
				import->valid = valid;
				import->suspect = suspect;

				if (module->isValid())
				{
					scanAndFixModuleList();
					displayAllImports();
				}
				else
				{
					updateImportInTreeView(import, item);
					updateCounts();
				}
				return true;
			}
		}
	}
	return false;
}

void ImportsHandling::updateImportInTreeView(const ImportThunk * importThunk, CTreeItem item)
{
	if (importThunk->valid)
	{
		WCHAR tempString[300];

		if (importThunk->name[0] != 0x00)
		{
			swprintf_s(tempString, L"ord: %04X name: %S", importThunk->ordinal, importThunk->name);
		}
		else
		{
			swprintf_s(tempString, L"ord: %04X", importThunk->ordinal);
		}

		swprintf_s(stringBuffer, L" rva: " PRINTF_DWORD_PTR_HALF L" mod: %s %s", importThunk->rva, importThunk->moduleName, tempString);
	}
	else
	{
		swprintf_s(stringBuffer, L" rva: " PRINTF_DWORD_PTR_HALF L" ptr: " PRINTF_DWORD_PTR_FULL, importThunk->rva, importThunk->apiAddressVA);
	}

	item.SetText(stringBuffer);
	Icon icon = getAppropiateIcon(importThunk);
	item.SetImage(icon, icon);
}

void ImportsHandling::updateModuleInTreeView(const ImportModuleThunk * importThunk, CTreeItem item)
{
	swprintf_s(stringBuffer, L"%s (%d) FThunk: " PRINTF_DWORD_PTR_HALF, importThunk->moduleName,importThunk->thunkList.size(), importThunk->firstThunk);

	item.SetText(stringBuffer);
	Icon icon = getAppropiateIcon(importThunk->isValid());
	item.SetImage(icon, icon);
}

ImportsHandling::Icon ImportsHandling::getAppropiateIcon(const ImportThunk * importThunk)
{
	if(importThunk->valid)
	{
		if(importThunk->suspect)
		{
			return iconWarning;
		}
		else
		{
			return iconCheck;
		}
	}
	else
	{
		return iconError;
	}
}

ImportsHandling::Icon ImportsHandling::getAppropiateIcon(bool valid)
{
	if(valid)
	{
		return iconCheck;
	}
	else
	{
		return iconError;
	}
}

bool ImportsHandling::cutImport(CTreeItem item)
{
	ImportThunk * import = getImportThunk(item);
	if(import)
	{
		CTreeItem parent = item.GetParent();
		if(!parent.IsNull())
		{
			ImportModuleThunk * module = getModuleThunk(parent);
			if(module)
			{
				itemData.erase(item);
				import->hTreeItem.Delete();
				module->thunkList.erase(import->key);
				import = 0;

				if (module->thunkList.empty())
				{
					itemData.erase(parent);
					module->hTreeItem.Delete();
					moduleList.erase(module->key);
					module = 0;
				}
				else
				{
					if (module->isValid() && module->moduleName[0] == L'?')
					{
						//update module name
						wcscpy_s(module->moduleName, module->thunkList.begin()->second.moduleName);
					}

					module->firstThunk = module->thunkList.begin()->second.rva;
					updateModuleInTreeView(module, module->hTreeItem);
				}

				updateCounts();
				return true;
			}
		}
	}
	return false;
}

bool ImportsHandling::cutModule(CTreeItem item)
{
	ImportModuleThunk * module = getModuleThunk(item);
	if(module)
	{
		CTreeItem child = item.GetChild();
		while(!child.IsNull())
		{
			itemData.erase(child);
			child = child.GetNextSibling();
		}
		itemData.erase(item);
		module->hTreeItem.Delete();
		moduleList.erase(module->key);
		module = 0;
		updateCounts();
		return true;
	}
	return false;
}

DWORD_PTR ImportsHandling::getApiAddressByNode(CTreeItem item)
{
	const ImportThunk * import = getImportThunk(item);
	if(import)
	{
		return import->apiAddressVA;
	}
	return 0;
}

void ImportsHandling::scanAndFixModuleList()
{
	WCHAR prevModuleName[MAX_PATH] = {0};
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_import;

	it_module = moduleList.begin();
	while (it_module != moduleList.end())
	{
		ImportModuleThunk &moduleThunk = it_module->second;

		it_import = moduleThunk.thunkList.begin();

		ImportThunk * importThunkPrev;
		importThunkPrev = &it_import->second;

		while (it_import != moduleThunk.thunkList.end())
		{
			ImportThunk &importThunk = it_import->second;

			if (importThunk.moduleName[0] == 0 || importThunk.moduleName[0] == L'?')
			{
				addNotFoundApiToModuleList(&importThunk);
			}
			else 
			{
				
				if (_wcsicmp(importThunk.moduleName, prevModuleName))
				{
					addModuleToModuleList(importThunk.moduleName, importThunk.rva);
				}
				
				addFunctionToModuleList(&importThunk);
			}

			wcscpy_s(prevModuleName, importThunk.moduleName);
			it_import++;
		}

		moduleThunk.thunkList.clear();

		it_module++;
	}

	moduleList = moduleListNew;
	moduleListNew.clear();
}

bool ImportsHandling::findNewModules(std::map<DWORD_PTR, ImportThunk> & thunkList)
{
	throw std::exception("The method or operation is not implemented.");
}

bool ImportsHandling::addModuleToModuleList(const WCHAR * moduleName, DWORD_PTR firstThunk)
{
	ImportModuleThunk module;

	module.firstThunk = firstThunk;
	wcscpy_s(module.moduleName, moduleName);

	module.key = module.firstThunk;
	moduleListNew[module.key] = module;
	return true;
}

bool ImportsHandling::isNewModule(const WCHAR * moduleName)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;

	it_module = moduleListNew.begin();
	while (it_module != moduleListNew.end())
	{
		if (!_wcsicmp(it_module->second.moduleName, moduleName))
		{
			return false;
		}

		it_module++;
	}

	return true;
}

void ImportsHandling::addUnknownModuleToModuleList(DWORD_PTR firstThunk)
{
	ImportModuleThunk module;

	module.firstThunk = firstThunk;
	wcscpy_s(module.moduleName, L"?");

	module.key = module.firstThunk;
	moduleListNew[module.key] = module;
}

bool ImportsHandling::addNotFoundApiToModuleList(const ImportThunk * apiNotFound)
{
	ImportThunk import;
	ImportModuleThunk  * module = 0;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	DWORD_PTR rva = apiNotFound->rva;

	if (moduleListNew.size() > 0)
	{
		it_module = moduleListNew.begin();
		while (it_module != moduleListNew.end())
		{
			if (rva >= it_module->second.firstThunk)
			{
				it_module++;
				if (it_module == moduleListNew.end())
				{
					it_module--;
					//new unknown module
					if (it_module->second.moduleName[0] == L'?')
					{
						module = &(it_module->second);
					}
					else
					{
						addUnknownModuleToModuleList(apiNotFound->rva);
						module = &(moduleListNew.find(rva)->second);
					}

					break;
				}
				else if (rva < it_module->second.firstThunk)
				{
					it_module--;
					module = &(it_module->second);
					break;
				}
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"Error iterator1 != (*moduleThunkList).end()");
#endif
				break;
			}
		}
	}
	else
	{
		//new unknown module
		addUnknownModuleToModuleList(apiNotFound->rva);
		module = &(moduleListNew.find(rva)->second);
	}

	if (!module)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"ImportsHandling::addFunction module not found rva " PRINTF_DWORD_PTR_FULL, rva);
#endif
		return false;
	}


	import.suspect = true;
	import.valid = false;
	import.va = apiNotFound->va;
	import.rva = apiNotFound->rva;
	import.apiAddressVA = apiNotFound->apiAddressVA;
	import.ordinal = 0;

	wcscpy_s(import.moduleName, L"?");
	strcpy_s(import.name, "?");

	import.key = import.rva;
	module->thunkList[import.key] = import;
	return true;
}

bool ImportsHandling::addFunctionToModuleList(const ImportThunk * apiFound)
{
	ImportThunk import;
	ImportModuleThunk  * module = 0;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;

	if (moduleListNew.size() > 1)
	{
		it_module = moduleListNew.begin();
		while (it_module != moduleListNew.end())
		{
			if (apiFound->rva >= it_module->second.firstThunk)
			{
				it_module++;
				if (it_module == moduleListNew.end())
				{
					it_module--;
					module = &(it_module->second);
					break;
				}
				else if (apiFound->rva < it_module->second.firstThunk)
				{
					it_module--;
					module = &(it_module->second);
					break;
				}
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Scylla::debugLog.log(L"Error iterator1 != moduleListNew.end()");
#endif
				break;
			}
		}
	}
	else
	{
		it_module = moduleListNew.begin();
		module = &(it_module->second);
	}

	if (!module)
	{
#ifdef DEBUG_COMMENTS
		Scylla::debugLog.log(L"ImportsHandling::addFunction module not found rva " PRINTF_DWORD_PTR_FULL, apiFound->rva);
#endif
		return false;
	}


	import.suspect = apiFound->suspect;
	import.valid = apiFound->valid;
	import.va = apiFound->va;
	import.rva = apiFound->rva;
	import.apiAddressVA = apiFound->apiAddressVA;
	import.ordinal = apiFound->ordinal;
	import.hint = apiFound->hint;

	wcscpy_s(import.moduleName, apiFound->moduleName);
	strcpy_s(import.name, apiFound->name);

	import.key = import.rva;
	module->thunkList[import.key] = import;
	return true;
}

void ImportsHandling::expandAllTreeNodes()
{
	changeExpandStateOfTreeNodes(TVE_EXPAND);
}

void ImportsHandling::collapseAllTreeNodes()
{
	changeExpandStateOfTreeNodes(TVE_COLLAPSE);
}

void ImportsHandling::changeExpandStateOfTreeNodes(UINT flag)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;

	it_module = moduleList.begin();
	while (it_module != moduleList.end())
	{
		ImportModuleThunk &moduleThunk = it_module->second;

		moduleThunk.hTreeItem.Expand(flag);

		it_module++;
	}
}

