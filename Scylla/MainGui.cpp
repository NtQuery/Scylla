#include "MainGui.h"
#include "ImportsHandling.h"
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

HINSTANCE MainGui::hInstance = 0;
HWND MainGui::hWndParent = 0;
HWND MainGui::hWndMainDlg = 0;
Process * MainGui::selectedProcess = 0;

WCHAR MainGui::stringBuffer[300] = {0};

ProcessLister MainGui::processLister;
ImportsHandling MainGui::importsHandling;
ProcessAccessHelp MainGui::processAccessHelp;
ApiReader MainGui::apiReader;

void MainGui::initDialog(HINSTANCE hInstance)
{
	hInstance = hInstance;
	Logger::getDebugLogFilePath();
	ConfigurationHolder::loadConfiguration();
	PluginLoader::findAllPlugins();
	NativeWinApi::initialize();
	SystemInformation::getSystemInformation();


	if (SystemInformation::currenOS == UNKNOWN_OS)
	{
		MessageBox(0, TEXT("Operating System is not supported"), TEXT("Error Operating System"),MB_OK);
		return;
	}

	
	processAccessHelp.getProcessModules(GetCurrentProcessId(), processAccessHelp.ownModuleList);

	//Register controls, required for Windows XP
	InitCommonControls();
	return;
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DLG_MAIN),hWndParent, (DLGPROC)mainDlgProc);

	//ConfigurationHolder::saveConfiguration();
}

LRESULT CALLBACK MainGui::mainDlgProc(HWND hWndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hWndMainDlg = hWndDlg;
	HTREEITEM selectedTreeNode = 0;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		//init dialog elements
		dialogInitActionHandler();
		return TRUE;

	case WM_NOTIFY:
		switch(LOWORD(wParam))
		{
		case IDC_TREE_IMPORTS:
			{
				if(((LPNMHDR)lParam)->code == NM_CLICK)
				{
					//Logger::printfDialog(L"NM_CLICK");
				}
				if(((LPNMHDR)lParam)->code == NM_DBLCLK)
				{
					//Logger::printfDialog(L"NM_DBLCLK");
				}
				if(((LPNMHDR)lParam)->code == NM_RCLICK)
				{
					//Logger::printfDialog(L"NM_RCLICK");

					selectedTreeNode=(HTREEITEM)SendDlgItemMessage (hWndMainDlg,IDC_TREE_IMPORTS,TVM_GETNEXTITEM,TVGN_DROPHILITE,0);
					if(selectedTreeNode != NULL)
					{
						SendDlgItemMessage(hWndMainDlg,IDC_TREE_IMPORTS,TVM_SELECTITEM,TVGN_CARET,(LPARAM)selectedTreeNode);
					}
				}
				if(((LPNMHDR)lParam)->code == NM_RDBLCLK)
				{
					//Logger::printfDialog(L"NM_RDBLCLK");
				}
			}
			break;
			/*case IDC_MODULELIST:
				LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
				if (pnmv->uChanged & LVIF_STATE)
				{
					if ((pnmv->uNewState & LVIS_SELECTED) && (!(pnmv->uOldState & LVIS_SELECTED)) && (pnmv->hdr.code == LVN_ITEMCHANGED))
					{

						//sprintf(stringBuffer,"%X",i);
						//MessageBox(hWndDlg, text,"Display Notification", MB_OK);
						break;
					}
				}*/
		}
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CBO_PROCESSLIST:
			switch(HIWORD(wParam))
			{
			case CBN_DROPDOWN: //list is about to display
				fillProcessListComboBox(GetDlgItem(hWndDlg, IDC_CBO_PROCESSLIST));
				break;
			case CBN_SELENDOK: //item selected
				processSelectedActionHandler(SendMessage(GetDlgItem(hWndDlg, IDC_CBO_PROCESSLIST),CB_GETCURSEL,0,0));
				break;
			}

			return TRUE;

		case IDC_BTN_PICKDLL:
			pickDllActionHandler();
			return TRUE;
		case IDC_BTN_OPTIONS:
			optionsActionHandler();
			return TRUE;
		case IDC_BTN_DUMP:
			dumpActionHandler();
			return TRUE;
		case IDC_BTN_FIXDUMP:
			dumpFixActionHandler();
			return TRUE;
		case IDC_BTN_PEREBUILD:
			peRebuildActionHandler();
			return TRUE;
		case IDC_BTN_DLLINJECT:
			dllInjectActionHandler();
			return TRUE;
		case ID_MISC_DLLINJECTION:
			dllInjectActionHandler();
			return TRUE;
		case ID_MISC_PREFERENCES:
			optionsActionHandler();
			return TRUE;
		case IDC_BTN_IATAUTOSEARCH:
			iatAutosearchActionHandler();
			return TRUE;
		case IDC_BTN_GETIMPORTS:
			getImportsActionHandler();
			return TRUE;
		case IDC_BTN_INVALIDIMPORTS:
			showInvalidImportsActionHandler();
			return TRUE;
		case IDC_BTN_SUSPECTIMPORTS:
			showSuspectImportsActionHandler();
			return TRUE;
		case IDC_BTN_CLEARIMPORTS:
			TreeView_DeleteAllItems(GetDlgItem(hWndDlg, IDC_TREE_IMPORTS));
			importsHandling.moduleList.clear();
			return TRUE;
		case IDC_BTN_CLEARLOG:
			clearOutputLog();
			return TRUE;
		case IDC_BTN_ABOUT:
			showAboutDialog();
			return TRUE;
		case ID_HELP_ABOUT:
			showAboutDialog();
			return TRUE;
		case IDC_BTN_EXIT:
			PostQuitMessage(0);
			EndDialog(hWndDlg, 0);
			return TRUE;
		case ID_FILE_EXIT:
			PostQuitMessage(0);
			EndDialog(hWndDlg, 0);
			return TRUE;
		case IDCANCEL:
			PostQuitMessage(0);
			EndDialog(hWndDlg, 0);
			return TRUE;
		}
		return TRUE;

	case WM_LBUTTONDOWN:
		//leftButtonDownActionHandler();
		//return TRUE;

	case WM_CONTEXTMENU:
		return OnContextMenu(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	default:
		return FALSE;
	}
}

void MainGui::setIconAndDialogCaption()
{
	if (hWndMainDlg)
	{
		HICON hicon = LoadIcon(GetModuleHandle(0),MAKEINTRESOURCE(IDI_ICON_SCYLLA1));
		SendMessage(hWndMainDlg, WM_SETICON, ICON_BIG, (LPARAM)hicon);
		SendMessage(hWndMainDlg, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
		
		swprintf_s(stringBuffer, _countof(stringBuffer),TEXT(APPNAME)TEXT(" ")TEXT(ARCHITECTURE)TEXT(" ")TEXT(APPVERSION)TEXT(" "));
		SetWindowText(hWndMainDlg,stringBuffer);
	}
}


void MainGui::leftButtonDownActionHandler(WPARAM wParam, LPARAM lParam)
{
	if(wParam & MK_CONTROL)
	{

	}
	else if(wParam & MK_SHIFT)
	{

	}
	else
	{

	}
}

void MainGui::dialogInitActionHandler()
{
		setIconAndDialogCaption();

		if (ConfigurationHolder::getConfigObject(DEBUG_PRIVILEGE)->isTrue())
		{
			processLister.setDebugPrivileges();
		}
		

		enableDialogButtons(FALSE);

		Edit_LimitText(GetDlgItem(hWndMainDlg,IDC_EDIT_OEPADDRESS), MAX_HEX_VALUE_EDIT_LENGTH);
		Edit_LimitText(GetDlgItem(hWndMainDlg,IDC_EDIT_IATADDRESS), MAX_HEX_VALUE_EDIT_LENGTH);
		Edit_LimitText(GetDlgItem(hWndMainDlg,IDC_EDIT_IATSIZE), MAX_HEX_VALUE_EDIT_LENGTH);
}

void MainGui::pickDllActionHandler()
{
	if (PickDllGui::initDialog(hInstance,hWndMainDlg, processAccessHelp.moduleList))
	{
		//get selected module
		processAccessHelp.selectedModule = PickDllGui::selectedModule;
		Logger::printfDialog(TEXT("->>> Module %s selected."), processAccessHelp.selectedModule->getFilename());
		Logger::printfDialog(TEXT("Imagebase: ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size: %08X"),processAccessHelp.selectedModule->modBaseAddr,processAccessHelp.selectedModule->modBaseSize);
	}
	else
	{
		processAccessHelp.selectedModule = 0;
	}
}

void MainGui::startDisassemblerGui(HTREEITEM selectedTreeNode)
{
	DWORD_PTR address = importsHandling.getApiAddressByNode(selectedTreeNode);
	if (address)
	{
		DisassemblerGui::initDialog(hInstance,hWndMainDlg,address);
	}
	
}

void MainGui::processSelectedActionHandler(LRESULT index)
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
	SetDlgItemText(hWndMainDlg, IDC_EDIT_OEPADDRESS, stringBuffer);
}



void MainGui::fillProcessListComboBox(HWND hCombo)
{
	if (hCombo) 
	{
		SendMessage(hCombo,CB_RESETCONTENT,0,0);

		std::vector<Process>& processList = processLister.getProcessListSnapshot();

		for (size_t i = 0; i < processList.size(); i++)
		{
			swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("0x%04X - %s - %s"),processList[i].PID,processList[i].filename,processList[i].fullPath);
			SendMessage(hCombo,CB_ADDSTRING,0,(LPARAM)stringBuffer);
		}
	}
}

void MainGui::addTextToOutputLog(const WCHAR * text)
{
	if (hWndMainDlg)
	{
		HWND hList = GetDlgItem(hWndMainDlg,IDC_LIST_LOG);

		ListBox_SetCurSel(hList, ListBox_AddString(hList,text));
	}
}

void MainGui::clearOutputLog()
{
	if (hWndMainDlg)
	{
		SendDlgItemMessage(hWndMainDlg, IDC_LIST_LOG, LB_RESETCONTENT, 0, 0);
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

	GetDlgItemText(hWndMainDlg, IDC_EDIT_OEPADDRESS, stringBuffer, _countof(stringBuffer));

	if (wcslen(stringBuffer) > 1)
	{
		searchAddress = stringToDwordPtr(stringBuffer);
		if (searchAddress)
		{
			if (iatSearch.searchImportAddressTableInProcess(searchAddress, &addressIAT, &sizeIAT))
			{
				Logger::printfDialog(TEXT("IAT found at VA ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" RVA ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size 0x%04X (%d)"),addressIAT, addressIAT - processAccessHelp.targetImageBase,sizeIAT,sizeIAT);

				swprintf_s(stringBuffer, _countof(stringBuffer),TEXT(PRINTF_DWORD_PTR_FULL),addressIAT);
				SetDlgItemText(hWndMainDlg,IDC_EDIT_IATADDRESS,stringBuffer);

				swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("%08X"),sizeIAT);
				SetDlgItemText(hWndMainDlg,IDC_EDIT_IATSIZE,stringBuffer);

				swprintf_s(stringBuffer, _countof(stringBuffer),TEXT("IAT found! Start Address ")TEXT(PRINTF_DWORD_PTR_FULL)TEXT(" Size 0x%04X (%d) "),addressIAT,sizeIAT,sizeIAT);
				MessageBox(hWndMainDlg,stringBuffer, TEXT("IAT found"), MB_OK);
				
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

	GetDlgItemText(hWndMainDlg, IDC_EDIT_IATADDRESS, stringBuffer, sizeof(stringBuffer));
	addressIAT = stringToDwordPtr(stringBuffer);

	GetDlgItemText(hWndMainDlg, IDC_EDIT_IATSIZE, stringBuffer, sizeof(stringBuffer));
	sizeIAT = wcstoul(stringBuffer, NULL, 16);

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

bool MainGui::OnContextMenu(int x, int y) 
{ 
	HWND hwnd = 0; 
	POINT pt = { x, y };        // location of mouse click 
	//TV_ITEM tvi;
	//WCHAR ttt[260] = {0};
	//HTREEITEM selectedTreeNode = 0;

	if ((hwnd = mouseInDialogItem(IDC_TREE_IMPORTS, pt)) != NULL)
	{
		if (TreeView_GetCount(hwnd)) //module list should not be empty
		{
			/*selectedTreeNode = (HTREEITEM)SendDlgItemMessage(hWndMainDlg,IDC_TREE_IMPORTS,TVM_GETNEXTITEM,TVGN_CARET,(LPARAM)selectedTreeNode);
			tvi.mask=TVIF_TEXT;   // item text attrivute

			tvi.pszText=ttt;     // Text is the pointer to the text 

			tvi.cchTextMax=260;   // size of text to retrieve.

			tvi.hItem=selectedTreeNode;   // the selected item

			SendDlgItemMessage(hWndMainDlg,IDC_TREE_IMPORTS,TVM_GETITEM,TVGN_CARET,(LPARAM)&tvi);
			Logger::printfDialog(L"selected %s",tvi.pszText);*/
			DisplayContextMenuImports(hwnd, pt);
		}
		return true;
	}
	//if (PtInRect(&rc, pt)) 
	//{ 
	//	ClientToScreen(hwnd, &pt); 
	//	DisplayContextMenu(hwnd, pt); 
	//	return TRUE; 
	//} 

	// Return FALSE if no menu is displayed. 

	return false; 
}

void MainGui::DisplayContextMenuImports(HWND hwnd, POINT pt)
{
	BOOL menuItem = 0;
	HTREEITEM selectedTreeNode = 0;
	std::vector<Plugin> &pluginList = PluginLoader::getPluginList();
	HMENU hmenuTrackPopup = getCorrectSubMenu(IDR_MENU_IMPORTS, 0);

	appendPluginListToMenu(hmenuTrackPopup);

	if (hmenuTrackPopup)
	{
		menuItem = TrackPopupMenu(hmenuTrackPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, 0);
		if (menuItem)
		{

			if ((menuItem >= PLUGIN_MENU_BASE_ID) && (menuItem <= (int)(pluginList.size() + PLUGIN_MENU_BASE_ID)))
			{
				//wsprintf(stringBuffer, L"%d %s\n",menuItem,pluginList[menuItem - PLUGIN_MENU_BASE_ID].pluginName);
				//MessageBox(0,stringBuffer,L"plugin selection",0);

				DllInjectionPlugin dllInjectionPlugin;
				dllInjectionPlugin.hProcess = ProcessAccessHelp::hProcess;
				dllInjectionPlugin.apiReader = &apiReader;
				dllInjectionPlugin.injectPlugin(pluginList[menuItem - PLUGIN_MENU_BASE_ID], importsHandling.moduleList,selectedProcess->imageBase, selectedProcess->imageSize);
				
				importsHandling.scanAndFixModuleList();
				importsHandling.displayAllImports();
				return;
			}

			selectedTreeNode = (HTREEITEM)SendDlgItemMessage(hWndMainDlg,IDC_TREE_IMPORTS,TVM_GETNEXTITEM,TVGN_CARET,(LPARAM)selectedTreeNode);

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

HWND MainGui::mouseInDialogItem(int dlgItem, POINT pt)
{
	RECT rc;
	HWND hwnd = GetDlgItem(hWndMainDlg, dlgItem);
	if (hwnd)
	{
		// Get the bounding rectangle of the client area. 
		GetClientRect(hwnd, &rc); 

		// Convert the mouse position to client coordinates. 
		ScreenToClient(hwnd, &pt); 

		// If the position is in the client area, display a  
		// shortcut menu.
		if (PtInRect(&rc, pt))
		{
			return hwnd;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

HMENU MainGui::getCorrectSubMenu(int menuItem, int subMenuItem)
{
	HMENU hmenu;            // top-level menu 
	HMENU hmenuTrackPopup;  // shortcut menu 
	// Load the menu resource. 
	if ((hmenu = LoadMenu(hInstance, MAKEINTRESOURCE(menuItem))) == NULL) 
		return 0; 

	hmenuTrackPopup = GetSubMenu(hmenu, subMenuItem);

	if (hmenuTrackPopup)
	{
		return hmenuTrackPopup;
	}
	else
	{
		return 0;
	}
}

void MainGui::DisplayContextMenu(HWND hwnd, POINT pt) 
{ 
	HMENU hmenu;            // top-level menu 
	HMENU hmenuTrackPopup;  // shortcut menu 
	int menuItem;			// selected menu item

	// Load the menu resource. 
	if ((hmenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU_IMPORTS))) == NULL) 
		return; 

	// TrackPopupMenu cannot display the menu bar so get 
	// a handle to the first shortcut menu. 

	hmenuTrackPopup = GetSubMenu(hmenu, 0); 

	// Display the shortcut menu. Track the right mouse 
	// button. 
	if (!hmenuTrackPopup)
	{
		MessageBoxA(0,"hmenuTrackPopup == null","hmenuTrackPopup",0);
	}

	menuItem = TrackPopupMenu(hmenuTrackPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);

	if (menuItem)
	{
		/*if (menuItem == ID_LISTCONTROL_SHOWEXPORTS)
		{
			MessageBox(0,"exports","dshhhhh",0);
		}*/
	}

	// Destroy the menu. 

	DestroyMenu(hmenu); 
}

void MainGui::appendPluginListToMenu(HMENU hMenuTrackPopup)
{
	HMENU newMenu = CreatePopupMenu();
	
	std::vector<Plugin> &pluginList = PluginLoader::getPluginList();

	if (pluginList.size() > 0)
	{
		for (size_t i = 0; i < pluginList.size(); i++)
		{
			AppendMenu(newMenu, MF_STRING, i + PLUGIN_MENU_BASE_ID, pluginList[i].pluginName);
		}

		AppendMenu(hMenuTrackPopup,MF_MENUBARBREAK,0,0);
		AppendMenu(hMenuTrackPopup,MF_POPUP,(UINT_PTR)newMenu,TEXT("Plugins"));
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
			//MessageBox(hWndMainDlg,TEXT("Image dumped successfully."),TEXT("Success"),MB_OK);
		}
		else
		{
			Logger::printfDialog(TEXT("Error: Cannot dump image."));
			MessageBox(hWndMainDlg,TEXT("Cannot dump image."),TEXT("Failure"),MB_OK);
		}

		delete [] targetFile;
	}
}

DWORD_PTR MainGui::getOEPFromGui()
{
	if (GetDlgItemText(hWndMainDlg, IDC_EDIT_OEPADDRESS, stringBuffer, _countof(stringBuffer)))
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
			MessageBox(hWndMainDlg,TEXT("Rebuild failed."),TEXT("Failure"),MB_OK);
		}
		else
		{
			peRebuild.truncateFile(targetFile, newSize);

			Logger::printfDialog(TEXT("Rebuild success %s"), targetFile);
			Logger::printfDialog(TEXT("-> Old file size 0x%08X new file size 0x%08X (%d %%)"), (DWORD)fileSize, newSize, (DWORD)((newSize * 100) / (DWORD)fileSize) );
			//MessageBox(hWndMainDlg,TEXT("Image rebuilded successfully."),TEXT("Success"),MB_OK);
		}


		delete [] targetFile;
	}
}

void MainGui::dumpFixActionHandler()
{
	WCHAR * targetFile = 0;
	WCHAR newFilePath[MAX_PATH];
	ImportRebuild importRebuild;

	if (TreeView_GetCount(GetDlgItem(hWndMainDlg, IDC_TREE_IMPORTS)) < 2)
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

		for (size_t i = wcslen(newFilePath) - 1; i >= 0; i--)
		{
			if (newFilePath[i] == L'.')
			{
				newFilePath[i] = 0;
				break;
			}
		}

		wcscat_s(newFilePath,MAX_PATH, L"_SCY.exe");

		if (importRebuild.rebuildImportTable(targetFile,newFilePath,importsHandling.moduleList))
		{
			//MessageBox(hWndMainDlg,L"Imports rebuilding successful",L"Success",MB_OK);

			Logger::printfDialog(TEXT("Import Rebuild success %s"), newFilePath);
		}
		else
		{
			Logger::printfDialog(TEXT("Import Rebuild failed, target %s"), targetFile);
			MessageBox(hWndMainDlg,L"Imports rebuilding failed",L"Failure",MB_OK);
		}

		delete [] targetFile;
	}

}

void MainGui::enableDialogButtons(BOOL value)
{
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_PICKDLL), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_DUMP), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_DLLINJECT), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_FIXDUMP), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_IATAUTOSEARCH), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_GETIMPORTS), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_SUSPECTIMPORTS), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_INVALIDIMPORTS), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_CLEARIMPORTS), value);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_OPTIONS), TRUE);

	//not yet implemented
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_AUTOTRACE), FALSE);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_SAVETREE), FALSE);
	EnableWindow(GetDlgItem(hWndMainDlg, IDC_BTN_LOADTREE), FALSE);
	
}

void MainGui::showAboutDialog()
{
	AboutGui::initDialog(hInstance,hWndMainDlg);
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
	OptionsGui::initOptionsDialog(hInstance, hWndMainDlg);
}
