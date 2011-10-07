#include "MainGui.h"

#include <atldlgs.h> // WTL common dialogs

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
#include "WindowDeferrer.h"

extern CAppModule _Module; // o_O

const WCHAR MainGui::filterExe[] = L"Executable (*.exe)\0*.exe\0All files\0*.*\0";
const WCHAR MainGui::filterDll[] = L"Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
const WCHAR MainGui::filterExeDll[] = L"Executable (*.exe)\0*.exe\0Dynamic Link Library (*.dll)\0*.dll\0All files\0*.*\0";
const WCHAR MainGui::filterTxt[] = L"Text file (*.txt)\0*.txt\0All files\0*.*\0";

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

	if(hMenuImports)
	{
		appendPluginListToMenu(hMenuImports.GetSubMenu(0));
	}

	accelerators.LoadAccelerators(IDR_ACCELERATOR_MAIN);
}

BOOL MainGui::PreTranslateMessage(MSG* pMsg)
{
	if(accelerators.TranslateAccelerator(m_hWnd, pMsg))
	{
		return TRUE;
	}
	else if(IsDialogMessage(pMsg))
	{
		return TRUE;
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

	DoDataExchange(); // attach controls

	CMessageLoop* pLoop = _Module.GetMessageLoop();
	pLoop->AddMessageFilter(this);

	EditOEPAddress.LimitText(MAX_HEX_VALUE_EDIT_LENGTH);
	EditIATAddress.LimitText(MAX_HEX_VALUE_EDIT_LENGTH);
	EditIATSize.LimitText(MAX_HEX_VALUE_EDIT_LENGTH);

	appendPluginListToMenu(CMenuHandle(GetMenu()).GetSubMenu(MenuImportsOffsetTrace));

	enableDialogControls(FALSE);

	setIconAndDialogCaption();

	GetWindowRect(&minDlgSize);

	return TRUE;
}

void MainGui::OnDestroy()
{
	PostQuitMessage(0);
}

void MainGui::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize = CPoint(minDlgSize.Size());
}

void MainGui::OnSizing(UINT fwSide, const RECT* pRect)
{
	// Get size difference
	CRect rectOld;
	GetWindowRect(&rectOld);
	CRect rectNew = *pRect;

	sizeOffset = rectNew.Size() - rectOld.Size();
}

void MainGui::OnSize(UINT nType, CSize size)
{
	const WindowDeferrer::Deferrable controls[] =
	{
		{IDC_GROUP_ATTACH,    false, false, true, false},
		{IDC_CBO_PROCESSLIST, false, false, true, false},
		{IDC_BTN_PICKDLL,     true, false, false, false},

		{IDC_GROUP_IMPORTS, false, false, true, true},
		{IDC_TREE_IMPORTS,  false, false, true, true},
		{IDC_BTN_INVALIDIMPORTS, false, true, false, false},
		{IDC_BTN_SUSPECTIMPORTS, false, true, false, false},
		{IDC_BTN_SAVETREE,       true, true, false, false},
		{IDC_BTN_LOADTREE,       true, true, false, false},
		{IDC_BTN_CLEARIMPORTS,   true, true, false, false},

		{IDC_GROUP_IATINFO,     false, true, false, false},
		{IDC_STATIC_OEPADDRESS, false, true, false, false},
		{IDC_STATIC_IATADDRESS, false, true, false, false},
		{IDC_STATIC_IATSIZE,    false, true, false, false},
		{IDC_EDIT_OEPADDRESS,   false, true, false, false},
		{IDC_EDIT_IATADDRESS,   false, true, false, false},
		{IDC_EDIT_IATSIZE,      false, true, false, false},
		{IDC_BTN_IATAUTOSEARCH, false, true, false, false},
		{IDC_BTN_GETIMPORTS,    false, true, false, false},

		{IDC_GROUP_ACTIONS, false, true, false, false},
		{IDC_BTN_AUTOTRACE, false, true, false, false},

		{IDC_GROUP_DUMP,    false, true, false, false},
		{IDC_BTN_DUMP,      false, true, false, false},
		{IDC_BTN_PEREBUILD, false, true, false, false},
		{IDC_BTN_FIXDUMP,   false, true, false, false},

		{IDC_GROUP_LOG, false, true, true, false},
		{IDC_LIST_LOG,  false, true, true, false}
	};

	if(nType == SIZE_RESTORED)
	{
		WindowDeferrer deferrer(m_hWnd, controls, _countof(controls));
		deferrer.defer(sizeOffset.cx, sizeOffset.cy);
		sizeOffset.SetSize(0, 0);
	}
}

void MainGui::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetMsgHandled(FALSE);
}

void MainGui::OnContextMenu(CWindow wnd, CPoint point)
{ 
	// point = -1, -1 for keyboard invoked shortcut!
	switch(wnd.GetDlgCtrlID())
	{
	case IDC_TREE_IMPORTS:
		DisplayContextMenuImports(wnd, point);
		return;
	case IDC_LIST_LOG:
		DisplayContextMenuLog(wnd, point);
		return;
	//default: // wnd == m_hWnd?
	//	DisplayContextMenu(wnd, point); 
	//	return;
	}

	SetMsgHandled(FALSE);
}

void MainGui::OnCommand(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	// Make sure it's a menu
	if(uNotifyCode == 0 && !wndCtl.IsWindow())
	{
		if ((nID >= PLUGIN_MENU_BASE_ID) && (nID <= (int)(PluginLoader::getScyllaPluginList().size() + PluginLoader::getImprecPluginList().size() + PLUGIN_MENU_BASE_ID)))
		{
			pluginActionHandler(nID);
			return;
		}
	}
	SetMsgHandled(FALSE);
}

LRESULT MainGui::OnTreeImportsClick(const NMHDR* pnmh)
{
	SetMsgHandled(FALSE);
	return 0;
}

LRESULT MainGui::OnTreeImportsDoubleClick(const NMHDR* pnmh)
{
	if(TreeImports.GetCount() < 1)
		return 0;

	// Get item under cursor
	CPoint client = GetMessagePos();
	TreeImports.ScreenToClient(&client);
	UINT flags;
	CTreeItem over = TreeImports.HitTest(client, &flags);
	CTreeItem parent;
	if(over)
	{
		if(!(flags & TVHT_ONITEM))
		{
			over = NULL;
		}
		else
		{
			parent = over.GetParent();
		}
	}

	if(!over.IsNull() && !parent.IsNull())
	{
		pickApiActionHandler(over);
	}

	return 0;
}

LRESULT MainGui::OnTreeImportsRightClick(const NMHDR* pnmh)
{
	SetMsgHandled(FALSE);
	return 0;
}

LRESULT MainGui::OnTreeImportsRightDoubleClick(const NMHDR* pnmh)
{
	SetMsgHandled(FALSE);
	return 0;
}

LRESULT MainGui::OnTreeImportsOnKey(const NMHDR* pnmh)
{
	const NMTVKEYDOWN * tkd = (NMTVKEYDOWN *)pnmh;
	switch(tkd->wVKey)
	{
	case VK_RETURN:
		{
			CTreeItem selected = TreeImports.GetSelectedItem();
			if(!selected.IsNull() && !selected.GetParent().IsNull())
			{
				pickApiActionHandler(selected);
			}
		}
		return 1;
	case VK_DELETE:
		{
			CTreeItem selected = TreeImports.GetSelectedItem();
			if(!selected.IsNull())
			{
				if(selected.GetParent().IsNull())
				{
					importsHandling.deleteTreeNode(selected);
				}
				else
				{
					importsHandling.cutThunk(selected);
				}
			}
		}
		return 1;
	}

	SetMsgHandled(FALSE);
	return 0;
}

UINT MainGui::OnTreeImportsSubclassGetDlgCode(const MSG * lpMsg)
{
	//TreeImportsSubclass.ProcessWindowMessage();

	//UINT original = 0;

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
	// TODO
}

void MainGui::OnCutSelected(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	// TODO
}

void MainGui::OnSaveTree(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	// TODO
}

void MainGui::OnLoadTree(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	// TODO
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
		Logger::printfDialog(TEXT("->>> Module %s selected."), processAccessHelp.selectedModule->getFilename());
		Logger::printfDialog(TEXT("Imagebase: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size: %08X"),processAccessHelp.selectedModule->modBaseAddr,processAccessHelp.selectedModule->modBaseSize);
	}
	else
	{
		processAccessHelp.selectedModule = 0;
	}
}

void MainGui::pickApiActionHandler(CTreeItem item)
{
	CTreeItem parent = item.GetParent();
	if(parent.IsNull())
		return;

	// TODO: new node when user picked an API from another DLL?

	PickApiGui dlgPickApi(processAccessHelp.moduleList);
	if(dlgPickApi.DoModal())
	{
		const ApiInfo* api = dlgPickApi.getSelectedApi();
		if(api && api->module)
		{
			std::map<DWORD_PTR, ImportModuleThunk>::iterator iterator1;
			std::map<DWORD_PTR, ImportThunk>::iterator iterator2;

			iterator1 = importsHandling.moduleList.begin();
			while(iterator1 != importsHandling.moduleList.end())
			{
				if(iterator1->second.hTreeItem == parent)
				{
					iterator2 = iterator1->second.thunkList.begin();
					while(iterator2 != iterator1->second.thunkList.end())
					{
						if(iterator2->second.hTreeItem == item)
						{
							ImportThunk &imp = iterator2->second;
							wcscpy_s(imp.moduleName, MAX_PATH, api->module->getFilename());
							strcpy_s(imp.name, MAX_PATH, api->name);
							imp.ordinal = api->ordinal;
							//imp.apiAddressVA = api->va; //??
							imp.hint = api->hint;
							imp.valid = true;
							imp.suspect = api->isForwarded;

							importsHandling.updateImportInTreeView(&imp, item);
							break;
						}
						iterator2++;
					}
					break;
				}
				iterator1++;
			}
		}
	}
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

	swprintf_s(stringBuffer, _countof(stringBuffer),TEXT(PRINTF_DWORD_PTR_FULL),process.entryPoint + process.imageBase);

	EditOEPAddress.SetWindowText(stringBuffer);

	selectedProcess = &process;
	enableDialogControls(TRUE);
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
		int bufsize = 0;
		for(int i = 0; i < ListLog.GetCount(); i++)
		{
			int size = ListLog.GetTextLen(i);
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

			ProcessAccessHelp::writeMemoryToFileEnd(hFile, size * sizeof(WCHAR), buffer);
		}
		delete[] buffer;
		CloseHandle(hFile);
	}
	return success;
}

void MainGui::showInvalidImportsActionHandler()
{
	importsHandling.showImports(true, false);
}

void MainGui::showSuspectImportsActionHandler()
{
	importsHandling.showImports(false, true);
}

void MainGui::iatAutosearchActionHandler()
{
	DWORD_PTR searchAddress = 0;
	DWORD_PTR addressIAT = 0;
	DWORD sizeIAT = 0;
	IATSearch iatSearch;

	if(!selectedProcess)
		return;

	if(EditOEPAddress.GetWindowText(stringBuffer, _countof(stringBuffer)) > 1)
	{
		searchAddress = stringToDwordPtr(stringBuffer);
		if (searchAddress)
		{
			if (iatSearch.searchImportAddressTableInProcess(searchAddress, &addressIAT, &sizeIAT))
			{
				Logger::printfDialog(TEXT("IAT found at VA ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" RVA ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size 0x%04X (%d)"),addressIAT, addressIAT - processAccessHelp.targetImageBase,sizeIAT,sizeIAT);

				swprintf_s(stringBuffer, _countof(stringBuffer),TEXT(PRINTF_DWORD_PTR_FULL),addressIAT);
				EditIATAddress.SetWindowText(stringBuffer);

				swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("%08X"),sizeIAT);
				EditIATSize.SetWindowText(stringBuffer);

				swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("IAT found! Start Address ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size 0x%04X (%d) "),addressIAT,sizeIAT,sizeIAT);
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
	DWORD_PTR addressIAT = 0;
	DWORD sizeIAT = 0;

	if(!selectedProcess)
		return;

	if (EditIATAddress.GetWindowText(stringBuffer, _countof(stringBuffer)) > 0)
	{
		addressIAT = stringToDwordPtr(stringBuffer);
	}

	if (EditIATSize.GetWindowText(stringBuffer, _countof(stringBuffer)) > 0)
	{
		sizeIAT = wcstoul(stringBuffer, NULL, 16);
	}

	if (addressIAT && sizeIAT)
	{
		apiReader.readAndParseIAT(addressIAT, sizeIAT,importsHandling.moduleList);
		importsHandling.displayAllImports();
	}
}

DWORD_PTR MainGui::stringToDwordPtr(const WCHAR * hexString)
{
	DWORD_PTR address = 0;

#ifdef _WIN64
	address = _wcstoui64(hexString, NULL, 16);
#else
	address = wcstoul(hexString, NULL, 16);
#endif

	if (address == 0)
	{
#ifdef DEBUG_COMMENTS
		Logger::debugLog(L"stringToDwordPtr :: address == 0, %s",hexString);
#endif
		return 0;
	}
	else
	{
		return address;
	}
}

void MainGui::SetupImportsMenuItems(bool isItem, bool isThunk)
{
	// assert(!(!isItem && isThunk));

	CMenuHandle hSub = hMenuImports.GetSubMenu(0);

	UINT itemOnly = isItem ? MF_ENABLED : MF_GRAYED;
	UINT thunkOnly = isThunk ? MF_ENABLED : MF_GRAYED;

	hSub.EnableMenuItem(ID__INVALIDATEFUNCTION, thunkOnly);
	hSub.EnableMenuItem(ID__DISASSEMBLE, thunkOnly);
	hSub.EnableMenuItem(ID__CUTTHUNK, thunkOnly);

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
		over = TreeImports.GetSelectedItem();
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
		CPoint client = pt;
		TreeImports.ScreenToClient(&client);
		UINT flags;
		over = TreeImports.HitTest(client, &flags);
		if(over && !(flags & TVHT_ONITEM))
		{
			over = NULL;
		}
	}

	if(over)
	{
		parent = over.GetParent();
	}

	if (hMenuImports)
	{
		// Prepare hmenuImports
		SetupImportsMenuItems(!over.IsNull(), !parent.IsNull());

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
			case ID__INVALIDATEFUNCTION:
				importsHandling.invalidateFunction(over);
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
				importsHandling.cutThunk(over);
				break;
			case ID__DELETETREENODE:
				importsHandling.deleteTreeNode(parent ? parent : over);
				break;
			}
		}
	}
}

void MainGui::DisplayContextMenuLog(CWindow hwnd, CPoint pt)
{
	if (hMenuLog)
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
			peDump.entryPoint = getOEPFromGui();
			wcscpy_s(peDump.fullpath, MAX_PATH, processAccessHelp.selectedModule->fullPath);
		}
		else
		{
			peDump.imageBase = ProcessAccessHelp::targetImageBase;
			peDump.sizeOfImage = (DWORD)ProcessAccessHelp::targetSizeOfImage;
			//get it from gui
			peDump.entryPoint = getOEPFromGui();
			wcscpy_s(peDump.fullpath, MAX_PATH, selectedProcess->fullPath);
		}

		peDump.useHeaderFromDisk = ConfigurationHolder::getConfigObject(USE_PE_HEADER_FROM_DISK)->isTrue();
		if (peDump.dumpCompleteProcessToDisk(selectedFilePath))
		{
			Logger::printfDialog(TEXT("Dump success %s"),selectedFilePath);
			//MessageBox(L"Image dumped successfully.", L"Success");
		}
		else
		{
			Logger::printfDialog(TEXT("Error: Cannot dump image."));
			MessageBox(L"Cannot dump image.", L"Failure", MB_ICONERROR);
		}
	}
}

DWORD_PTR MainGui::getOEPFromGui()
{
	if (EditOEPAddress.GetWindowText(stringBuffer, _countof(stringBuffer)) > 0)
	{
		return stringToDwordPtr(stringBuffer);
	}
	else
	{
		return 0;
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
			//MessageBox(L"Image rebuilded successfully.", L"Success", MB_ICONINFORMATION);
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
		wcscpy_s(newFilePath,MAX_PATH,selectedFilePath);

		const WCHAR * extension = 0;

		WCHAR* dot = wcsrchr(newFilePath, L'.');
		if (dot)
		{
			*dot = L'\0';
			extension = selectedFilePath + (dot - newFilePath); //wcsrchr(selectedFilePath, L'.');
		}

		wcscat_s(newFilePath, MAX_PATH, L"_SCY");

		if(extension)
		{
			wcscat_s(newFilePath, MAX_PATH, extension);
		}

		if (importRebuild.rebuildImportTable(selectedFilePath,newFilePath,importsHandling.moduleList))
		{
			//MessageBox(L"Imports rebuilding successful", L"Success", MB_ICONINFORMATION);

			Logger::printfDialog(TEXT("Import Rebuild success %s"), newFilePath);
		}
		else
		{
			Logger::printfDialog(TEXT("Import Rebuild failed, target %s"), selectedFilePath);
			MessageBox(L"Imports rebuilding failed", L"Failure", MB_ICONERROR);
		}
	}
}

void MainGui::enableDialogControls(BOOL value)
{
	GetDlgItem(IDC_BTN_PICKDLL).EnableWindow(value);
	GetDlgItem(IDC_BTN_DUMP).EnableWindow(value);
	GetDlgItem(IDC_BTN_FIXDUMP).EnableWindow(value);
	GetDlgItem(IDC_BTN_IATAUTOSEARCH).EnableWindow(value);
	GetDlgItem(IDC_BTN_GETIMPORTS).EnableWindow(value);
	GetDlgItem(IDC_BTN_SUSPECTIMPORTS).EnableWindow(value);
	GetDlgItem(IDC_BTN_INVALIDIMPORTS).EnableWindow(value);
	GetDlgItem(IDC_BTN_CLEARIMPORTS).EnableWindow(value);

	CMenuHandle menu = GetMenu();

	menu.EnableMenuItem(ID_FILE_DUMP, value ? MF_ENABLED : MF_GRAYED);
	menu.EnableMenuItem(ID_FILE_FIXDUMP, value ? MF_ENABLED : MF_GRAYED);
	menu.EnableMenuItem(ID_MISC_DLLINJECTION, value ? MF_ENABLED : MF_GRAYED);
	menu.GetSubMenu(MenuImportsOffsetTrace).EnableMenuItem(MenuImportsTraceOffsetScylla, MF_BYPOSITION | (value ? MF_ENABLED : MF_GRAYED));
	menu.GetSubMenu(MenuImportsOffsetTrace).EnableMenuItem(MenuImportsTraceOffsetImpRec, MF_BYPOSITION | (value ? MF_ENABLED : MF_GRAYED));

	//not yet implemented
	GetDlgItem(IDC_BTN_AUTOTRACE).EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_SAVETREE).EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_LOADTREE).EnableWindow(FALSE);

	menu.EnableMenuItem(ID_IMPORTS_INVALIDATESELECTED, MF_GRAYED);
	menu.EnableMenuItem(ID_IMPORTS_CUTSELECTED, MF_GRAYED);
	menu.EnableMenuItem(ID_IMPORTS_SAVETREE, MF_GRAYED);
	menu.EnableMenuItem(ID_IMPORTS_SAVETREE, MF_GRAYED);
	menu.EnableMenuItem(ID_IMPORTS_LOADTREE, MF_GRAYED);
	menu.EnableMenuItem(ID_TRACE_AUTOTRACE, MF_GRAYED);
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
	TreeImports.DeleteAllItems();
	importsHandling.moduleList.clear();
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
}
