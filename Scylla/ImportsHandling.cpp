
#include "ImportsHandling.h"
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
	HTREEITEM module;
	HTREEITEM apiFunction;
	//HWND idTreeView = GetDlgItem(hWndMainDlg, IDC_TREE_IMPORTS);

	//TreeView_DeleteAllItems(idTreeView);

	TreeImports.DeleteAllItems();

	 iterator1 = moduleList.begin();

	 while (iterator1 != moduleList.end())
	 {
		 moduleThunk = &(iterator1->second);

		 module = addDllToTreeView(TreeImports/*idTreeView*/,moduleThunk->moduleName,moduleThunk->firstThunk,moduleThunk->thunkList.size(),moduleThunk->isValid());
		
		 moduleThunk->hTreeItem = module;

		 iterator2 = moduleThunk->thunkList.begin();

		 while (iterator2 != moduleThunk->thunkList.end())
		 {
			 importThunk = &(iterator2->second);
			 apiFunction = addApiToTreeView(TreeImports/*idTreeView*/,module,importThunk);
			 importThunk->hTreeItem = apiFunction;
			 iterator2++;
		 }

		 iterator1++;
	 }

}

HTREEITEM ImportsHandling::addDllToTreeView(CTreeViewCtrl& idTreeView, const WCHAR * dllName, DWORD_PTR firstThunk, size_t numberOfFunctions, bool valid)
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
	
	tvInsert.hParent = NULL;
	tvInsert.hInsertAfter = TVI_ROOT;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvInsert.item.pszText = stringBuffer;
	//return TreeView_InsertItem(idTreeView, &tvInsert);
	return idTreeView.InsertItem(&tvInsert);
}

HTREEITEM ImportsHandling::addApiToTreeView(CTreeViewCtrl& idTreeView, HTREEITEM parentDll, ImportThunk * importThunk)
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


	tvInsert.hParent = parentDll;
	tvInsert.hInsertAfter = TVI_LAST;
	tvInsert.item.mask = TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
	tvInsert.item.pszText = stringBuffer;
	//return TreeView_InsertItem(idTreeView, &tvInsert);
	return idTreeView.InsertItem(&tvInsert);
}

void ImportsHandling::showImports(bool invalid, bool suspect)
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	//HWND idTreeView = GetDlgItem(hWndMainDlg, IDC_TREE_IMPORTS);

	//SetFocus(idTreeView);
	//TreeView_SelectItem(idTreeView,0); //remove selection

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

bool ImportsHandling::isItemSelected(CTreeViewCtrl& hwndTV, HTREEITEM hItem)
{
	TV_ITEM tvi;
	tvi.mask = TVIF_STATE | TVIF_HANDLE;
	tvi.stateMask = TVIS_SELECTED;
	tvi.hItem = hItem;

	//TreeView_GetItem(hwndTV, &tvi);
	hwndTV.GetItem(&tvi);

	return (tvi.state & TVIS_SELECTED) != 0;
}

void ImportsHandling::unselectItem(CTreeViewCtrl& hwndTV, HTREEITEM htItem)
{
	selectItem(hwndTV, htItem, false);
}

bool ImportsHandling::selectItem(CTreeViewCtrl& hwndTV, HTREEITEM hItem, bool select)
{
	TV_ITEM tvi;
	tvi.mask = TVIF_STATE | TVIF_HANDLE;
	tvi.stateMask = TVIS_SELECTED;
	tvi.state = select ? TVIS_SELECTED : 0;
	tvi.hItem = hItem;


	//*if ( TreeView_SetItem(hwndTV, &tvi) == -1 )
	if ( hwndTV.SetItem(&tvi) == -1 )
	{
		return false;
	}

	return true;
}

void ImportsHandling::setFocus(CTreeViewCtrl& hwndTV, HTREEITEM htItem)
{
	// the current focus
	HTREEITEM htFocus = hwndTV.GetSelectedItem(); //(HTREEITEM)TreeView_GetSelection(hwndTV);

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
				hwndTV.SelectItem(NULL); //TreeView_SelectItem(hwndTV, 0);
				selectItem(hwndTV, htFocus);
			}

			hwndTV.SelectItem(htItem); //TreeView_SelectItem(hwndTV, htItem);

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
			hwndTV.SelectItem(NULL); //TreeView_SelectItem(hwndTV, 0);

			if ( wasFocusSelected )
			{
				// restore the selection state
				selectItem(hwndTV, htFocus);
			}
		}
		//else: nothing to do, no focus already
	}
}

bool ImportsHandling::invalidateFunction( HTREEITEM selectedTreeNode )
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	TV_ITEM tvi = {0};
	

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
	TV_ITEM tvi = {0};
	//HWND treeControl = GetDlgItem(hWndMainDlg, IDC_TREE_IMPORTS);

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

	tvi.pszText = stringBuffer;
	tvi.cchTextMax = 260;
	tvi.hItem = importThunk->hTreeItem;
	tvi.mask = TVIF_TEXT;
	TreeImports.SetItem(&tvi); //TreeView_SetItem(treeControl,&tvi);
}

void ImportsHandling::updateModuleInTreeView(ImportModuleThunk * importThunk)
{
	TV_ITEM tvi = {0};
	//HWND treeControl = GetDlgItem(hWndMainDlg,IDC_TREE_IMPORTS);

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


	tvi.pszText = stringBuffer;
	tvi.cchTextMax = 260;
	tvi.hItem = importThunk->hTreeItem;
	tvi.mask = TVIF_TEXT;
	TreeImports.SetItem(&tvi); //TreeView_SetItem(treeControl,&tvi);
}

bool ImportsHandling::cutThunk( HTREEITEM selectedTreeNode )
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	TV_ITEM tvi = {0};
	//HWND treeControl = GetDlgItem(hWndMainDlg,IDC_TREE_IMPORTS);

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

				TreeImports.DeleteItem(importThunk->hTreeItem); //TreeView_DeleteItem(treeControl,importThunk->hTreeItem);
				moduleThunk->thunkList.erase(iterator2);

				if (moduleThunk->thunkList.empty())
				{
					TreeImports.DeleteItem(moduleThunk->hTreeItem); //TreeView_DeleteItem(treeControl,moduleThunk->hTreeItem);
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

bool ImportsHandling::deleteTreeNode( HTREEITEM selectedTreeNode )
{
	std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
	std::map<DWORD_PTR, ImportThunk>::iterator iterator2;
	ImportModuleThunk * moduleThunk;
	ImportThunk * importThunk;

	TV_ITEM tvi = {0};
	//HWND treeControl = GetDlgItem(hWndMainDlg,IDC_TREE_IMPORTS);

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		

		if (moduleThunk->hTreeItem == selectedTreeNode)
		{
			TreeImports.DeleteItem(moduleThunk->hTreeItem); //TreeView_DeleteItem(treeControl,moduleThunk->hTreeItem);
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
					TreeImports.DeleteItem(moduleThunk->hTreeItem); //TreeView_DeleteItem(treeControl,moduleThunk->hTreeItem);
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

DWORD_PTR ImportsHandling::getApiAddressByNode( HTREEITEM selectedTreeNode )
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

bool ImportsHandling::findNewModules( std::map<DWORD_PTR, ImportThunk> & thunkList )
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
	wcscpy_s(module.moduleName, MAX_PATH, TEXT("?"));

	moduleListNew.insert(std::pair<DWORD_PTR,ImportModuleThunk>(firstThunk,module));
}

bool ImportsHandling::addNotFoundApiToModuleList(ImportThunk * apiNotFound)
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

	wcscpy_s(import.moduleName, MAX_PATH, TEXT("?"));
	strcpy_s(import.name, MAX_PATH, "?");

	module->thunkList.insert(std::pair<DWORD_PTR,ImportThunk>(import.rva, import));

	return true;
}

bool ImportsHandling::addFunctionToModuleList(ImportThunk * apiFound)
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

	//HWND treeControl = GetDlgItem(hWndMainDlg,IDC_TREE_IMPORTS);

	iterator1 = moduleList.begin();

	while (iterator1 != moduleList.end())
	{
		moduleThunk = &(iterator1->second);

		TreeImports.Expand(moduleThunk->hTreeItem, flag); //TreeView_Expand(treeControl, moduleThunk->hTreeItem, flag);

		iterator1++;
	}
}
