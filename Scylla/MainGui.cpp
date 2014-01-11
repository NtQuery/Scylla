#include "MainGui.h"

#include "Architecture.h"
//#include "PluginLoader.h"
//#include "ConfigurationHolder.h"
#include "PeParser.h"
#include "DllInjectionPlugin.h"
#include "DisassemblerGui.h"
#include "PickApiGui.h"
//#include "NativeWinApi.h"
#include "ImportRebuilder.h"
#include "SystemInformation.h"
#include "Scylla.h"
#include "AboutGui.h"
#include "DonateGui.h"
#include "OptionsGui.h"
#include "TreeImportExport.h"

extern CAppModule _Module; // o_O

const WCHAR MainGui::filterExe[]    = L"Executable (*.exe)\0*.exe\0All files\0*.*\0";
const WCHAR MainGui::filterDll[]    = L"Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
const WCHAR MainGui::filterExeDll[] = L"Executable (*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
const WCHAR MainGui::filterTxt[]    = L"Text file (*.txt)\0*.txt\0All files\0*.*\0";
const WCHAR MainGui::filterXml[]    = L"XML file (*.xml)\0*.xml\0All files\0*.*\0";
const WCHAR MainGui::filterMem[]    = L"MEM file (*.mem)\0*.mem\0All files\0*.*\0";

MainGui::MainGui() : selectedProcess(0), isProcessSuspended(false), importsHandling(TreeImports), TreeImportsSubclass(this, IDC_TREE_IMPORTS)
{
	/*
	Logger::getDebugLogFilePath();
	ConfigurationHolder::loadConfiguration();
	PluginLoader::findAllPlugins();
	NativeWinApi::initialize();
	SystemInformation::getSystemInformation();

	if(ConfigurationHolder::getConfigObject(DEBUG_PRIVILEGE)->isTrue())
	{
		processLister.setDebugPrivileges();
	}
	

	ProcessAccessHelp::getProcessModules(GetCurrentProcessId(), ProcessAccessHelp::ownModuleList);
	*/

	hIcon.LoadIcon(IDI_ICON_SCYLLA);
	hMenuImports.LoadMenu(IDR_MENU_IMPORTS);
	hMenuLog.LoadMenu(IDR_MENU_LOG);
	accelerators.LoadAccelerators(IDR_ACCELERATOR_MAIN);

	hIconCheck.LoadIcon(IDI_ICON_CHECK, 16, 16);
	hIconWarning.LoadIcon(IDI_ICON_WARNING, 16, 16);
	hIconError.LoadIcon(IDI_ICON_ERROR, 16, 16);
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

void MainGui::InitDllStartWithPreSelect( PGUI_DLL_PARAMETER guiParam )
{
	ComboProcessList.ResetContent();
	std::vector<Process>& processList = Scylla::processLister.getProcessListSnapshot();
	int newSel = -1;
	for (size_t i = 0; i < processList.size(); i++)
	{
		if (processList[i].PID == guiParam->dwProcessId)
			newSel = (int)i;
		swprintf_s(stringBuffer, L"0x%04X - %s - %s", processList[i].PID, processList[i].filename, processList[i].fullPath);
		ComboProcessList.AddString(stringBuffer);
	}
	if (newSel != -1)
	{
		ComboProcessList.SetCurSel(newSel);
		processSelectedActionHandler(newSel);

		if (guiParam->mod) //init mod
		{
			//select DLL
			size_t len = ProcessAccessHelp::moduleList.size();
			newSel = -1;
			for (size_t i = 0; i < len; i++)
			{
				if (ProcessAccessHelp::moduleList.at(i).modBaseAddr == (DWORD_PTR)guiParam->mod)
				{
					newSel = (int)i;
					break;
				}
			}
			if (newSel != -1)
			{
				//get selected module
				ProcessAccessHelp::selectedModule = &ProcessAccessHelp::moduleList.at(newSel);

				ProcessAccessHelp::targetImageBase = ProcessAccessHelp::selectedModule->modBaseAddr;

				DWORD modEntryPoint = ProcessAccessHelp::getEntryPointFromFile(ProcessAccessHelp::selectedModule->fullPath);

				EditOEPAddress.SetValue(modEntryPoint + ProcessAccessHelp::targetImageBase);

				Scylla::windowLog.log(L"->>> Module %s selected.", ProcessAccessHelp::selectedModule->getFilename());
				Scylla::windowLog.log(L"Imagebase: " PRINTF_DWORD_PTR_FULL L" Size: %08X EntryPoint: %08X", ProcessAccessHelp::selectedModule->modBaseAddr, ProcessAccessHelp::selectedModule->modBaseSize, modEntryPoint);
			}
		}
	}
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

	Scylla::windowLog.setWindow(ListLog);

	appendPluginListToMenu(hMenuImports.GetSubMenu(0));
	appendPluginListToMenu(CMenuHandle(GetMenu()).GetSubMenu(MenuImportsOffsetTrace));

	enableDialogControls(FALSE);
	setIconAndDialogCaption();

	if (lInitParam)
	{
		InitDllStartWithPreSelect((PGUI_DLL_PARAMETER)lInitParam);
	}
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
		if ((nID >= PLUGIN_MENU_BASE_ID) && (nID <= (int)(Scylla::plugins.getScyllaPluginList().size() + Scylla::plugins.getImprecPluginList().size() + PLUGIN_MENU_BASE_ID)))
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

void MainGui::OnDumpMemory(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	dumpMemoryActionHandler();
}

void MainGui::OnDumpSection(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	dumpSectionActionHandler();
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
void MainGui::OnDisassembler(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	disassemblerActionHandler();
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
	if (isProcessSuspended)
	{
		int msgboxID = MessageBox(L"Process is suspended. Do you want to terminate the process?\r\n\r\nYES = Terminate Process\r\nNO = Try to resume the process\r\nCancel = Do nothing", L"Information", MB_YESNOCANCEL|MB_ICONINFORMATION);
		
		switch (msgboxID)
		{
		case IDYES:
			ProcessAccessHelp::terminateProcess();
			break;
		case IDNO:
			ProcessAccessHelp::resumeProcess();
			break;
		default:
			break;
		}
	}

	DestroyWindow();
}

void MainGui::OnAbout(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	showAboutDialog();
}

void MainGui::OnDonate(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	showDonateDialog();
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
	swprintf_s(stringBuffer, L"\tImports: %u", totalImports);
	StatusBar.SetText(PART_COUNT, stringBuffer);

	if(invalidImports > 0)
	{
		StatusBar.SetIcon(PART_INVALID, hIconError);
	}
	else
	{
		StatusBar.SetIcon(PART_INVALID, hIconCheck);
	}

	swprintf_s(stringBuffer, L"\tInvalid: %u", invalidImports);
	StatusBar.SetText(PART_INVALID, stringBuffer);

	if(selectedProcess)
	{
		DWORD_PTR imageBase = 0;
		const WCHAR * fileName = 0;

		if(ProcessAccessHelp::selectedModule)
		{
			imageBase = ProcessAccessHelp::selectedModule->modBaseAddr;
			fileName = ProcessAccessHelp::selectedModule->getFilename();
		}
		else
		{
			imageBase = selectedProcess->imageBase;
			fileName = selectedProcess->filename;
		}

		swprintf_s(stringBuffer, L"\tImagebase: " PRINTF_DWORD_PTR_FULL, imageBase);
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
		selectedFile[0] = L'\0';
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

	SetWindowText(APPNAME L" " ARCHITECTURE L" " APPVERSION);
}

void MainGui::pickDllActionHandler()
{
	if(!selectedProcess)
		return;

	PickDllGui dlgPickDll(ProcessAccessHelp::moduleList);
	if(dlgPickDll.DoModal())
	{
		//get selected module
		ProcessAccessHelp::selectedModule = dlgPickDll.getSelectedModule();

		ProcessAccessHelp::targetImageBase = ProcessAccessHelp::selectedModule->modBaseAddr;

		DWORD modEntryPoint = ProcessAccessHelp::getEntryPointFromFile(ProcessAccessHelp::selectedModule->fullPath);

		EditOEPAddress.SetValue(modEntryPoint + ProcessAccessHelp::targetImageBase);

		Scylla::windowLog.log(L"->>> Module %s selected.", ProcessAccessHelp::selectedModule->getFilename());
		Scylla::windowLog.log(L"Imagebase: " PRINTF_DWORD_PTR_FULL L" Size: %08X EntryPoint: %08X", ProcessAccessHelp::selectedModule->modBaseAddr, ProcessAccessHelp::selectedModule->modBaseSize, modEntryPoint);
	}
	else
	{
		ProcessAccessHelp::selectedModule = 0;
	}

	updateStatusBar();
}

void MainGui::pickApiActionHandler(CTreeItem item)
{
	if(!importsHandling.isImport(item))
		return;

	// TODO: new node when user picked an API from another DLL?

	PickApiGui dlgPickApi(ProcessAccessHelp::moduleList);
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
			swprintf_s(stringBuffer, L"Can't read memory at " PRINTF_DWORD_PTR_FULL, address);
			MessageBox(stringBuffer, L"Failure", MB_ICONERROR);
		}
		else
		{
			DisassemblerGui dlgDisassembler(address, &apiReader);
			dlgDisassembler.DoModal();
		}
	}
}

void MainGui::processSelectedActionHandler(int index)
{
	std::vector<Process>& processList = Scylla::processLister.getProcessList();
	Process &process = processList.at(index);
	selectedProcess = 0;

	clearImportsActionHandler();

	Scylla::windowLog.log(L"Analyzing %s", process.fullPath);

	if (ProcessAccessHelp::hProcess != 0)
	{
		ProcessAccessHelp::closeProcessHandle();
		apiReader.clearAll();
	}

	if (!ProcessAccessHelp::openProcessHandle(process.PID))
	{
		enableDialogControls(FALSE);
		Scylla::windowLog.log(L"Error: Cannot open process handle.");
		updateStatusBar();
		return;
	}

	ProcessAccessHelp::getProcessModules(process.PID, ProcessAccessHelp::moduleList);

	apiReader.readApisFromModuleList();

	Scylla::windowLog.log(L"Loading modules done.");

	//TODO improve
	ProcessAccessHelp::selectedModule = 0;
	ProcessAccessHelp::targetSizeOfImage = process.imageSize;
	ProcessAccessHelp::targetImageBase = process.imageBase;

	ProcessAccessHelp::getSizeOfImageCurrentProcess();

	process.imageSize = (DWORD)ProcessAccessHelp::targetSizeOfImage;


	Scylla::windowLog.log(L"Imagebase: " PRINTF_DWORD_PTR_FULL L" Size: %08X", process.imageBase, process.imageSize);

	process.entryPoint = ProcessAccessHelp::getEntryPointFromFile(process.fullPath);

	EditOEPAddress.SetValue(process.entryPoint + process.imageBase);

	selectedProcess = &process;
	enableDialogControls(TRUE);

	updateStatusBar();
}

void MainGui::fillProcessListComboBox(CComboBox& hCombo)
{
	hCombo.ResetContent();

	std::vector<Process>& processList = Scylla::processLister.getProcessListSnapshot();

	for (size_t i = 0; i < processList.size(); i++)
	{
		swprintf_s(stringBuffer, L"0x%04X - %s - %s", processList[i].PID, processList[i].filename, processList[i].fullPath);
		hCombo.AddString(stringBuffer);
	}
}

/*
void MainGui::addTextToOutputLog(const WCHAR * text)
{
	if (m_hWnd)
	{
		ListLog.SetCurSel(ListLog.AddString(text));
	}
}
*/

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
	getCurrentModulePath(stringBuffer, _countof(stringBuffer));
	if(showFileDialog(selectedFilePath, false, NULL, filterXml, NULL, stringBuffer))
	{
		TreeImportExport treeIO(selectedFilePath);
		DWORD_PTR addrOEP = 0;
		DWORD_PTR addrIAT = 0;
		DWORD sizeIAT = 0;

		if(!treeIO.importTreeList(importsHandling.moduleList, &addrOEP, &addrIAT, &sizeIAT))
		{
			Scylla::windowLog.log(L"Loading tree file failed %s", selectedFilePath);
			MessageBox(L"Loading tree file failed.", L"Failure", MB_ICONERROR);
		}
		else
		{
			EditOEPAddress.SetValue(addrOEP);
			EditIATAddress.SetValue(addrIAT);
			EditIATSize.SetValue(sizeIAT);

			importsHandling.displayAllImports();
			updateStatusBar();

			Scylla::windowLog.log(L"Loaded tree file %s", selectedFilePath);
			Scylla::windowLog.log(L"-> OEP: " PRINTF_DWORD_PTR_FULL, addrOEP);
			Scylla::windowLog.log(L"-> IAT: " PRINTF_DWORD_PTR_FULL L" Size: " PRINTF_DWORD_PTR, addrIAT, sizeIAT);
		}
	}
}

void MainGui::saveTreeActionHandler()
{
	if(!selectedProcess)
		return;

	WCHAR selectedFilePath[MAX_PATH];
	getCurrentModulePath(stringBuffer, _countof(stringBuffer));
	if(showFileDialog(selectedFilePath, true, NULL, filterXml, L"xml", stringBuffer))
	{
		TreeImportExport treeIO(selectedFilePath);
		DWORD_PTR addrOEP = EditOEPAddress.GetValue();
		DWORD_PTR addrIAT = EditIATAddress.GetValue();
		DWORD sizeIAT = EditIATSize.GetValue();

		if(!treeIO.exportTreeList(importsHandling.moduleList, selectedProcess, addrOEP, addrIAT, sizeIAT))
		{
			Scylla::windowLog.log(L"Saving tree file failed %s", selectedFilePath);
			MessageBox(L"Saving tree file failed.", L"Failure", MB_ICONERROR);
		}
		else
		{
			Scylla::windowLog.log(L"Saved tree file %s", selectedFilePath);
		}
	}
}

void MainGui::iatAutosearchActionHandler()
{
	DWORD_PTR searchAddress = 0;
	DWORD_PTR addressIAT = 0, addressIATAdv = 0;
	DWORD sizeIAT = 0, sizeIATAdv = 0;
	IATSearch iatSearch;

	if(!selectedProcess)
		return;

	if(EditOEPAddress.GetWindowTextLength() > 0)
	{
		searchAddress = EditOEPAddress.GetValue();
		if (searchAddress)
		{

			if (Scylla::config[USE_ADVANCED_IAT_SEARCH].isTrue())
			{
				if (iatSearch.searchImportAddressTableInProcess(searchAddress, &addressIATAdv, &sizeIATAdv, true))
				{
					Scylla::windowLog.log(L"IAT Search Advanced: IAT VA " PRINTF_DWORD_PTR_FULL L" RVA " PRINTF_DWORD_PTR_FULL L" Size 0x%04X (%d)", addressIATAdv, addressIATAdv - ProcessAccessHelp::targetImageBase, sizeIATAdv, sizeIATAdv);
				}
				else
				{
					Scylla::windowLog.log(L"IAT Search Advanced: IAT not found at OEP " PRINTF_DWORD_PTR_FULL L"!", searchAddress);
				}
			}


			if (iatSearch.searchImportAddressTableInProcess(searchAddress, &addressIAT, &sizeIAT, false))
			{
				Scylla::windowLog.log(L"IAT Search Normal: IAT VA " PRINTF_DWORD_PTR_FULL L" RVA " PRINTF_DWORD_PTR_FULL L" Size 0x%04X (%d)", addressIAT, addressIAT - ProcessAccessHelp::targetImageBase, sizeIAT, sizeIAT);
			}
			else
			{
				Scylla::windowLog.log(L"IAT Search Normal: IAT not found at OEP " PRINTF_DWORD_PTR_FULL L"!", searchAddress);
			}

			if (addressIAT != 0 && addressIATAdv == 0)
			{
				setDialogIATAddressAndSize(addressIAT, sizeIAT);
			}
			else if (addressIAT == 0 && addressIATAdv != 0)
			{
				setDialogIATAddressAndSize(addressIATAdv, sizeIATAdv);
			}
			else if (addressIAT != 0 && addressIATAdv != 0)
			{
				if (addressIATAdv != addressIAT || sizeIAT != sizeIATAdv)
				{
					int msgboxID = MessageBox(L"Result of advanced and normal search is different. Do you want to use the IAT Search Advanced result?", L"Information", MB_YESNO|MB_ICONINFORMATION);
					if (msgboxID == IDYES)
					{
						setDialogIATAddressAndSize(addressIATAdv, sizeIATAdv);
					}
					else
					{
						setDialogIATAddressAndSize(addressIAT, sizeIAT);
					}
				}
				else
				{
					setDialogIATAddressAndSize(addressIAT, sizeIAT);
				}
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
		apiReader.readAndParseIAT(addressIAT, sizeIAT, importsHandling.moduleList);
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
		if ((menuItem >= PLUGIN_MENU_BASE_ID) && (menuItem <= (int)(Scylla::plugins.getScyllaPluginList().size() + Scylla::plugins.getImprecPluginList().size() + PLUGIN_MENU_BASE_ID)))
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
			getCurrentModulePath(stringBuffer, _countof(stringBuffer));
			if(showFileDialog(selectedFilePath, true, NULL, filterTxt, L"txt", stringBuffer))
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
	std::vector<Plugin> &scyllaPluginList = Scylla::plugins.getScyllaPluginList();
	std::vector<Plugin> &imprecPluginList = Scylla::plugins.getImprecPluginList();

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

void MainGui::dumpMemoryActionHandler()
{
	WCHAR selectedFilePath[MAX_PATH];
	DumpMemoryGui dlgDumpMemory;

	if(dlgDumpMemory.DoModal())
	{
		getCurrentModulePath(stringBuffer, _countof(stringBuffer));
		if(showFileDialog(selectedFilePath, true, dlgDumpMemory.dumpFilename, filterMem, L"mem", stringBuffer))
		{
			if (ProcessAccessHelp::writeMemoryToNewFile(selectedFilePath,dlgDumpMemory.dumpedMemorySize,dlgDumpMemory.dumpedMemory))
			{
				Scylla::windowLog.log(L"Memory dump saved %s", selectedFilePath);
			}
			else
			{
				Scylla::windowLog.log(L"Error! Cannot write memory dump to disk");
			}
		}
	}
}

void MainGui::dumpSectionActionHandler()
{
	WCHAR selectedFilePath[MAX_PATH];
	DumpSectionGui dlgDumpSection;
	const WCHAR * fileFilter;
	const WCHAR * defExtension;
	PeParser * peFile = 0;

	dlgDumpSection.entryPoint = EditOEPAddress.GetValue();

	if (ProcessAccessHelp::selectedModule)
	{
		//dump DLL
		fileFilter = filterDll;
		defExtension = L"dll";

		dlgDumpSection.imageBase = ProcessAccessHelp::selectedModule->modBaseAddr;
		//get it from gui
		wcscpy_s(dlgDumpSection.fullpath, ProcessAccessHelp::selectedModule->fullPath);
	}
	else
	{
		fileFilter = filterExe;
		defExtension = L"exe";

		dlgDumpSection.imageBase = ProcessAccessHelp::targetImageBase;
		//get it from gui
		wcscpy_s(dlgDumpSection.fullpath, selectedProcess->fullPath);
	}

	if(dlgDumpSection.DoModal())
	{
		getCurrentModulePath(stringBuffer, _countof(stringBuffer));
		if(showFileDialog(selectedFilePath, true, NULL, fileFilter, defExtension, stringBuffer))
		{
			checkSuspendProcess();

			if (Scylla::config[USE_PE_HEADER_FROM_DISK].isTrue())
			{
				peFile = new PeParser(dlgDumpSection.fullpath, true);
			}
			else
			{
				peFile = new PeParser(dlgDumpSection.imageBase, true);
			}

			std::vector<PeSection> & sectionList = dlgDumpSection.getSectionList();

			if (peFile->dumpProcess(dlgDumpSection.imageBase, dlgDumpSection.entryPoint, selectedFilePath, sectionList))
			{
				Scylla::windowLog.log(L"Dump success %s", selectedFilePath);
			}
			else
			{
				Scylla::windowLog.log(L"Error: Cannot dump image.");
				MessageBox(L"Cannot dump image.", L"Failure", MB_ICONERROR);
			}

			delete peFile;
		}
	}
}

void MainGui::dumpActionHandler()
{
	if(!selectedProcess)
		return;

	WCHAR selectedFilePath[MAX_PATH];
	const WCHAR * fileFilter;
	const WCHAR * defExtension;
	DWORD_PTR modBase = 0;
	DWORD_PTR entrypoint = 0;
	WCHAR * filename = 0;
	PeParser * peFile = 0;

	if (ProcessAccessHelp::selectedModule)
	{
		fileFilter = filterDll;
		defExtension = L"dll";
	}
	else
	{
		fileFilter = filterExe;
		defExtension = L"exe";
	}

	getCurrentModulePath(stringBuffer, _countof(stringBuffer));
	if(showFileDialog(selectedFilePath, true, NULL, fileFilter, defExtension, stringBuffer))
	{
		entrypoint = EditOEPAddress.GetValue();

		checkSuspendProcess();

		if (ProcessAccessHelp::selectedModule)
		{
			//dump DLL
			modBase = ProcessAccessHelp::selectedModule->modBaseAddr;
			filename = ProcessAccessHelp::selectedModule->fullPath;
		}
		else
		{
			//dump exe
			modBase = ProcessAccessHelp::targetImageBase;
			filename = selectedProcess->fullPath;
		}

		if (Scylla::config[USE_PE_HEADER_FROM_DISK].isTrue())
		{
			peFile = new PeParser(filename, true);
		}
		else
		{
			peFile = new PeParser(modBase, true);
		}

		if (peFile->dumpProcess(modBase, entrypoint, selectedFilePath))
		{
			Scylla::windowLog.log(L"Dump success %s", selectedFilePath);
		}
		else
		{
			Scylla::windowLog.log(L"Error: Cannot dump image.");
			MessageBox(L"Cannot dump image.", L"Failure", MB_ICONERROR);
		}

		delete peFile;
	}
}

void MainGui::peRebuildActionHandler()
{
	DWORD newSize = 0;
	WCHAR selectedFilePath[MAX_PATH];

	getCurrentModulePath(stringBuffer, _countof(stringBuffer));
	if(showFileDialog(selectedFilePath, false, NULL, filterExeDll, NULL, stringBuffer))
	{
		if (Scylla::config[CREATE_BACKUP].isTrue())
		{
			if (!ProcessAccessHelp::createBackupFile(selectedFilePath))
			{
				Scylla::windowLog.log(L"Creating backup file failed %s", selectedFilePath);
			}
		}

		DWORD fileSize = (DWORD)ProcessAccessHelp::getFileSize(selectedFilePath);

		PeParser peFile(selectedFilePath, true);

		if (!peFile.isValidPeFile())
		{
			Scylla::windowLog.log(L"This is not a valid PE file %s", selectedFilePath);
			MessageBox(L"Not a valid PE file.", L"Failure", MB_ICONERROR);
			return;
		}

		if (peFile.readPeSectionsFromFile())
		{
			peFile.setDefaultFileAlignment();

			if (Scylla::config[REMOVE_DOS_HEADER_STUB].isTrue())
			{
				peFile.removeDosStub();
			}
			
			peFile.alignAllSectionHeaders();
			peFile.fixPeHeader();

			if (peFile.savePeFileToDisk(selectedFilePath))
			{
				newSize = (DWORD)ProcessAccessHelp::getFileSize(selectedFilePath);

				if (Scylla::config[UPDATE_HEADER_CHECKSUM].isTrue())
				{
					Scylla::windowLog.log(L"Generating PE header checksum");
					if (!PeParser::updatePeHeaderChecksum(selectedFilePath, newSize))
					{
						Scylla::windowLog.log(L"Generating PE header checksum FAILED!");
					}
				}

				Scylla::windowLog.log(L"Rebuild success %s", selectedFilePath);
				Scylla::windowLog.log(L"-> Old file size 0x%08X new file size 0x%08X (%d %%)", fileSize, newSize, ((newSize * 100) / fileSize) );
			}
			else
			{
				Scylla::windowLog.log(L"Rebuild failed, cannot save file %s", selectedFilePath);
				MessageBox(L"Rebuild failed. Cannot save file.", L"Failure", MB_ICONERROR);
			}
		}
		else
		{
			Scylla::windowLog.log(L"Rebuild failed, cannot read file %s", selectedFilePath);
			MessageBox(L"Rebuild failed. Cannot read file.", L"Failure", MB_ICONERROR);
		}

	}
}

void MainGui::dumpFixActionHandler()
{
	if(!selectedProcess)
		return;

	if (TreeImports.GetCount() < 2)
	{
		Scylla::windowLog.log(L"Nothing to rebuild");
		return;
	}

	WCHAR newFilePath[MAX_PATH];
	WCHAR selectedFilePath[MAX_PATH];
	const WCHAR * fileFilter;
	DWORD_PTR modBase = 0;
	DWORD_PTR entrypoint = EditOEPAddress.GetValue();

	if (ProcessAccessHelp::selectedModule)
	{
		modBase = ProcessAccessHelp::selectedModule->modBaseAddr;
		fileFilter = filterDll;
	}
	else
	{
		modBase = ProcessAccessHelp::targetImageBase;
		fileFilter = filterExe;
	}

	getCurrentModulePath(stringBuffer, _countof(stringBuffer));
	if (showFileDialog(selectedFilePath, false, NULL, fileFilter, NULL, stringBuffer))
	{
		wcscpy_s(newFilePath, selectedFilePath);

		const WCHAR * extension = 0;

		WCHAR* dot = wcsrchr(newFilePath, L'.');
		if (dot)
		{
			*dot = L'\0';
			extension = selectedFilePath + (dot - newFilePath); //wcsrchr(selectedFilePath, L'.');
		}

		wcscat_s(newFilePath, L"_SCY");

		if(extension)
		{
			wcscat_s(newFilePath, extension);
		}

		ImportRebuilder importRebuild(selectedFilePath);

		if (Scylla::config[IAT_FIX_AND_OEP_FIX].isTrue())
		{
			importRebuild.setEntryPointRva((DWORD)(entrypoint - modBase));
		}

		if (Scylla::config[OriginalFirstThunk_SUPPORT].isTrue())
		{
			importRebuild.enableOFTSupport();
		}

		if (importRebuild.rebuildImportTable(newFilePath, importsHandling.moduleList))
		{
			Scylla::windowLog.log(L"Import Rebuild success %s", newFilePath);
		}
		else
		{
			Scylla::windowLog.log(L"Import Rebuild failed %s", selectedFilePath);
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
	menu.EnableMenuItem(ID_FILE_DUMPMEMORY, valMenu);
	menu.EnableMenuItem(ID_FILE_DUMPSECTION, valMenu);
	menu.EnableMenuItem(ID_FILE_FIXDUMP, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_INVALIDATESELECTED, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_CUTSELECTED, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_SAVETREE, valMenu);
	menu.EnableMenuItem(ID_IMPORTS_LOADTREE, valMenu);
	menu.EnableMenuItem(ID_MISC_DLLINJECTION, valMenu);
	menu.EnableMenuItem(ID_MISC_DISASSEMBLER, valMenu);
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

void MainGui::showDonateDialog()
{
	DonateGui dlgDonate;
	dlgDonate.DoModal();
}

void MainGui::dllInjectActionHandler()
{
	if(!selectedProcess)
		return;

	WCHAR selectedFilePath[MAX_PATH];
	HMODULE hMod = 0;
	DllInjection dllInjection;

	getCurrentModulePath(stringBuffer, _countof(stringBuffer));
	if (showFileDialog(selectedFilePath, false, NULL, filterDll, NULL, stringBuffer))
	{
		hMod = dllInjection.dllInjection(ProcessAccessHelp::hProcess, selectedFilePath);
		if (hMod && Scylla::config[DLL_INJECTION_AUTO_UNLOAD].isTrue())
		{
			if (!dllInjection.unloadDllInProcess(ProcessAccessHelp::hProcess, hMod))
			{
				Scylla::windowLog.log(L"DLL unloading failed, target %s", selectedFilePath);
			}
		}

		if (hMod)
		{
			Scylla::windowLog.log(L"DLL Injection was successful, target %s", selectedFilePath);
		}
		else
		{
			Scylla::windowLog.log(L"DLL Injection failed, target %s", selectedFilePath);
		}
	}
}

void MainGui::disassemblerActionHandler()
{
	DWORD_PTR oep = EditOEPAddress.GetValue();
	DisassemblerGui disGuiDlg(oep, &apiReader);
	disGuiDlg.DoModal();
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

	std::vector<Plugin> &scyllaPluginList = Scylla::plugins.getScyllaPluginList();
	std::vector<Plugin> &imprecPluginList = Scylla::plugins.getImprecPluginList();

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

bool MainGui::getCurrentModulePath(WCHAR * buffer, size_t bufferSize)
{
	if(!selectedProcess)
		return false;

	if(ProcessAccessHelp::selectedModule)
	{
		wcscpy_s(buffer, bufferSize, ProcessAccessHelp::selectedModule->fullPath);
	}
	else
	{
		wcscpy_s(buffer, bufferSize, selectedProcess->fullPath);
	}

	WCHAR * slash = wcsrchr(buffer, L'\\');
	if(slash)
	{
		*(slash+1) = L'\0';
	}

	return true;
}

void MainGui::checkSuspendProcess()
{
	if (Scylla::config[SUSPEND_PROCESS_FOR_DUMPING].isTrue())
	{
		if (!ProcessAccessHelp::suspendProcess())
		{
			Scylla::windowLog.log(L"Error: Cannot suspend process.");
		}
		else
		{
			isProcessSuspended = true;
			Scylla::windowLog.log(L"Suspending process successful, please resume manually.");
		}
	}
}

void MainGui::setDialogIATAddressAndSize( DWORD_PTR addressIAT, DWORD sizeIAT )
{
	EditIATAddress.SetValue(addressIAT);
	EditIATSize.SetValue(sizeIAT);

	swprintf_s(stringBuffer, L"IAT found:\r\n\r\nStart: " PRINTF_DWORD_PTR_FULL L"\r\nSize: 0x%04X (%d) ", addressIAT, sizeIAT, sizeIAT);
	MessageBox(stringBuffer, L"IAT found", MB_ICONINFORMATION);
}
