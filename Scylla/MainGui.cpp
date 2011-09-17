#include "MainGui.h"

#include "definitions.h"
#include "PluginLoader.h"
#include "ConfigurationHolder.h"
#include "PeDump.h"
#include "PeRebuild.h"
#include "DllInjectionPlugin.h"
#include "DisassemblerGui.h"
#include "NativeWinApi.h"
#include "ImportRebuild.h"
#include "SystemInformation.h"
#include "AboutGui.h"
#include "OptionsGui.h"

MainGui::MainGui() : selectedProcess(0), importsHandling(TreeImports)
{
	Logger::getDebugLogFilePath();
	ConfigurationHolder::loadConfiguration();
	PluginLoader::findAllPlugins();
	NativeWinApi::initialize();
	SystemInformation::getSystemInformation();
}

BOOL MainGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	if (SystemInformation::currenOS == UNKNOWN_OS)
	{
		MessageBox(L"Operating System is not supported", L"Error Operating System", MB_ICONERROR);
		EndDialog(0);
		return FALSE;
	}

	if(ConfigurationHolder::getConfigObject(DEBUG_PRIVILEGE)->isTrue())
	{
		processLister.setDebugPrivileges();
	}

	processAccessHelp.getProcessModules(GetCurrentProcessId(), processAccessHelp.ownModuleList);

	TreeImports.Attach(GetDlgItem(IDC_TREE_IMPORTS));
	ComboProcessList.Attach(GetDlgItem(IDC_CBO_PROCESSLIST));
	ListLog.Attach(GetDlgItem(IDC_LIST_LOG));

	EditOEPAddress.Attach(GetDlgItem(IDC_EDIT_OEPADDRESS));
	EditIATAddress.Attach(GetDlgItem(IDC_EDIT_IATADDRESS));
	EditIATSize.Attach(GetDlgItem(IDC_EDIT_IATSIZE));

	EditOEPAddress.LimitText(MAX_HEX_VALUE_EDIT_LENGTH);
	EditIATAddress.LimitText(MAX_HEX_VALUE_EDIT_LENGTH);
	EditIATSize.LimitText(MAX_HEX_VALUE_EDIT_LENGTH);

	enableDialogButtons(FALSE);

	setIconAndDialogCaption();

	return TRUE;
}

void MainGui::OnLButtonDown(UINT nFlags, CPoint point)
{

}

void MainGui::OnContextMenu(CWindow wnd, CPoint point)
{ 
	//TV_ITEM tvi;
	//WCHAR ttt[260] = {0};
	//HTREEITEM selectedTreeNode = 0;

	if(wnd.GetDlgCtrlID() == IDC_TREE_IMPORTS)
	{
		if(TreeImports.GetCount()) //module list should not be empty
		{
			/*selectedTreeNode = (HTREEITEM)SendDlgItemMessage(hWndMainDlg,IDC_TREE_IMPORTS,TVM_GETNEXTITEM,TVGN_CARET,(LPARAM)selectedTreeNode);
			tvi.mask=TVIF_TEXT;   // item text attrivute

			tvi.pszText=ttt;     // Text is the pointer to the text 

			tvi.cchTextMax=260;   // size of text to retrieve.

			tvi.hItem=selectedTreeNode;   // the selected item

			SendDlgItemMessage(hWndMainDlg,IDC_TREE_IMPORTS,TVM_GETITEM,TVGN_CARET,(LPARAM)&tvi);
			Logger::printfDialog(L"selected %s",tvi.pszText);*/

			//CPoint pt = GetMessagePos();
			//UINT flags = 0;
			//if(TreeImports.HitTest(pt, &flags))
			//{
				DisplayContextMenuImports(wnd, point);
			//}
		}
		return;
	}

	//if (PtInRect(&rc, pt)) 
	//{ 
	//	ClientToScreen(hwnd, &pt); 
	//	DisplayContextMenu(hwnd, pt); 
	//	return TRUE; 
	//} 
}

LRESULT MainGui::OnTreeImportsClick(const NMHDR* pnmh)
{
	//Logger::printfDialog(L"NM_CLICK");
	return FALSE;
}

LRESULT MainGui::OnTreeImportsDoubleClick(const NMHDR* pnmh)
{
	//Logger::printfDialog(L"NM_DBLCLK");
	return FALSE;
}

LRESULT MainGui::OnTreeImportsRightClick(const NMHDR* pnmh)
{
	//Logger::printfDialog(L"NM_RCLICK");

	/*
	HTREEITEM selectedTreeNode = TreeImports.GetNextItem(NULL, TVGN_DROPHILITE);
	if(selectedTreeNode != NULL)
	{
		TreeImports.Select(selectedTreeNode, TVGN_CARET);
	}
	*/
	return FALSE;
}

LRESULT MainGui::OnTreeImportsRightDoubleClick(const NMHDR* pnmh)
{
	//Logger::printfDialog(L"NM_RDBLCLK");
	return FALSE;
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
	TreeImports.DeleteAllItems();
	importsHandling.moduleList.clear();
}

void MainGui::OnClearLog(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	clearOutputLog();
}

void MainGui::OnExit(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void MainGui::OnAbout(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	showAboutDialog();
}

void MainGui::setIconAndDialogCaption()
{
	CIconHandle hicon; // Resource leak!
	if(hicon.LoadIcon(IDI_ICON_SCYLLA1))
	{
		SetIcon(hicon, TRUE);
		SetIcon(hicon, FALSE);
	}

	SetWindowText(TEXT(APPNAME)TEXT(" ")TEXT(ARCHITECTURE)TEXT(" ")TEXT(APPVERSION));
}

void MainGui::pickDllActionHandler()
{
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

void MainGui::startDisassemblerGui(CTreeItem selectedTreeNode)
{
	DWORD_PTR address = importsHandling.getApiAddressByNode(selectedTreeNode);
	if (address)
	{
		DisassemblerGui dlgDisassembler(address);
		dlgDisassembler.DoModal();
	}
}

void MainGui::processSelectedActionHandler(int index)
{
	std::vector<Process>& processList = processLister.getProcessList();
	Process &process = processList.at(index);
	selectedProcess = &process;

	enableDialogButtons(TRUE);

	Logger::printfDialog(TEXT("Analyzing %s"),process.fullPath);

	if (processAccessHelp.hProcess != 0)
	{
		processAccessHelp.closeProcessHandle();
		apiReader.clearAll();
	}

	if (!processAccessHelp.openProcessHandle(process.PID))
	{
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

	selectedProcess->entryPoint = ProcessAccessHelp::getEntryPointFromFile(selectedProcess->fullPath);

	swprintf_s(stringBuffer, _countof(stringBuffer),TEXT(PRINTF_DWORD_PTR_FULL),selectedProcess->entryPoint + selectedProcess->imageBase);

	EditOEPAddress.SetWindowText(stringBuffer);
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

DWORD_PTR MainGui::stringToDwordPtr(WCHAR * hexString)
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

void MainGui::DisplayContextMenuImports(CWindow hwnd, CPoint pt)
{
	BOOL menuItem = 0;
	CTreeItem selectedTreeNode = 0;
	std::vector<Plugin> &pluginList = PluginLoader::getScyllaPluginList();
	CMenuHandle hmenuTrackPopup = getCorrectSubMenu(IDR_MENU_IMPORTS, 0);

	if (hmenuTrackPopup)
	{
		appendPluginListToMenu(hmenuTrackPopup);

		menuItem = hmenuTrackPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd);
		hmenuTrackPopup.DestroyMenu();
		if (menuItem)
		{
			if ((menuItem >= PLUGIN_MENU_BASE_ID) && (menuItem <= (int)(PluginLoader::getScyllaPluginList().size() + PluginLoader::getImprecPluginList().size() + PLUGIN_MENU_BASE_ID)))
			{
				//wsprintf(stringBuffer, L"%d %s\n",menuItem,pluginList[menuItem - PLUGIN_MENU_BASE_ID].pluginName);
				//MessageBox(stringBuffer, L"plugin selection");

				pluginActionHandler(menuItem);
				return;
			}
			
			selectedTreeNode = TreeImports.GetSelectedItem();

			switch (menuItem)
			{
			case ID__INVALIDATEFUNCTION:
				{
					importsHandling.invalidateFunction(selectedTreeNode);
				}
				
				break;
			case ID__DISASSEMBLE:
				{
					startDisassemblerGui(selectedTreeNode);
				}
				break;
			case ID__CUTTHUNK:
				{
					importsHandling.cutThunk(selectedTreeNode);
				}
				break;
			case ID__DELETETREENODE:
				{
					importsHandling.deleteTreeNode(selectedTreeNode);
				}
				break;
			case ID__EXPANDALLNODES:
				{
					importsHandling.expandAllTreeNodes();
				}
				break;
			case ID__COLLAPSEALLNODES:
				{
					importsHandling.collapseAllTreeNodes();
				}
				break;
			}


		}
	}
}

CMenuHandle MainGui::getCorrectSubMenu(int menuItem, int subMenuItem)
{
	CMenuHandle hmenu; // top-level menu 

	// Load the menu resource. 
	if (!hmenu.LoadMenu(menuItem)) 
		return NULL; 

	return hmenu.GetSubMenu(subMenuItem);
}

void MainGui::DisplayContextMenu(CWindow hwnd, CPoint pt) 
{ 
	CMenu hmenu;            // top-level menu 
	CMenuHandle hmenuTrackPopup;  // shortcut menu 
	int menuItem;			// selected menu item

	// Load the menu resource. 
	if (!hmenu.LoadMenu(IDR_MENU_IMPORTS)) 
		return; 

	// TrackPopupMenu cannot display the menu bar so get 
	// a handle to the first shortcut menu. 

	hmenuTrackPopup = hmenu.GetSubMenu(0);

	// Display the shortcut menu. Track the right mouse 
	// button. 
	if (!hmenuTrackPopup)
	{
		MessageBox(L"hmenuTrackPopup == null", L"hmenuTrackPopup");
	}

	menuItem = hmenuTrackPopup.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, hwnd);

	if (menuItem)
	{
		/*if (menuItem == ID_LISTCONTROL_SHOWEXPORTS)
		{
			MessageBox(L"exports",L"dshhhhh");
		}*/
	}
}

void MainGui::appendPluginListToMenu(CMenuHandle hMenuTrackPopup)
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

		hMenuTrackPopup.AppendMenu(MF_MENUBARBREAK);
		hMenuTrackPopup.AppendMenu(MF_POPUP, newMenu, L"Scylla Plugins");
	}

	if (imprecPluginList.size() > 0)
	{
		CMenuHandle newMenu;
		newMenu.CreatePopupMenu();

		for (size_t i = 0; i < imprecPluginList.size(); i++)
		{
			newMenu.AppendMenu(MF_STRING, scyllaPluginList.size() + i + PLUGIN_MENU_BASE_ID, imprecPluginList[i].pluginName);
		}

		hMenuTrackPopup.AppendMenu(MF_MENUBARBREAK);
		hMenuTrackPopup.AppendMenu(MF_POPUP, newMenu, L"ImpREC Plugins");
	}
}

void MainGui::dumpActionHandler()
{
	WCHAR * targetFile = 0;
	PeDump peDump;

	if (processAccessHelp.selectedModule)
	{
		targetFile = ProcessAccessHelp::selectFileToSave(0, 0);
	}
	else
	{
		targetFile = ProcessAccessHelp::selectFileToSave(0, 1);
	}
	
	
	if (targetFile)
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
		if (peDump.dumpCompleteProcessToDisk(targetFile))
		{
			Logger::printfDialog(TEXT("Dump success %s"),targetFile);
			//MessageBox(L"Image dumped successfully.", L"Success");
		}
		else
		{
			Logger::printfDialog(TEXT("Error: Cannot dump image."));
			MessageBox(L"Cannot dump image.", L"Failure", MB_ICONERROR);
		}

		delete [] targetFile;
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
	WCHAR * targetFile = 0;
	PeRebuild peRebuild;

	targetFile = ProcessAccessHelp::selectFileToSave(OFN_FILEMUSTEXIST, 2);

	if (targetFile)
	{
		if (ConfigurationHolder::getConfigObject(CREATE_BACKUP)->isTrue())
		{
			if (!ProcessAccessHelp::createBackupFile(targetFile))
			{
				Logger::printfDialog(TEXT("Creating backup file failed %s"), targetFile);
			}
		}

		LONGLONG fileSize = ProcessAccessHelp::getFileSize(targetFile);
		LPVOID mapped = peRebuild.createFileMappingViewFull(targetFile);

		newSize = peRebuild.realignPE(mapped, (DWORD)fileSize);
		peRebuild.closeAllMappingHandles();

		if (newSize < 10)
		{
			Logger::printfDialog(TEXT("Rebuild failed %s"), targetFile);
			MessageBox(L"Rebuild failed.", L"Failure", MB_ICONERROR);
		}
		else
		{
			peRebuild.truncateFile(targetFile, newSize);

			Logger::printfDialog(TEXT("Rebuild success %s"), targetFile);
			Logger::printfDialog(TEXT("-> Old file size 0x%08X new file size 0x%08X (%d %%)"), (DWORD)fileSize, newSize, (DWORD)((newSize * 100) / (DWORD)fileSize) );
			//MessageBox(L"Image rebuilded successfully.", L"Success", MB_ICONINFORMATION);
		}

		delete [] targetFile;
	}
}

void MainGui::dumpFixActionHandler()
{
	WCHAR * targetFile = 0;
	WCHAR newFilePath[MAX_PATH];
	ImportRebuild importRebuild;

	if (TreeImports.GetCount() < 2)
	{
		Logger::printfDialog(TEXT("Nothing to rebuild"));
		return;
	}

	if (processAccessHelp.selectedModule)
	{
		targetFile = ProcessAccessHelp::selectFileToSave(OFN_FILEMUSTEXIST, 0);
	}
	else
	{
		targetFile = ProcessAccessHelp::selectFileToSave(OFN_FILEMUSTEXIST, 1);
	}

	if (targetFile)
	{
		wcscpy_s(newFilePath,MAX_PATH,targetFile);

		WCHAR* dot = wcsrchr(newFilePath, L'.');
		if (dot)
		{
			*dot = L'\0';
		}

		if (processAccessHelp.selectedModule)
		{
			wcscat_s(newFilePath,MAX_PATH, L"_SCY.dll");
		}
		else
		{
			wcscat_s(newFilePath,MAX_PATH, L"_SCY.exe");
		}
		

		if (importRebuild.rebuildImportTable(targetFile,newFilePath,importsHandling.moduleList))
		{
			//MessageBox(L"Imports rebuilding successful", L"Success", MB_ICONINFORMATION);

			Logger::printfDialog(TEXT("Import Rebuild success %s"), newFilePath);
		}
		else
		{
			Logger::printfDialog(TEXT("Import Rebuild failed, target %s"), targetFile);
			MessageBox(L"Imports rebuilding failed", L"Failure", MB_ICONERROR);
		}

		delete [] targetFile;
	}

}

void MainGui::enableDialogButtons(BOOL value)
{
	GetDlgItem(IDC_BTN_PICKDLL).EnableWindow(value);
	GetDlgItem(IDC_BTN_DUMP).EnableWindow(value);
	GetDlgItem(IDC_BTN_DLLINJECT).EnableWindow(value);
	GetDlgItem(IDC_BTN_FIXDUMP).EnableWindow(value);
	GetDlgItem(IDC_BTN_IATAUTOSEARCH).EnableWindow(value);
	GetDlgItem(IDC_BTN_GETIMPORTS).EnableWindow(value);
	GetDlgItem(IDC_BTN_SUSPECTIMPORTS).EnableWindow(value);
	GetDlgItem(IDC_BTN_INVALIDIMPORTS).EnableWindow(value);
	GetDlgItem(IDC_BTN_CLEARIMPORTS).EnableWindow(value);

	GetDlgItem(IDC_BTN_OPTIONS).EnableWindow(TRUE);

	//not yet implemented
	GetDlgItem(IDC_BTN_AUTOTRACE).EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_SAVETREE).EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_LOADTREE).EnableWindow(FALSE);
}

void MainGui::showAboutDialog()
{
	AboutGui dlgAbout;
	dlgAbout.DoModal();
}

void MainGui::dllInjectActionHandler()
{
	WCHAR * targetFile = 0;
	HMODULE hMod = 0;
	DllInjection dllInjection;

	targetFile = ProcessAccessHelp::selectFileToSave(OFN_FILEMUSTEXIST, 0);

	if (targetFile)
	{
		hMod = dllInjection.dllInjection(ProcessAccessHelp::hProcess, targetFile);
		if (hMod && ConfigurationHolder::getConfigObject(DLL_INJECTION_AUTO_UNLOAD)->isTrue())
		{
			if (!dllInjection.unloadDllInProcess(ProcessAccessHelp::hProcess, hMod))
			{
				Logger::printfDialog(TEXT("DLL unloading failed, target %s"), targetFile);
			}
		}

		if (hMod)
		{
			Logger::printfDialog(TEXT("DLL Injection was successful, target %s"), targetFile);
		}
		else
		{
			Logger::printfDialog(TEXT("DLL Injection failed, target %s"), targetFile);
		}

		delete [] targetFile;
	}
	
}

void MainGui::optionsActionHandler()
{
	OptionsGui dlgOptions;
	dlgOptions.DoModal();
}

void MainGui::pluginActionHandler( int menuItem )
{
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
