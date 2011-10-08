#include "ImportsHandling.h"

#include "Thunks.h"
#include "definitions.h"

#include <atlmisc.h>
#include <atlcrack.h>
#include "multitree.h" // CMultiSelectTreeViewCtrl

#include "resource.h"

//#define DEBUG_COMMENTS

ImportsHandling::ImportsHandling(CMultiSelectTreeViewCtrl& TreeImports) : TreeImports(TreeImports)
{
	hIconCheck.LoadIcon(IDI_ICON_CHECK);
	hIconWarning.LoadIcon(IDI_ICON_WARNING);
	hIconError.LoadIcon(IDI_ICON_ERROR);

	TreeIcons.Create(16, 16, ILC_COLOR32, 3, 1);
	TreeIcons.AddIcon(hIconCheck);
	TreeIcons.AddIcon(hIconWarning);
	TreeIcons.AddIcon(hIconError);
}

ImportsHandling::~ImportsHandling()
{
	TreeIcons.Destroy();
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

/*bool ImportsHandling::addModule(WCHAR * moduleName, DWORD_PTR firstThunk)
{
	ImportModuleThunk module;

	module.firstThunk = firstThunk;
	wcscpy_s(module.moduleName, MAX_PATH, moduleName);

	moduleList.insert(std::pair<DWORD_PTR,ImportModuleThunk>(firstThunk,module));

	return true;
}*/

/*bool ImportsHandling::addFunction(WCHAR * moduleName, char * name, DWORD_PTR va, DWORD_PTR rva, DWORD_PTR ordinal, bool valid, bool suspect)
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
		Logger::debugLog(TEXT("ImportsHandling::addFunction module not found rva ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(""),rva);
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
}*/

void ImportsHandling::displayAllImports()
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator it_module;
	std::map<DWORD_PTR, ImportThunk>::iterator it_thunk;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;
	CTreeItem module;
	CTreeItem apiFunction;

	TreeImports.DeleteAllItems();
	TreeImports.SetImageList(TreeIcons);

	 it_module = moduleList.begin();

	 while (it_module != moduleList.end())
	 {
		 moduleThunk = &(it_module->second);

		 module = addDllToTreeView(TreeImports,moduleThunk);
		
		 moduleThunk->hTreeItem = module;

		 it_thunk = moduleThunk->thunkList.begin();

		 while (it_thunk != moduleThunk->thunkList.end())
		 {
			 importThunk = &(it_thunk->second);
			 apiFunction = addApiToTreeView(TreeImports,module,importThunk);
			 importThunk->hTreeItem = apiFunction;
			 it_thunk++;
		 }

		 it_module++;
	 }
}

CTreeItem ImportsHandling::addDllToTreeView(CMultiSelectTreeViewCtrl& idTreeView, const ImportModuleThunk * importThunk)
{
	CTreeItem item = idTreeView.InsertItem(L"", NULL, TVI_ROOT);
	updateModuleInTreeView(importThunk, item);
	return item;
}

CTreeItem ImportsHandling::addApiToTreeView(CMultiSelectTreeViewCtrl& idTreeView, CTreeItem parentDll, const ImportThunk * importThunk)
{
	CTreeItem item = idTreeView.InsertItem(L"", parentDll, TVI_LAST);
	updateImportInTreeView(importThunk, item);
	return item;
}

void ImportsHandling::showImports(bool invalid, bool suspect)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	TreeImports.SetFocus(); // should be GotoDlgCtrl...
	TreeImports.SelectAllItems(FALSE); //remove selection

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (invalid && !importThunk->valid)
			{
				selectItem(importThunk->hTreeItem);
				setFocus(TreeImports, importThunk->hTreeItem);
			}
			else if (suspect && importThunk->suspect)
			{
				selectItem(importThunk->hTreeItem);
				setFocus(TreeImports, importThunk->hTreeItem);
			}
			else
			{
				unselectItem(importThunk->hTreeItem);
			}

			iterator2++;
		}

		iterator1++;
	}
}

bool ImportsHandling::isItemSelected(CTreeItem hItem)
{
	const UINT state = TVIS_SELECTED;
	return ((hItem.GetState(state) & state) == state);
}

void ImportsHandling::unselectItem(CTreeItem htItem)
{
	selectItem(htItem, false);
}

bool ImportsHandling::selectItem(CTreeItem hItem, bool select)
{
	const UINT state = TVIS_SELECTED;
	return FALSE != hItem.SetState((select ? state : 0), state);
}

void ImportsHandling::setFocus(CMultiSelectTreeViewCtrl& hwndTV, CTreeItem htItem)
{
	// the current focus
	CTreeItem htFocus = hwndTV.GetFirstSelectedItem();

	if ( htItem )
	{
		// set the focus
		if ( htItem != htFocus )
		{
			// remember the selection state of the item
			bool wasSelected = isItemSelected(htItem);

			if ( htFocus && isItemSelected(htFocus) )
			{
				// prevent the tree from unselecting the old focus which it
				// would do by default (TreeView_SelectItem unselects the
				// focused item)
				hwndTV.SelectAllItems(FALSE);
				selectItem(htFocus);
			}

			hwndTV.SelectItem(htItem, FALSE);

			if ( !wasSelected )
			{
				// need to clear the selection which TreeView_SelectItem() gave
				// us
				unselectItem(htItem);
			}
			//else: was selected, still selected - ok
		}
		//else: nothing to do, focus already there
	}
	else
	{
		if ( htFocus )
		{
			bool wasFocusSelected = isItemSelected(htFocus);

			// just clear the focus
			hwndTV.SelectItem(NULL, FALSE);

			if ( wasFocusSelected )
			{
				// restore the selection state
				selectItem(htFocus);
			}
		}
		//else: nothing to do, no focus already
	}
}

bool ImportsHandling::invalidateFunction(CTreeItem selectedTreeNode)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;


	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (importThunk->hTreeItem == selectedTreeNode)
			{
				importThunk->ordinal = 0;
				importThunk->hint = 0;
				importThunk->valid = false;
				importThunk->suspect = false;
				importThunk->moduleName[0] = 0;
				importThunk->name[0] = 0;

				updateImportInTreeView(importThunk, importThunk->hTreeItem);
				updateModuleInTreeView(moduleThunk, moduleThunk->hTreeItem);
				return true;
			}

			iterator2++;
		}

		iterator1++;
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
			swprintf_s(tempString, _countof(tempString),TEXT("ord: %04X name: %S"),importThunk->ordinal,importThunk->name);
		}
		else
		{
			swprintf_s(tempString, _countof(tempString),TEXT("ord: %04X"),importThunk->ordinal);
		}

		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT(" rva: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" mod: %s %s"),importThunk->rva,importThunk->moduleName,tempString);
	}
	else
	{
		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT(" rva: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" ptr: ")TEXT(PRINTF_DWORD_PTR_FULL),importThunk->rva,importThunk->apiAddressVA);
	}

	item.SetText(stringBuffer);
	Icon icon = getAppropiateIcon(importThunk);
	item.SetImage(icon, icon);
}

void ImportsHandling::updateModuleInTreeView(const ImportModuleThunk * importThunk, CTreeItem item)
{
	swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("%s (%d) FThunk: ")TEXT(PRINTF_DWORD_PTR_HALF),importThunk->moduleName,importThunk->thunkList.size(), importThunk->firstThunk);

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

bool ImportsHandling::cutThunk(CTreeItem selectedTreeNode)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (importThunk->hTreeItem == selectedTreeNode)
			{
				importThunk->hTreeItem.Delete();
				moduleThunk->thunkList.erase(iterator2);

				if (moduleThunk->thunkList.empty())
				{
					moduleThunk->hTreeItem.Delete();
					moduleList.erase(iterator1);
				}
				else
				{
					updateModuleInTreeView(moduleThunk, moduleThunk->hTreeItem);
				}
				return true;
			}

			iterator2++;
		}

		iterator1++;
	}

	return false;
}

bool ImportsHandling::deleteTreeNode(CTreeItem selectedTreeNode)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		if (moduleThunk->hTreeItem == selectedTreeNode)
		{
			moduleThunk->hTreeItem.Delete();

			moduleThunk->thunkList.clear();
			moduleList.erase(iterator1);
			return true;
		}
		else
		{
			iterator2 = moduleThunk->thunkList.begin();

			while (iterator2 != moduleThunk->thunkList.end())
			{
				importThunk = &(iterator2->second);

				if (importThunk->hTreeItem == selectedTreeNode)
				{
					moduleThunk->hTreeItem.Delete();
					moduleThunk->thunkList.clear();
					moduleList.erase(iterator1);
					return true;
				}

				iterator2++;
			}
		}

		iterator1++;
	}

	return false;
}

DWORD_PTR ImportsHandling::getApiAddressByNode(CTreeItem selectedTreeNode)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (importThunk->hTreeItem == selectedTreeNode)
			{
				return importThunk->apiAddressVA;
			}

			iterator2++;
		}

		iterator1++;
	}
	return 0;
}


void ImportsHandling::scanAndFixModuleList()
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;


	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		iterator2 = moduleThunk->thunkList.begin();

		while (iterator2 != moduleThunk->thunkList.end())
		{
			importThunk = &(iterator2->second);

			if (importThunk->moduleName[0] == 0 || importThunk->moduleName[0] == L'?')
			{
				addNotFoundApiToModuleList(importThunk);
			}
			else 
			{
				if (isNewModule(importThunk->moduleName))
				{
					addModuleToModuleList(importThunk->moduleName, importThunk->rva);
				}
				
				addFunctionToModuleList(importThunk);
			}

			iterator2++;
		}

		moduleThunk->thunkList.clear();

		iterator1++;
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
	wcscpy_s(module.moduleName, MAX_PATH, moduleName);

	moduleListNew.insert(std::pair<DWORD_PTR,ImportModuleThunk>(firstThunk,module));

	return true;
}

bool ImportsHandling::isNewModule(const WCHAR * moduleName)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;

	iterator1 = moduleListNew.begin();

	while (iterator1 != moduleListNew.end())
	{
		if (!_wcsicmp(iterator1->second.moduleName, moduleName))
		{
			return false;
		}

		iterator1++;
	}

	return true;
}

void ImportsHandling::addUnknownModuleToModuleList(DWORD_PTR firstThunk)
{
	ImportModuleThunk module;

	module.firstThunk = firstThunk;
	wcscpy_s(module.moduleName, MAX_PATH, L"?");

	moduleListNew.insert(std::pair<DWORD_PTR,ImportModuleThunk>(firstThunk,module));
}

bool ImportsHandling::addNotFoundApiToModuleList(const ImportThunk * apiNotFound)
{
	ImportThunk import;
	ImportModuleThunk  * module = 0;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	DWORD_PTR rva = apiNotFound->rva;

	if (moduleListNew.size() > 0)
	{
		iterator1 = moduleListNew.begin();
		while (iterator1 != moduleListNew.end())
		{
			if (rva >= iterator1->second.firstThunk)
			{
				iterator1++;
				if (iterator1 == moduleListNew.end())
				{
					iterator1--;
					//new unknown module
					if (iterator1->second.moduleName[0] == L'?')
					{
						module = &(iterator1->second);
					}
					else
					{
						addUnknownModuleToModuleList(apiNotFound->rva);
						module = &(moduleListNew.find(rva)->second);
					}

					break;
				}
				else if (rva < iterator1->second.firstThunk)
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog("Error iterator1 != (*moduleThunkList).end()\r\n");
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
		Logger::debugLog(TEXT("ImportsHandling::addFunction module not found rva ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"),rva);
#endif
		return false;
	}


	import.suspect = true;
	import.valid = false;
	import.va = apiNotFound->va;
	import.rva = apiNotFound->rva;
	import.apiAddressVA = apiNotFound->apiAddressVA;
	import.ordinal = 0;

	wcscpy_s(import.moduleName, MAX_PATH, L"?");
	strcpy_s(import.name, MAX_PATH, "?");

	module->thunkList.insert(std::pair<DWORD_PTR,ImportThunk>(import.rva, import));

	return true;
}

bool ImportsHandling::addFunctionToModuleList(const ImportThunk * apiFound)
{
	ImportThunk import;
	ImportModuleThunk  * module = 0;
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;

	if (moduleListNew.size() > 1)
	{
		iterator1 = moduleListNew.begin();
		while (iterator1 != moduleListNew.end())
		{
			if (apiFound->rva >= iterator1->second.firstThunk)
			{
				iterator1++;
				if (iterator1 == moduleListNew.end())
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
				else if (apiFound->rva < iterator1->second.firstThunk)
				{
					iterator1--;
					module = &(iterator1->second);
					break;
				}
			}
			else
			{
#ifdef DEBUG_COMMENTS
				Logger::debugLog(TEXT("Error iterator1 != moduleListNew.end()\r\n"));
#endif
				break;
			}
		}
	}
	else
	{
		iterator1 = moduleListNew.begin();
		module = &(iterator1->second);
	}

	if (!module)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(TEXT("ImportsHandling::addFunction module not found rva ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\n"),apiFound->rva);
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

	wcscpy_s(import.moduleName, MAX_PATH, apiFound->moduleName);
	strcpy_s(import.name, MAX_PATH, apiFound->name);

	module->thunkList.insert(std::pair<DWORD_PTR,ImportThunk>(import.rva, import));

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
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	ImportModuleThunk * moduleThunk;

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		moduleThunk->hTreeItem.Expand(flag);

		iterator1++;
	}
}
