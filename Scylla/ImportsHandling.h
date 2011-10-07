#pragma once

#include <map>

// WTL
#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>

class ImportThunk;
class ImportModuleThunk;

class CMultiSelectTreeViewCtrl;

class ImportsHandling
{
public:
	std::map<DWORD_PTR, ImportModuleThunk> moduleList;
	std::map<DWORD_PTR, ImportModuleThunk> moduleListNew;

	//bool addFunction(WCHAR * moduleName, char * name, DWORD_PTR va, DWORD_PTR rva, DWORD_PTR ordinal, bool valid, bool suspect);
	//bool addModule(WCHAR * moduleName, DWORD_PTR firstThunk);

	ImportsHandling(CMultiSelectTreeViewCtrl& TreeImports);
	~ImportsHandling();

	void displayAllImports();
	void showImports(bool invalid, bool suspect);
	bool invalidateFunction(CTreeItem selectedTreeNode);
	bool cutThunk(CTreeItem selectedTreeNode);
	bool deleteTreeNode(CTreeItem selectedTreeNode);

	void updateImportInTreeView(const ImportThunk * importThunk, CTreeItem item);
	void updateModuleInTreeView(const ImportModuleThunk * importThunk, CTreeItem item);
	DWORD_PTR getApiAddressByNode(CTreeItem selectedTreeNode);
	void scanAndFixModuleList();
	void expandAllTreeNodes();
	void collapseAllTreeNodes();

private:
	DWORD numberOfFunctions;

	WCHAR stringBuffer[600];

	CMultiSelectTreeViewCtrl& TreeImports;
	CImageList TreeIcons;
	CIcon hIconCheck;
	CIcon hIconWarning;
	CIcon hIconError;

	// They have to be added to the image list in that order!
	enum Icon {
		iconCheck = 0,
		iconWarning,
		iconError
	};

	CTreeItem addDllToTreeView(CMultiSelectTreeViewCtrl& idTreeView, const ImportModuleThunk * importThunk);
	CTreeItem addApiToTreeView(CMultiSelectTreeViewCtrl& idTreeView, CTreeItem parentDll, const ImportThunk * importThunk);
	
	bool isItemSelected(CTreeItem hItem);
	void unselectItem(CTreeItem htItem);
	bool selectItem(CTreeItem hItem, bool select = true);
	void setFocus(CMultiSelectTreeViewCtrl& hwndTV, CTreeItem htItem);
	bool findNewModules(std::map<DWORD_PTR, ImportThunk> & thunkList);

	Icon getAppropiateIcon(const ImportThunk * importThunk);
	Icon getAppropiateIcon(bool valid);

	bool addModuleToModuleList(const WCHAR * moduleName, DWORD_PTR firstThunk);
	void addUnknownModuleToModuleList(DWORD_PTR firstThunk);
	bool addNotFoundApiToModuleList(const ImportThunk * apiNotFound);
	bool addFunctionToModuleList(const ImportThunk * apiFound);
	bool isNewModule(const WCHAR * moduleName);

	void changeExpandStateOfTreeNodes(UINT flag);
};
