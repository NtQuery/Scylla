#pragma once

#include "Thunks.h"
#include "MainGui.h"

class ImportsHandling : public MainGui {
public:
	std::map<DWORD_PTR, ImportModuleThunk> moduleList;
	std::map<DWORD_PTR, ImportModuleThunk> moduleListNew;

	//bool addFunction(WCHAR * moduleName, char * name, DWORD_PTR va, DWORD_PTR rva, DWORD_PTR ordinal, bool valid, bool suspect);
	//bool addModule(WCHAR * moduleName, DWORD_PTR firstThunk);

	void displayAllImports();
	void showImports(bool invalid, bool suspect);
	bool invalidateFunction(HTREEITEM selectedTreeNode);
	bool cutThunk( HTREEITEM selectedTreeNode );
	bool deleteTreeNode( HTREEITEM selectedTreeNode );

	void updateImportInTreeView(ImportThunk * importThunk);
	void updateModuleInTreeView(ImportModuleThunk * importThunk);
	DWORD_PTR getApiAddressByNode( HTREEITEM selectedTreeNode );
	void scanAndFixModuleList();
	void expandAllTreeNodes();
	void collapseAllTreeNodes();

private:
	DWORD numberOfFunctions;

	WCHAR tempString[300];

	TV_INSERTSTRUCT tvInsert;
	HTREEITEM m_hItemFirstSel;

	HTREEITEM addDllToTreeView(HWND idTreeView, const WCHAR * dllName, DWORD_PTR firstThunk, size_t numberOfFunctions, bool valid);
	HTREEITEM addApiToTreeView(HWND idTreeView, HTREEITEM parentDll, ImportThunk * importThunk);
	

	bool isItemSelected(HWND hwndTV, HTREEITEM hItem);
	void unselectItem(HWND hwndTV, HTREEITEM htItem);
	bool selectItem(HWND hwndTV, HTREEITEM hItem, bool select = true);
	void setFocus(HWND hwndTV, HTREEITEM htItem);
	bool findNewModules( std::map<DWORD_PTR, ImportThunk> & thunkList );

	bool addModuleToModuleList(const WCHAR * moduleName, DWORD_PTR firstThunk);
	void addUnknownModuleToModuleList(DWORD_PTR firstThunk);
	bool addNotFoundApiToModuleList(ImportThunk * apiNotFound);
	bool addFunctionToModuleList(ImportThunk * apiFound);
	bool isNewModule(const WCHAR * moduleName);

	void changeExpandStateOfTreeNodes(UINT flag);


};