#pragma once

#include <map>
#include <unordered_map>

// WTL
#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h> // CTreeItem

class CMultiSelectTreeViewCtrl;

class ImportThunk;
class ImportModuleThunk;

class ImportsHandling
{
public:
	std::map<DWORD_PTR, ImportModuleThunk> moduleList;
	std::map<DWORD_PTR, ImportModuleThunk> moduleListNew;

	ImportsHandling(CMultiSelectTreeViewCtrl& TreeImports);
	~ImportsHandling();

	unsigned int thunkCount() const { return m_thunkCount; }
	unsigned int invalidThunkCount() const { return m_invalidThunkCount; }
	unsigned int suspectThunkCount() const { return m_suspectThunkCount; }

	bool isModule(CTreeItem item);
	bool isImport(CTreeItem item);

	ImportModuleThunk * getModuleThunk(CTreeItem item);
	ImportThunk * getImportThunk(CTreeItem item);

	void displayAllImports();
	void clearAllImports();
	void selectImports(bool invalid, bool suspect);

	bool invalidateImport(CTreeItem item);
	bool invalidateModule(CTreeItem item);
	bool setImport(CTreeItem item, const WCHAR * moduleName, const CHAR * apiName, WORD ordinal = 0, WORD hint = 0, bool valid = true, bool suspect = false);
	bool cutImport(CTreeItem item);
	bool cutModule(CTreeItem item);
	//bool addImport(const WCHAR * moduleName, const CHAR * name, DWORD_PTR va, DWORD_PTR rva, WORD ordinal = 0, bool valid = true, bool suspect = false);
	//bool addModule(const WCHAR * moduleName, DWORD_PTR firstThunk);

	DWORD_PTR getApiAddressByNode(CTreeItem selectedTreeNode);
	void scanAndFixModuleList();
	void expandAllTreeNodes();
	void collapseAllTreeNodes();

private:
	DWORD numberOfFunctions;

	unsigned int m_thunkCount;
	unsigned int m_invalidThunkCount;
	unsigned int m_suspectThunkCount;

	struct TreeItemData
	{
		bool isModule;
		union
		{
			ImportModuleThunk * module;
			ImportThunk * import;
		};
	};

	std::unordered_map<HTREEITEM, TreeItemData> itemData;

	void setItemData(CTreeItem item, const TreeItemData * data);
	TreeItemData * getItemData(CTreeItem item);

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

	void updateCounts();

	CTreeItem addDllToTreeView(CMultiSelectTreeViewCtrl& idTreeView, ImportModuleThunk * moduleThunk);
	CTreeItem addApiToTreeView(CMultiSelectTreeViewCtrl& idTreeView, CTreeItem parentDll, ImportThunk * importThunk);

	void updateImportInTreeView(const ImportThunk * importThunk, CTreeItem item);
	void updateModuleInTreeView(const ImportModuleThunk * importThunk, CTreeItem item);
	
	//bool isItemSelected(CTreeItem hItem);
	//void unselectItem(CTreeItem htItem);
	//bool selectItem(CTreeItem hItem, bool select = true);
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
