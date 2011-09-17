#include "ImportsHandling.h"

#include "Thunks.h"
#include "definitions.h"

//#define DEBUG_COMMENTS

bool ImportModuleThunk::isValid()
{
	std::map<DWORD_PTR, ImportThunk>::iterator iterator = thunkList.begin();
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

DWORD_PTR ImportModuleThunk::getFirstThunk()
{
	if (thunkList.size() > 0)
	{
		std::map<DWORD_PTR, ImportThunk>::iterator iterator = thunkList.begin();
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
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;
	CTreeItem module;
	CTreeItem apiFunction;

	TreeImports.DeleteAllItems();

	 iterator1 = moduleList.begin();

	 while (iterator1 != moduleList.end())
	 {
		 moduleThunk = &(iterator1->second);

		 module = addDllToTreeView(TreeImports,moduleThunk->moduleName,moduleThunk->firstThunk,moduleThunk->thunkList.size(),moduleThunk->isValid());
		
		 moduleThunk->hTreeItem = module;

		 iterator2 = moduleThunk->thunkList.begin();

		 while (iterator2 != moduleThunk->thunkList.end())
		 {
			 importThunk = &(iterator2->second);
			 apiFunction = addApiToTreeView(TreeImports,module,importThunk);
			 importThunk->hTreeItem = apiFunction;
			 iterator2++;
		 }

		 iterator1++;
	 }

}

CTreeItem ImportsHandling::addDllToTreeView(CTreeViewCtrl& idTreeView, const WCHAR * dllName, DWORD_PTR firstThunk, size_t numberOfFunctions, bool valid)
{
	WCHAR validString[4];

	if (valid)
	{
		wcscpy_s(validString,_countof(validString),TEXT("YES"));
	}
	else
	{
		wcscpy_s(validString,_countof(validString),TEXT("NO"));
	}

	swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("%s FThunk: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" NbThunk: %02X (dec: %02d) valid: %s"),dllName,firstThunk,numberOfFunctions,numberOfFunctions,validString);
	
	return idTreeView.InsertItem(stringBuffer, NULL, TVI_ROOT);
}

CTreeItem ImportsHandling::addApiToTreeView(CTreeViewCtrl& idTreeView, CTreeItem parentDll, const ImportThunk * importThunk)
{
	if (importThunk->ordinal != 0)
	{
		if (importThunk->name[0] != 0x00)
		{
			swprintf_s(tempString, _countof(tempString),TEXT("ord: %04X name: %S"),importThunk->ordinal,importThunk->name);
		}
		else
		{
			swprintf_s(tempString, _countof(tempString),TEXT("ord: %04X"),importThunk->ordinal);
		}

		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("va: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" rva: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" mod: %s %s"),importThunk->va,importThunk->rva,importThunk->moduleName,tempString);
	}
	else
	{
		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("va: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" rva: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" ptr: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(""),importThunk->va,importThunk->rva,importThunk->apiAddressVA);
	}

	return idTreeView.InsertItem(stringBuffer, parentDll, TVI_LAST);
}

void ImportsHandling::showImports(bool invalid, bool suspect)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	TreeImports.SetFocus();
	TreeImports.SelectItem(NULL); //remove selection

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
				selectItem(TreeImports, importThunk->hTreeItem);
				setFocus(TreeImports, importThunk->hTreeItem);
			}
			else if (suspect && importThunk->suspect)
			{
				selectItem(TreeImports, importThunk->hTreeItem);
				setFocus(TreeImports, importThunk->hTreeItem);
			}
			else
			{
				unselectItem(TreeImports, importThunk->hTreeItem);
			}

			iterator2++;
		}

		iterator1++;
	}
}

bool ImportsHandling::isItemSelected(const CTreeViewCtrl& hwndTV, CTreeItem hItem)
{
	const UINT state = TVIS_SELECTED;
	return ((hwndTV.GetItemState(hItem, state) & state) == state);
}

void ImportsHandling::unselectItem(CTreeViewCtrl& hwndTV, CTreeItem htItem)
{
	selectItem(hwndTV, htItem, false);
}

bool ImportsHandling::selectItem(CTreeViewCtrl& hwndTV, CTreeItem hItem, bool select)
{
	const UINT state = TVIS_SELECTED;
	return FALSE != hwndTV.SetItemState(hItem, (select ? state : 0), state);
}

void ImportsHandling::setFocus(CTreeViewCtrl& hwndTV, CTreeItem htItem)
{
	// the current focus
	CTreeItem htFocus = hwndTV.GetSelectedItem();

	if ( htItem )
	{
		// set the focus
		if ( htItem != htFocus )
		{
			// remember the selection state of the item
			bool wasSelected = isItemSelected(hwndTV, htItem);

			if ( htFocus && isItemSelected(hwndTV, htFocus) )
			{
				// prevent the tree from unselecting the old focus which it
				// would do by default (TreeView_SelectItem unselects the
				// focused item)
				hwndTV.SelectItem(NULL);
				selectItem(hwndTV, htFocus);
			}

			hwndTV.SelectItem(htItem);

			if ( !wasSelected )
			{
				// need to clear the selection which TreeView_SelectItem() gave
				// us
				unselectItem(hwndTV, htItem);
			}
			//else: was selected, still selected - ok
		}
		//else: nothing to do, focus already there
	}
	else
	{
		if ( htFocus )
		{
			bool wasFocusSelected = isItemSelected(hwndTV, htFocus);

			// just clear the focus
			hwndTV.SelectItem(NULL);

			if ( wasFocusSelected )
			{
				// restore the selection state
				selectItem(hwndTV, htFocus);
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
			
				updateImportInTreeView(importThunk);
				updateModuleInTreeView(moduleThunk);
				return true;
			}

			iterator2++;
		}

		iterator1++;
	}

	return false;
}

void ImportsHandling::updateImportInTreeView(ImportThunk * importThunk)
{
	if (importThunk->ordinal != 0)
	{
		if (importThunk->name[0] != 0x00)
		{
			swprintf_s(tempString, _countof(tempString),TEXT("ord: %04X name: %S"),importThunk->ordinal,importThunk->name);
		}
		else
		{
			swprintf_s(tempString, _countof(tempString),TEXT("ord: %04X"),importThunk->ordinal);
		}

		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("va: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" rva: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" mod: %s %s"),importThunk->va,importThunk->rva,importThunk->moduleName,tempString);
	}
	else
	{
		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("va: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" rva: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" prt: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(""),importThunk->va,importThunk->rva,importThunk->apiAddressVA);
	}

	TreeImports.SetItemText(importThunk->hTreeItem, stringBuffer);
}

void ImportsHandling::updateModuleInTreeView(ImportModuleThunk * importThunk)
{
	WCHAR validString[4];

	if (importThunk->isValid())
	{
		wcscpy_s(validString,_countof(validString),TEXT("YES"));
	}
	else
	{
		wcscpy_s(validString,_countof(validString),TEXT("NO"));
	}

	swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("%s FThunk: ")TEXT(PRINTF_DWORD_PTR_HALF)TEXT(" NbThunk: %02X (dec: %02d) valid: %s"),importThunk->moduleName,importThunk->firstThunk,importThunk->thunkList.size(),importThunk->thunkList.size(),validString);

	TreeImports.SetItemText(importThunk->hTreeItem, stringBuffer);
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
				TreeImports.DeleteItem(importThunk->hTreeItem);
				moduleThunk->thunkList.erase(iterator2);

				if (moduleThunk->thunkList.empty())
				{
					TreeImports.DeleteItem(moduleThunk->hTreeItem);
					moduleList.erase(iterator1);
				}
				else
				{
					updateModuleInTreeView(moduleThunk);
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
			TreeImports.DeleteItem(moduleThunk->hTreeItem);

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
					TreeImports.DeleteItem(moduleThunk->hTreeItem);
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

	moduleList.clear();
	moduleList.insert(moduleListNew.begin(), moduleListNew.end());
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

		TreeImports.Expand(moduleThunk->hTreeItem, flag);

		iterator1++;
	}
}
