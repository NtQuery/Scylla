#include "MainGui.h"

#include "definitions.h"
#include "PluginLoader.h"
#include "ConfigurationHolder.h"
#include "PeDump.h"
#include "PeRebuild.h"
#include "DllInjectionPlugin.h"
#include "DisassemblerGui.h"
#include "PickApiGui.h"
#include "NativeWinApi.h"
#include "ImportRebuild.h"
#include "SystemInformation.h"
#include "AboutGui.h"
#include "OptionsGui.h"
#include "TreeImportExport.h"

extern CAppModule _Module; // o_O

const WCHAR MainGui::filterExe[] = L"Executable (*.exe)\0*.exe\0All files\0*.*\0";
const WCHAR MainGui::filterDll[] = L"Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
const WCHAR MainGui::filterExeDll[] = L"Executable (*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
const WCHAR MainGui::filterTxt[] = L"Text file (*.txt)\0*.txt\0All files\0*.*\0";
const WCHAR MainGui::filterXml[] = L"XML file (*.xml)\0*.xml\0All files\0*.*\0";

MainGui::MainGui() : selectedProcess(0), importsHandling(TreeImports), TreeImportsSubclass(this, IDC_TREE_IMPORTS)
{
	Logger::getDebugLogFilePath();
	ConfigurationHolder::loadConfiguration();
	PluginLoader::findAllPlugins();
	NativeWinApi::initialize();
	SystemInformation::getSystemInformation();

	if(ConfigurationHolder::getConfigObject(DEBUG_PRIVILEGE)->isTrue())
	{
		processLister.setDebugPrivileges();
	}

	processAccessHelp.getProcessModules(GetCurrentProcessId(), processAccessHelp.ownModuleList);

	hIcon.LoadIcon(IDI_ICON_SCYLLA);
	hMenuImports.LoadMenu(IDR_MENU_IMPORTS);
	hMenuLog.LoadMenu(IDR_MENU_LOG);
	accelerators.LoadAccelerators(IDR_ACCELERATOR_MAIN);

	hIconCheck.LoadIcon(IDI_ICON_CHECK, 16, 16);
	hIconWarning.LoadIcon(IDI_ICON_WARNING, 16, 16);
	hIconError.LoadIcon(IDI_ICON_ERROR, 16, 16);

	appendPluginListToMenu(hMenuImports.GetSubMenu(0));
}

BOOL MainGui::PreTranslateMessage(MSG* pMsg)
{
	if(accelerators.TranslateAccelerator(m_hWnd, pMsg))
	{
		return TRUE; // handled keyboard shortcuts
	}
	else if(IsDialogMessage(pMsg))
	{
		return TRUE; // handled dialog messages
	}

	return FALSE;
}

BOOL MainGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	if (SystemInformation::currenOS == UNKNOWN_OS)
	{
		if(IDCANCEL == MessageBox(L"Operating System is not supported\r\nContinue anyway?", L"Scylla", MB_ICONWARNING | MB_OKCANCEL))
		{
			SendMessage(WM_CLOSE);
			return FALSE;
		}
	}

	// register ourselves to receive PreTranslateMessage
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->AddMessageFilter(this);

	setupStatusBar();

	DoDataExchange(); // attach controls
	DlgResize_Init(true, true); // init CDialogResize

	appendPluginListToMenu(CMenuHandle(GetMenu()).GetSubMenu(MenuImportsOffsetTrace));

	enableDialogControls(FALSE);
	setIconAndDialogCaption();
	return TRUE;
}

void MainGui::OnDestroy()
{
	PostQuitMessage(0);
}

void MainGui::OnSize(UINT nType, CSize size)
{
	StatusBar.SendMessage(WM_SIZE);
	SetMsgHandled(FALSE);
}

void MainGui::OnContextMenu(CWindow wnd, CPoint point)
{ 
	switch(wnd.GetDlgCtrlID())
	{
	case IDC_TREE_IMPORTS:
		DisplayContextMenuImports(wnd, point);
		return;
	case IDC_LIST_LOG:
		DisplayContextMenuLog(wnd, point);
		return;
	}

	SetMsgHandled(FALSE);
}

void MainGui::OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	// Handle plugin trace menu selection
	if(uNotifyCode == 0 && !wndCtl.IsWindow()) // make sure it's a menu
	{
		if ((nID >= PLUGIN_MENU_BASE_ID) && (nID <= (int)(PluginLoader::getScyllaPluginList().size() + PluginLoader::getImprecPluginList().size() + PLUGIN_MENU_BASE_ID)))
		{
			pluginActionHandler(nID);
			return;
		}
	}
	SetMsgHandled(FALSE);
}

LRESULT MainGui::OnTreeImportsDoubleClick(const NMHDR* pnmh)
{
	if(TreeImports.GetCount() < 1)
		return 0;

	// Get item under cursor
	CTreeItem over = findTreeItem(CPoint(GetMessagePos()), true);
	if(over && importsHandling.isImport(over))
	{
		pickApiActionHandler(over);
	}

	return 0;
}

LRESULT MainGui::OnTreeImportsKeyDown(const NMHDR* pnmh)
{
	const NMTVKEYDOWN * tkd = (NMTVKEYDOWN *)pnmh;
	switch(tkd->wVKey)
	{
	case VK_RETURN:
		{
			CTreeItem selected = TreeImports.GetFocusItem();
			if(!selected.IsNull() && importsHandling.isImport(selected))
			{
				pickApiActionHandler(selected);
			}
		}
		return 1;
	case VK_DELETE:
		deleteSelectedImportsActionHandler();
		return 1;
	}

	SetMsgHandled(FALSE);
	return 0;
}

UINT MainGui::OnTreeImportsSubclassGetDlgCode(const MSG * lpMsg)
{
	if(lpMsg)
	{
		switch(lpMsg->wParam)
		{
		case VK_RETURN:
			return DLGC_WANTMESSAGE;
		}
	}

	SetMsgHandled(FALSE);
	return 0;
}

void MainGui::OnTreeImportsSubclassChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch(nChar)
	{
		case VK_RETURN:
			break;
		default:
			SetMsgHandled(FALSE);
			break;
	}
}

void MainGui::OnProcessListDrop(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	fillProcessListComboBox(ComboProcessList);
}

void MainGui::OnProcessListSelected(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	processSelectedActionHandler(ComboProcessList.GetCurSel());
}

void MainGui::OnPickDLL(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	pickDllActionHandler();
}

void MainGui::OnOptions(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	optionsActionHandler();
}

void MainGui::OnDump(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	dumpActionHandler();
}

void MainGui::OnFixDump(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	dumpFixActionHandler();
}

void MainGui::OnPERebuild(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	peRebuildActionHandler();
}

void MainGui::OnDLLInject(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	dllInjectActionHandler();
}

void MainGui::OnIATAutoSearch(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	iatAutosearchActionHandler();
}

void MainGui::OnGetImports(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	getImportsActionHandler();
}

void MainGui::OnInvalidImports(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	showInvalidImportsActionHandler();
}

void MainGui::OnSuspectImports(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	showSuspectImportsActionHandler();
}

void MainGui::OnClearImports(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	clearImportsActionHandler();
}

void MainGui::OnInvalidateSelected(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	invalidateSelectedImportsActionHandler();
}

void MainGui::OnCutSelected(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	deleteSelectedImportsActionHandler();
}

void MainGui::OnSaveTree(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	saveTreeActionHandler();
}

void MainGui::OnLoadTree(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	loadTreeActionHandler();
}

void MainGui::OnAutotrace(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	// TODO
}

void MainGui::OnExit(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	DestroyWindow();
}

void MainGui::OnAbout(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	showAboutDialog();
}

void MainGui::setupStatusBar()
{
	StatusBar.Create(m_hWnd, NULL, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_TOOLTIPS, NULL, IDC_STATUS_BAR);

	CRect rcMain, rcStatus;
	GetClientRect(&rcMain);
	StatusBar.GetWindowRect(&rcStatus);

	const int PARTS = 4;
	int widths[PARTS];

	widths[PART_COUNT]     = rcMain.Width() / 5;
	widths[PART_INVALID]   = widths[PART_COUNT] + rcMain.Width() / 5;
	widths[PART_IMAGEBASE] = widths[PART_INVALID] + rcMain.Width() / 3;
	widths[PART_MODULE]    = -1;

	StatusBar.SetParts(PARTS, widths);

	ResizeClient(rcMain.Width(), rcMain.Height() + rcStatus.Height(), FALSE);
}

void MainGui::updateStatusBar()
{
	// Rewrite ImportsHandling so we get these easily
	unsigned int totalImports = importsHandling.thunkCount();
	unsigned int invalidImports = importsHandling.invalidThunkCount();

	// \t = center, \t\t = right-align
	swprintf_s(stringBuffer, _countof(stringBuffer), TEXT("\tImports: %u"), totalImports);
	StatusBar.SetText(PART_COUNT, stringBuffer);

	if(invalidImports > 0)
	{
		StatusBar.SetIcon(PART_INVALID, hIconError);
	}
	else
	{
		StatusBar.SetIcon(PART_INVALID, hIconCheck);
	}

	swprintf_s(stringBuffer, _countof(stringBuffer), TEXT("\tInvalid: %u"), invalidImports);
	StatusBar.SetText(PART_INVALID, stringBuffer);

	if(selectedProcess)
	{
		DWORD_PTR imageBase = 0;
		const WCHAR * fileName = 0;

		if(processAccessHelp.selectedModule)
		{
			imageBase = processAccessHelp.selectedModule->modBaseAddr;
			fileName = processAccessHelp.selectedModule->getFilename();
		}
		else
		{
			imageBase = selectedProcess->imageBase;
			fileName = selectedProcess->filename;
		}

		swprintf_s(stringBuffer, _countof(stringBuffer), TEXT("\tImagebase: ")TEXT(PRINTF_DWORD_PTR_FULL), imageBase);
		StatusBar.SetText(PART_IMAGEBASE, stringBuffer);
		StatusBar.SetText(PART_MODULE, fileName);
		StatusBar.SetTipText(PART_MODULE, fileName);
	}
	else
	{
		StatusBar.SetText(PART_IMAGEBASE, L"");
		StatusBar.SetText(PART_MODULE, L"");
	}
}

bool MainGui::showFileDialog(WCHAR * selectedFile, bool save, const WCHAR * defFileName, const WCHAR * filter, const WCHAR * defExtension, const WCHAR * directory)
{
OPENFILENAME ofn = {0};

	// WTL doesn't support new explorer styles on Vista and up
	// This is because it uses a custom hook, we could remove it or derive
	// from CFileDialog but this solution is easier and allows more control anyway (e.g. initial dir)

	if(defFileName)
	{
		wcscpy_s(selectedFile, MAX_PATH, defFileName);
	}
	else
	{
		selectedFile[0] = _T('\0');
	}

	ofn.lStructSize     = sizeof(ofn);
	ofn.hwndOwner       = m_hWnd;
	ofn.lpstrFilter     = filter;
	ofn.lpstrDefExt     = defExtension; // only first 3 chars are used, no dots!
	ofn.lpstrFile       = selectedFile;
	ofn.lpstrInitialDir = directory;
	ofn.nMaxFile        = MAX_PATH;
	ofn.Flags           = OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

	/*
	 *OFN_EXPLORER is automatically used, it only has to be specified
	 *if using a custom hook
	 *OFN_LONGNAMES is automatically used by explorer-style dialogs
	 */

	if(save)
		ofn.Flags |= OFN_OVERWRITEPROMPT;
	else
		ofn.Flags |= OFN_FILEMUSTEXIST;

	if(save)
		return 0 != GetSaveFileName(&ofn);
	else
		return 0 != GetOpenFileName(&ofn);
}

void MainGui::setIconAndDialogCaption()
{
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	SetWindowText(TEXT(APPNAME)TEXT(" ")TEXT(ARCHITECTURE)TEXT(" ")TEXT(APPVERSION));
}

void MainGui::pickDllActionHandler()
{
	if(!selectedProcess)
		return;

	PickDllGui dlgPickDll(processAccessHelp.moduleList);
	if(dlgPickDll.DoModal())
	{
		//get selected module
		processAccessHelp.selectedModule = dlgPickDll.getSelectedModule();

		processAccessHelp.targetImageBase = processAccessHelp.selectedModule->modBaseAddr;

		Logger::printfDialog(TEXT("->>> Module %s selected."), processAccessHelp.selectedModule->getFilename());
		Logger::printfDialog(TEXT("Imagebase: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size: %08X"),processAccessHelp.selectedModule->modBaseAddr,processAccessHelp.selectedModule->modBaseSize);
	}
	else
	{
		processAccessHelp.selectedModule = 0;
	}

	updateStatusBar();
}

void MainGui::pickApiActionHandler(CTreeItem item)
{
	if(!importsHandling.isImport(item))
		return;

	// TODO: new node when user picked an API from another DLL?

	PickApiGui dlgPickApi(processAccessHelp.moduleList);
	if(dlgPickApi.DoModal())
	{
		const ApiInfo* api = dlgPickApi.getSelectedApi();
		if(api && api->module)
		{
			importsHandling.setImport(item, api->module->getFilename(), api->name, api->ordinal, api->hint, true, api->isForwarded);
		}
	}

	updateStatusBar();
}

void MainGui::startDisassemblerGui(CTreeItem selectedTreeNode)
{
	if(!selectedProcess)
		return;

	DWORD_PTR address = importsHandling.getApiAddressByNode(selectedTreeNode);
	if (address)
	{
		BYTE test;
		if(!ProcessAccessHelp::readMemoryFromProcess(address, sizeof(test), &test))
		{
			swprintf_s(stringBuffer, _countof(stringBuffer), TEXT("Can't read memory at ")TEXT(PRINTF_DWORD_PTR_FULL),address);
			MessageBox(stringBuffer, L"Failure", MB_ICONERROR);
		}
		else
		{
			DisassemblerGui dlgDisassembler(address);
			dlgDisassembler.DoModal();
		}
	}
}

void MainGui::processSelectedActionHandler(int index)
{
	std::vector<Process>& processList = processLister.getProcessList();
	Process &process = processList.at(index);
	selectedProcess = 0;

	clearImportsActionHandler();

	Logger::printfDialog(TEXT("Analyzing %s"),process.fullPath);

	if (processAccessHelp.hProcess != 0)
	{
		processAccessHelp.closeProcessHandle();
		apiReader.clearAll();
	}

	if (!processAccessHelp.openProcessHandle(process.PID))
	{
		enableDialogControls(FALSE);
		Logger::printfDialog(TEXT("Error: Cannot open process handle."));
		updateStatusBar();
		return;
	}

	processAccessHelp.getProcessModules(process.PID, processAccessHelp.moduleList);

	apiReader.readApisFromModuleList();

	Logger::printfDialog(TEXT("Loading modules done."));

	//TODO improve
	processAccessHelp.selectedModule = 0;
	processAccessHelp.targetSizeOfImage = process.imageSize;
	processAccessHelp.targetImageBase = process.imageBase;

	ProcessAccessHelp::getSizeOfImageCurrentProcess();

	process.imageSize = (DWORD)processAccessHelp.targetSizeOfImage;


	Logger::printfDialog(TEXT("Imagebase: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size: %08X"),process.imageBase, process.imageSize);

	process.entryPoint = ProcessAccessHelp::getEntryPointFromFile(process.fullPath);

	EditOEPAddress.SetValue(process.entryPoint + process.imageBase);

	selectedProcess = &process;
	enableDialogControls(TRUE);

	updateStatusBar();
}

void MainGui::fillProcessListComboBox(CComboBox& hCombo)
{
	hCombo.ResetContent();

	std::vector<Process>& processList = processLister.getProcessListSnapshot();

	for (size_t i = 0; i < processList.size(); i++)
	{
		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("0x%04X - %s - %s"),processList[i].PID,processList[i].filename,processList[i].fullPath);
		hCombo.AddString(stringBuffer);
	}
}

void MainGui::addTextToOutputLog(const WCHAR * text)
{
	if (m_hWnd)
	{
		ListLog.SetCurSel(ListLog.AddString(text));
	}
}

void MainGui::clearOutputLog()
{
	if (m_hWnd)
	{
		ListLog.ResetContent();
	}
}

bool MainGui::saveLogToFile(const WCHAR * file)
{
	const BYTE BOM[] = {0xFF, 0xFE}; // UTF-16 little-endian
	const WCHAR newLine[] = L"\r\n";
	bool success = true;

	HANDLE hFile = CreateFile(file, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
	if(hFile != INVALID_HANDLE_VALUE)
	{
		ProcessAccessHelp::writeMemoryToFileEnd(hFile, sizeof(BOM), BOM);

		WCHAR * buffer = 0;
		size_t bufsize = 0;
		for(int i = 0; i < ListLog.GetCount(); i++)
		{
			size_t size = ListLog.GetTextLen(i);
			size += _countof(newLine)-1;
			if(size+1 > bufsize)
			{
				bufsize = size+1;
				delete[] buffer;
				try
				{
					buffer = new WCHAR[bufsize];
				}
				catch(std::bad_alloc&)
				{
					buffer = 0;
					success = false;
					break;
				}
			}

			ListLog.GetText(i, buffer);
			wcscat_s(buffer, bufsize, newLine);

			ProcessAccessHelp::writeMemoryToFileEnd(hFile, (DWORD)(size * sizeof(WCHAR)), buffer);
		}
		delete[] buffer;
		CloseHandle(hFile);
	}
	return success;
}

void MainGui::showInvalidImportsActionHandler()
{
	importsHandling.selectImports(true, false);
	GotoDlgCtrl(TreeImports);
}

void MainGui::showSuspectImportsActionHandler()
{
	importsHandling.selectImports(false, true);
	GotoDlgCtrl(TreeImports);
}

void MainGui::deleteSelectedImportsActionHandler()
{
	CTreeItem selected = TreeImports.GetFirstSelectedItem();
	while(!selected.IsNull())
	{
		if(importsHandling.isModule(selected))
		{
			importsHandling.cutModule(selected);
		}
		else
		{
			importsHandling.cutImport(selected);
		}
		selected = TreeImports.GetNextSelectedItem(selected);
	}
	updateStatusBar();
}

void MainGui::invalidateSelectedImportsActionHandler()
{
	CTreeItem selected = TreeImports.GetFirstSelectedItem();
	while(!selected.IsNull())
	{
		if(importsHandling.isImport(selected))
		{
			importsHandling.invalidateImport(selected);
		}
		selected = TreeImports.GetNextSelectedItem(selected);
	}
	updateStatusBar();
}

void MainGui::loadTreeActionHandler()
{
	if(!selectedProcess)
		return;

	WCHAR selectedFilePath[MAX_PATH];
	TreeImportExport treeIO;
	DWORD_PTR addrOEP = 0;
	DWORD_PTR addrIAT = 0;
	DWORD sizeIAT = 0;

	if(showFileDialog(selectedFilePath, false, NULL, filterXml))
	{
		if(!treeIO.importTreeList(selectedFilePath, importsHandling.moduleList, &addrOEP, &addrIAT, &sizeIAT))
		{
			Logger::printfDialog(TEXT("Loading tree file failed %s"), selectedFilePath);
			MessageBox(L"Loading tree file failed.", L"Failure", MB_ICONERROR);
		}
		else
		{
			EditOEPAddress.SetValue(addrOEP);
			EditIATAddress.SetValue(addrIAT);
			EditIATSize.SetValue(sizeIAT);

			importsHandling.displayAllImports();
			updateStatusBar();

			Logger::printfDialog(TEXT("Loaded tree file %s"), selectedFilePath);
			Logger::printfDialog(TEXT("-> OEP: ")TEXT(PRINTF_DWORD_PTR_FULL), addrOEP);
			Logger::printfDialog(TEXT("-> IAT: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size: ")TEXT(PRINTF_DWORD_PTR), addrIAT, sizeIAT);
		}
	}
}

void MainGui::saveTreeActionHandler()
{
	if(!selectedProcess)
		return;

	WCHAR selectedFilePath[MAX_PATH];
	TreeImportExport treeIO;
	DWORD_PTR addrOEP;
	DWORD_PTR addrIAT;
	DWORD sizeIAT;

	if(showFileDialog(selectedFilePath, true, NULL, filterXml, L"xml"))
	{
		addrOEP = EditOEPAddress.GetValue();
		addrIAT = EditIATAddress.GetValue();
		sizeIAT = EditIATSize.GetValue();

		if(!treeIO.exportTreeList(selectedFilePath, importsHandling.moduleList, selectedProcess, addrOEP, addrIAT, sizeIAT))
		{
			Logger::printfDialog(TEXT("Saving tree file failed %s"), selectedFilePath);
			MessageBox(L"Saving tree file failed.", L"Failure", MB_ICONERROR);
		}
		else
		{
			Logger::printfDialog(TEXT("Saved tree file %s"), selectedFilePath);
		}
	}
}

void MainGui::iatAutosearchActionHandler()
{
	DWORD_PTR searchAddress = 0;
	DWORD_PTR addressIAT = 0;
	DWORD sizeIAT = 0;
	IATSearch iatSearch;

	if(!selectedProcess)
		return;

	if(EditOEPAddress.GetWindowTextLength() > 0)
	{
		searchAddress = EditOEPAddress.GetValue();
		if (searchAddress)
		{
			if (iatSearch.searchImportAddressTableInProcess(searchAddress, &addressIAT, &sizeIAT))
			{
				Logger::printfDialog(TEXT("IAT found at VA ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" RVA ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size 0x%04X (%d)"),addressIAT, addressIAT - processAccessHelp.targetImageBase,sizeIAT,sizeIAT);

				EditIATAddress.SetValue(addressIAT);
				EditIATSize.SetValue(sizeIAT);

				swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("IAT found:\r\n\r\nStart: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("\r\nSize: 0x%04X (%d) "),addressIAT,sizeIAT,sizeIAT);
				MessageBox(stringBuffer, L"IAT found", MB_ICONINFORMATION);
			}
			else
			{
				Logger::printfDialog(TEXT("IAT not found at OEP ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT("!"),searchAddress);
			}
		}
	}
}

void MainGui::getImportsActionHandler()
{
	if(!selectedProcess)
		return;

	DWORD_PTR addressIAT = EditIATAddress.GetValue();
	DWORD sizeIAT = EditIATSize.GetValue();

	if (addressIAT && sizeIAT)
	{
		apiReader.readAndParseIAT(addressIAT, sizeIAT,importsHandling.moduleList);
		importsHandling.displayAllImports();

		updateStatusBar();
	}
}

void MainGui::SetupImportsMenuItems(CTreeItem item)
{
	bool isItem, isImport = false;
	isItem = !item.IsNull();
	if(isItem)
	{
		isImport = importsHandling.isImport(item);
	}

	CMenuHandle hSub = hMenuImports.GetSubMenu(0);

	UINT itemOnly = isItem ? MF_ENABLED : MF_GRAYED;
	UINT importOnly = isImport ? MF_ENABLED : MF_GRAYED;

	hSub.EnableMenuItem(ID__INVALIDATE, itemOnly);
	hSub.EnableMenuItem(ID__DISASSEMBLE, importOnly);
	hSub.EnableMenuItem(ID__CUTTHUNK, importOnly);

	hSub.EnableMenuItem(ID__DELETETREENODE, itemOnly);
}

void MainGui::DisplayContextMenuImports(CWindow hwnd, CPoint pt)
{
	if(TreeImports.GetCount() < 1)
		return;

	CTreeItem over, parent;

	if(pt.x == -1 && pt.y == -1) // invoked by keyboard
	{
		CRect pos;
		over = TreeImports.GetFocusItem();
		if(over)
		{
			over.EnsureVisible();
			over.GetRect(&pos, TRUE);
			TreeImports.ClientToScreen(&pos);
		}
		else
		{
			TreeImports.GetWindowRect(&pos);
		}
		pt = pos.TopLeft();
	}
	else
	{
		// Get item under cursor
		over = findTreeItem(pt, true);
	}

	SetupImportsMenuItems(over);

	CMenuHandle hSub = hMenuImports.GetSubMenu(0);
	BOOL menuItem = hSub.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd);
	if (menuItem)
	{
		if ((menuItem >= PLUGIN_MENU_BASE_ID) && (menuItem <= (int)(PluginLoader::getScyllaPluginList().size() + PluginLoader::getImprecPluginList().size() + PLUGIN_MENU_BASE_ID)))
		{
			//wsprintf(stringBuffer, L"%d %s\n",menuItem,pluginList[menuItem - PLUGIN_MENU_BASE_ID].pluginName);
			//MessageBox(stringBuffer, L"plugin selection");

			pluginActionHandler(menuItem);
			return;
		}
		switch (menuItem)
		{
		case ID__INVALIDATE:
			if(importsHandling.isModule(over))
				importsHandling.invalidateModule(over);
			else
				importsHandling.invalidateImport(over);
			break;
		case ID__DISASSEMBLE:
			startDisassemblerGui(over);
			break;
		case ID__EXPANDALLNODES:
			importsHandling.expandAllTreeNodes();
			break;
		case ID__COLLAPSEALLNODES:
			importsHandling.collapseAllTreeNodes();
			break;
		case ID__CUTTHUNK:
			importsHandling.cutImport(over);
			break;
		case ID__DELETETREENODE:
			importsHandling.cutModule(importsHandling.isImport(over) ? over.GetParent() : over);
			break;
		}
	}

	updateStatusBar();
}

void MainGui::DisplayContextMenuLog(CWindow hwnd, CPoint pt)
{
	if(pt.x == -1 && pt.y == -1) // invoked by keyboard
	{
		CRect pos;
		ListLog.GetWindowRect(&pos);
		pt = pos.TopLeft();
	}

	CMenuHandle hSub = hMenuLog.GetSubMenu(0);
	BOOL menuItem = hSub.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd);
	if (menuItem)
	{
		switch (menuItem)
		{
		case ID__SAVE:
			WCHAR selectedFilePath[MAX_PATH];
			if(showFileDialog(selectedFilePath, true, NULL, filterTxt, L"txt"))
			{
				saveLogToFile(selectedFilePath);
			}
			break;
		case ID__CLEAR:
			clearOutputLog();
			break;
		}
	}
}

void MainGui::appendPluginListToMenu(CMenuHandle hMenu)
{
	std::vector<Plugin> &scyllaPluginList = PluginLoader::getScyllaPluginList();
	std::vector<Plugin> &imprecPluginList = PluginLoader::getImprecPluginList();

	if (scyllaPluginList.size() > 0)
	{
		CMenuHandle newMenu;
		newMenu.CreatePopupMenu();

		for (size_t i = 0; i < scyllaPluginList.size(); i++)
		{
			newMenu.AppendMenu(MF_STRING, i + PLUGIN_MENU_BASE_ID, scyllaPluginList[i].pluginName);
		}

		hMenu.AppendMenu(MF_MENUBARBREAK);
		hMenu.AppendMenu(MF_POPUP, newMenu, L"Scylla Plugins");
	}

	if (imprecPluginList.size() > 0)
	{
		CMenuHandle newMenu;
		newMenu.CreatePopupMenu();

		for (size_t i = 0; i < imprecPluginList.size(); i++)
		{
			newMenu.AppendMenu(MF_STRING, scyllaPluginList.size() + i + PLUGIN_MENU_BASE_ID, imprecPluginList[i].pluginName);
		}

		hMenu.AppendMenu(MF_MENUBARBREAK);
		hMenu.AppendMenu(MF_POPUP, newMenu, L"ImpREC Plugins");
	}
}

void MainGui::dumpActionHandler()
{
	if(!selectedProcess)
		return;

	WCHAR selectedFilePath[MAX_PATH];
	const WCHAR * fileFilter;
	const WCHAR * defExtension;
	PeDump peDump;

	if (processAccessHelp.selectedModule)
	{
		fileFilter = filterDll;
		defExtension = L"dll";
	}
	else
	{
		fileFilter = filterExe;
		defExtension = L"exe";
	}

	if(showFileDialog(selectedFilePath, true, NULL, fileFilter, defExtension))
	{
		if (processAccessHelp.selectedModule)
		{
			//dump DLL
			
			peDump.imageBase = processAccessHelp.selectedModule->modBaseAddr;
			peDump.sizeOfImage = processAccessHelp.selectedModule->modBaseSize;
			//get it from gui
			peDump.entryPoint = EditOEPAddress.GetValue();
			wcscpy_s(peDump.fullpath, _countof(peDump.fullpath), processAccessHelp.selectedModule->fullPath);
		}
		else
		{
			peDump.imageBase = ProcessAccessHelp::targetImageBase;
			peDump.sizeOfImage = (DWORD)ProcessAccessHelp::targetSizeOfImage;
			//get it from gui
			peDump.entryPoint = EditOEPAddress.GetValue();
			wcscpy_s(peDump.fullpath, _countof(peDump.fullpath), selectedProcess->fullPath);
		}

		peDump.useHeaderFromDisk = ConfigurationHolder::getConfigObject(USE_PE_HEADER_FROM_DISK)->isTrue();
		if (peDump.dumpCompleteProcessToDisk(selectedFilePath))
		{
			Logger::printfDialog(TEXT("Dump success %s"),selectedFilePath);
		}
		else
		{
			Logger::printfDialog(TEXT("Error: Cannot dump image."));
			MessageBox(L"Cannot dump image.", L"Failure", MB_ICONERROR);
		}
	}
}

void MainGui::peRebuildActionHandler()
{
	DWORD newSize = 0;
	WCHAR selectedFilePath[MAX_PATH];
	PeRebuild peRebuild;

	if(showFileDialog(selectedFilePath, false, NULL, filterExeDll))
	{
		if (ConfigurationHolder::getConfigObject(CREATE_BACKUP)->isTrue())
		{
			if (!ProcessAccessHelp::createBackupFile(selectedFilePath))
			{
				Logger::printfDialog(TEXT("Creating backup file failed %s"), selectedFilePath);
			}
		}

		LONGLONG fileSize = ProcessAccessHelp::getFileSize(selectedFilePath);
		LPVOID mapped = peRebuild.createFileMappingViewFull(selectedFilePath);

		newSize = peRebuild.realignPE(mapped, (DWORD)fileSize);
		peRebuild.closeAllMappingHandles();

		if (newSize < 10)
		{
			Logger::printfDialog(TEXT("Rebuild failed %s"), selectedFilePath);
			MessageBox(L"Rebuild failed.", L"Failure", MB_ICONERROR);
		}
		else
		{
			peRebuild.truncateFile(selectedFilePath, newSize);

			Logger::printfDialog(TEXT("Rebuild success %s"), selectedFilePath);
			Logger::printfDialog(TEXT("-> Old file size 0x%08X new file size 0x%08X (%d %%)"), (DWORD)fileSize, newSize, (DWORD)((newSize * 100) / (DWORD)fileSize) );
		}
	}
}

void MainGui::dumpFixActionHandler()
{
	if(!selectedProcess)
		return;

	if (TreeImports.GetCount() < 2)
	{
		Logger::printfDialog(TEXT("Nothing to rebuild"));
		return;
	}

	WCHAR newFilePath[MAX_PATH];
	WCHAR selectedFilePath[MAX_PATH];
	const WCHAR * fileFilter;

	ImportRebuild importRebuild;

	if (processAccessHelp.selectedModule)
	{
		fileFilter = filterDll;
	}
	else
	{
		fileFilter = filterExe;
	}

	if (showFileDialog(selectedFilePath, false, NULL, fileFilter))
	{
		wcscpy_s(newFilePath,_countof(newFilePath),selectedFilePath);

		const WCHAR * extension = 0;

		WCHAR* dot = wcsrchr(newFilePath, L'.');
		if (dot)
		{
			*dot = L'\0';
			extension = selectedFilePath + (dot - newFilePath); //wcsrchr(selectedFilePath, L'.');
		}

		wcscat_s(newFilePath, _countof(newFilePath), L"_SCY");

		if(extension)
		{
			wcscat_s(newFilePath, _countof(newFilePath), extension);
		}

		if (importRebuild.rebuildImportTable(selectedFilePath,newFilePath,importsHandling.moduleList))
		{
			Logger::printfDialog(TEXT("Import Rebuild success %s"), newFilePath);
		}
		else
		{
			Logger::printfDialog(TEXT("Import Rebuild failed %s"), selectedFilePath);
			MessageBox(L"Import Rebuild failed", L"Failure", MB_ICONERROR);
		}
	}
}

void MainGui::enableDialogControls(BOOL value)
{
	BOOL valButton = value ? TRUE : FALSE;

	GetDlgItem(IDC_BTN_PICKDLL).EnableWindow(valButton);
	GetDlgItem(IDC_BTN_DUMP).EnableWindow(valButton);
	GetDlgItem(IDC_BTN_FIXDUMP).EnableWindow(valButton);
	GetDlgItem(IDC_BTN_IATAUTOSEARCH).EnableWindow(valButton);
	GetDlgItem(IDC_BTN_GETIMPORTS).EnableWindow(valButton);
	GetDlgItem(IDC_BTN_SUSPECTIMPORTS).EnableWindow(valButton);
	GetDlgItem(IDC_BTN_INVALIDIMPORTS).EnableWindow(valButton);
	GetDlgItem(IDC_BTN_CLEARIMPORTS).EnableWindow(valButton);

	CMenuHandle menu = GetMenu();

	UINT valMenu = value ? MF_ENABLED : MF_GRAYED;

	menu.EnableMenuItem(ID_FILE_DUMP, valMenu);
	menu.EnableMenuItem(ID_FILE_FIXDUMP, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_INVALIDATESELECTED, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_CUTSELECTED, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_SAVETREE, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_LOADTREE, valMenu);
	menu.EnableMenuItem(ID_MISC_DLLINJECTION, valMenu);
	menu.GetSubMenu(MenuImportsOffsetTrace).EnableMenuItem(MenuImportsTraceOffsetScylla, MF_BYPOSITION | valMenu);
	menu.GetSubMenu(MenuImportsOffsetTrace).EnableMenuItem(MenuImportsTraceOffsetImpRec, MF_BYPOSITION | valMenu);

	//not yet implemented
	GetDlgItem(IDC_BTN_AUTOTRACE).EnableWindow(FALSE);
	menu.EnableMenuItem(ID_TRACE_AUTOTRACE, MF_GRAYED);
}

CTreeItem MainGui::findTreeItem(CPoint pt, bool screenCoordinates)
{
	if(screenCoordinates)
	{
		TreeImports.ScreenToClient(&pt);
	}

	UINT flags;
	CTreeItem over = TreeImports.HitTest(pt, &flags);
	if(over)
	{
		if(!(flags & TVHT_ONITEM))
		{
			over.m_hTreeItem = NULL;
		}
	}

	return over;
}

void MainGui::showAboutDialog()
{
	AboutGui dlgAbout;
	dlgAbout.DoModal();
}

void MainGui::dllInjectActionHandler()
{
	if(!selectedProcess)
		return;

	WCHAR selectedFilePath[MAX_PATH];
	HMODULE hMod = 0;
	DllInjection dllInjection;

	if (showFileDialog(selectedFilePath, false, NULL, filterDll))
	{
		hMod = dllInjection.dllInjection(ProcessAccessHelp::hProcess, selectedFilePath);
		if (hMod && ConfigurationHolder::getConfigObject(DLL_INJECTION_AUTO_UNLOAD)->isTrue())
		{
			if (!dllInjection.unloadDllInProcess(ProcessAccessHelp::hProcess, hMod))
			{
				Logger::printfDialog(TEXT("DLL unloading failed, target %s"), selectedFilePath);
			}
		}

		if (hMod)
		{
			Logger::printfDialog(TEXT("DLL Injection was successful, target %s"), selectedFilePath);
		}
		else
		{
			Logger::printfDialog(TEXT("DLL Injection failed, target %s"), selectedFilePath);
		}
	}
}

void MainGui::optionsActionHandler()
{
	OptionsGui dlgOptions;
	dlgOptions.DoModal();
}

void MainGui::clearImportsActionHandler()
{
	importsHandling.clearAllImports();
	updateStatusBar();
}

void MainGui::pluginActionHandler( int menuItem )
{
	if(!selectedProcess)
		return;

	DllInjectionPlugin dllInjectionPlugin;

	std::vector<Plugin> &scyllaPluginList = PluginLoader::getScyllaPluginList();
	std::vector<Plugin> &imprecPluginList = PluginLoader::getImprecPluginList();

	menuItem -= PLUGIN_MENU_BASE_ID;

	dllInjectionPlugin.hProcess = ProcessAccessHelp::hProcess;
	dllInjectionPlugin.apiReader = &apiReader;

	if (menuItem < (int)scyllaPluginList.size())
	{
		//scylla plugin
		dllInjectionPlugin.injectPlugin(scyllaPluginList[menuItem], importsHandling.moduleList,selectedProcess->imageBase, selectedProcess->imageSize);
	}
	else
	{
#ifndef _WIN64

		menuItem -= (int)scyllaPluginList.size();
		//imprec plugin
		dllInjectionPlugin.injectImprecPlugin(imprecPluginList[menuItem], importsHandling.moduleList,selectedProcess->imageBase, selectedProcess->imageSize);

#endif
	}

	importsHandling.scanAndFixModuleList();
	importsHandling.displayAllImports();
	updateStatusBar();
}
