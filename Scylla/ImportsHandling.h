#pragma once

#include <map>

// WTL
#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h> // CTreeViewCtrl, CTreeItem

class ImportThunk;
class ImportModuleThunk;

class ImportsHandling
{
public:
	std::map<DWORD_PTR, ImportModuleThunk> moduleList;
	std::map<DWORD_PTR, ImportModuleThunk> moduleListNew;

	//bool addFunction(WCHAR * moduleName, char * name, DWORD_PTR va, DWORD_PTR rva, DWORD_PTR ordinal, bool valid, bool suspect);
	//bool addModule(WCHAR * moduleName, DWORD_PTR firstThunk);

	ImportsHandling(CTreeViewCtrl& TreeImports);
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

	WCHAR stringBuffer[600]; // o_O
	WCHAR tempString[300];

	CTreeViewCtrl& TreeImports;
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

	CTreeItem addDllToTreeView(CTreeViewCtrl& idTreeView, const ImportModuleThunk * importThunk);
	CTreeItem addApiToTreeView(CTreeViewCtrl& idTreeView, CTreeItem parentDll, const ImportThunk * importThunk);
	
	bool isItemSelected(const CTreeViewCtrl& hwndTV, CTreeItem hItem);
	void unselectItem(CTreeViewCtrl& hwndTV, CTreeItem htItem);
	bool selectItem(CTreeViewCtrl& hwndTV, CTreeItem hItem, bool select = true);
	void setFocus(CTreeViewCtrl& hwndTV, CTreeItem htItem);
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
