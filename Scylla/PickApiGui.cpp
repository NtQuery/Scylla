#include "PickApiGui.h"

#include <atlconv.h> // string conversion
#include "WindowDeferrer.h"

PickApiGui::PickApiGui(const std::vector<ModuleInfo> &moduleList) : moduleList(moduleList)
{
	selectedApi = 0;
	hIcon.LoadIcon(IDI_ICON_SCYLLA);
}

BOOL PickApiGui::OnInitDialog(CWindow wndFocus, LPARAM lInitParam)
{
	DoDataExchange(); // attach controls

	fillDllComboBox(ComboDllSelect);

	CenterWindow();

	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	GetWindowRect(&minDlgSize);

	return TRUE;
}

void PickApiGui::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize = CPoint(minDlgSize.Size());
}

void PickApiGui::OnSizing(UINT fwSide, RECT* pRect)
{
	// Get size difference
	CRect rectOld;
	GetWindowRect(&rectOld);
	CRect rectNew = *pRect;

	sizeOffset = rectNew.Size() - rectOld.Size();
}

void PickApiGui::OnSize(UINT nType, CSize size)
{
	const WindowDeferrer::Deferrable controls[] =
	{
		{IDC_GROUP_DLL, false, false, true, false},
		{IDC_CBO_DLLSELECT, false, false, true, false},

		{IDC_GROUP_APIS, false, false, true, true},
		{IDC_LIST_APISELECT,  false, false, true, true},
		{IDC_STATIC_APIFILTER, false, true, false, false},
		{IDC_EDIT_APIFILTER, false, true, true, false},

		{IDC_BTN_PICKAPI_OK, true, true, false, false},
		{IDC_BTN_PICKAPI_CANCEL, true, true, false, false}
	};

	if(nType == SIZE_RESTORED)
	{
		WindowDeferrer deferrer(m_hWnd, controls, _countof(controls));
		deferrer.defer(sizeOffset.cx, sizeOffset.cy);
		sizeOffset.SetSize(0, 0);
	}
}

void PickApiGui::OnOK(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int indexDll = ComboDllSelect.GetCurSel();
	int indexApi = ListApiSelect.GetCurSel();
	if (indexDll != CB_ERR && indexApi != CB_ERR)
	{
		selectedApi = apiListTemp[indexApi];
		EndDialog(1);
	}
}

void PickApiGui::OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	EndDialog(0);
}

void PickApiGui::OnDllListSelected(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	int indexDll = ComboDllSelect.GetCurSel();
	if (indexDll != CB_ERR)
	{
		apiListTemp = moduleList[indexDll].apiList;
		fillApiListBox(ListApiSelect, apiListTemp);
		EditApiFilter.SetWindowText(L"");
	}
}

void PickApiGui::OnApiFilterUpdated(UINT uNotifyCode, int nID, CWindow wndCtl)
{
	// remove from apiListTemp that don't fit
	int indexDll = ComboDllSelect.GetCurSel();
	if (indexDll != CB_ERR)
	{
		WCHAR filter[MAX_PATH];
		int lenFilter = EditApiFilter.GetWindowText(filter, _countof(filter));
		if(lenFilter > 0)
		{
			apiListTemp.clear();

			const std::vector<ApiInfo *> &apis = moduleList[indexDll].apiList;

			for (size_t i = 0; i < apis.size(); i++)
			{
				ApiInfo* api = apis[i];
				if(api->name[0] != '\0')
				{
					CA2WEX<MAX_PATH> wStr(api->name);
					if(!_wcsnicmp(wStr, filter, lenFilter))
					{
						apiListTemp.push_back(api);
					}
				}
				else
				{
					WCHAR buf[6];
					swprintf_s(buf, _countof(buf), L"#%04X", api->ordinal);
					if(!_wcsnicmp(buf, filter, lenFilter))
					{
						apiListTemp.push_back(api);
					}
				}
			}
		}
		else
		{
			apiListTemp = moduleList[indexDll].apiList;
		}

		fillApiListBox(ListApiSelect, apiListTemp);
	}
}

void PickApiGui::fillDllComboBox(CComboBox& combo)
{
	combo.ResetContent();

	for (size_t i = 0; i < moduleList.size(); i++)
	{
		combo.AddString(moduleList[i].fullPath);
	}
}

void PickApiGui::fillApiListBox(CListBox& list, const std::vector<ApiInfo *> &apis)
{
	list.ResetContent();

	for (size_t i = 0; i < apis.size(); i++)
	{
		const ApiInfo* api = apis[i];
		if(api->name[0] != '\0')
		{
			CA2WEX<MAX_PATH> wStr(api->name);
			list.AddString(wStr);
		}
		else
		{
			WCHAR buf[6];
			swprintf_s(buf, _countof(buf), L"#%04X", api->ordinal);
			list.AddString(buf);
		}
	}
}
