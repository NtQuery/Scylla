#pragma once

/////////////////////////////////////////////////////////////////////////////
// MultiSelectTree - A tree control with multi-select capabilities
//
// Written by Bjarke Viksoe (bjarke@viksoe.dk)
// Copyright (c) 2005 Bjarke Viksoe.
//
// Add the following macro to the parent's message map:
//   REFLECT_NOTIFICATIONS()
//
// This code may be used in compiled form in any way you desire. This
// source file may be redistributed by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. 
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage to you or your
// computer whatsoever. It's free, so don't hassle me about it.
//
// Beware of bugs.
//

#ifndef __cplusplus
  #error WTL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLMISC_H__
  #error multitree.h requires atlmisc.h to be included first
#endif

#ifndef __ATLCRACK_H__
  #error multitree.h requires atlcrack.h to be included first
#endif

#ifndef __ATLCTRLS_H__
  #error multitree.h requires atlctrls.h to be included first
#endif

// Extended MultiSelectTree styles
static const DWORD MTVS_EX_NOMARQUEE = 0x00000001;

// New control notifications
static const UINT TVN_ITEMSELECTING = 0x0001;
static const UINT TVN_ITEMSELECTED  = 0x0002;

static bool operator==(CTreeItem ti1, CTreeItem ti2)
{
	return ti1.m_hTreeItem == ti2.m_hTreeItem && ti1.m_pTreeView == ti2.m_pTreeView;
}

template< class T, class TBase = CTreeViewCtrlEx, class TWinTraits = CControlWinTraits >
class ATL_NO_VTABLE CMultiSelectTreeViewImpl : 
   public CWindowImpl< T, TBase, TWinTraits >,
   public CCustomDraw< T >
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

	DWORD m_dwExStyle;              // Additional styles
	CTreeItem m_hExtSelStart;       // Item where SHIFT was last pressed
	bool m_bMarquee;                // Are we drawing rubberband?
	CPoint m_ptDragStart;            // Point where rubberband started
	CPoint m_ptDragOld;              // Last mousepos of rubberband

	CSimpleMap<CTreeItem, bool> m_aData;

	CMultiSelectTreeViewImpl() : m_dwExStyle(0), m_bMarquee(false)
	{
	}

	// Operations

	BOOL SubclassWindow(HWND hWnd)
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));
		BOOL bRet = CWindowImpl< T, TBase, TWinTraits >::SubclassWindow(hWnd);
		if( bRet )
			_Init();
		return bRet;
	}

	DWORD SetExtendedTreeStyle(DWORD dwStyle)
	{
		ATLASSERT(!m_ctrlTree.IsWindow());   // Before control is created, please!
		DWORD dwOldStyle = m_dwTreeStyle;
		m_dwTreeStyle = dwStyle;
		return dwOldStyle;
	}

	void SelectItem(HTREEITEM hItem, BOOL bSelect)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		_SelectItem(hItem, bSelect == TRUE);
		if( bSelect )
			TBase::SelectItem(hItem);
	}

	void SelectAllItems(BOOL bSelect)
	{
		ATLASSERT(::IsWindow(m_hWnd));

		for( int i = 0; i < m_aData.GetSize(); i++ )
		{
			_SelectItem(i, bSelect == TRUE);
		}
	}

	CTreeItem GetSelectedItem() const
	{
		ATLASSERT(false);  // Not usable!
		return GetFirstSelectedItem();
	}

	CTreeItem GetFocusItem() const
	{
		return TBase::GetSelectedItem();
	}

	UINT GetItemState(HTREEITEM hItem, UINT nStateMask) const
	{
		UINT nRes = TBase::GetItemState(hItem, nStateMask);
		if( (nStateMask & TVIS_SELECTED) != 0 )
		{
			int iIndex = m_aData.FindKey(CTreeItem(hItem, (CTreeViewCtrlEx*)this));
			if( iIndex >= 0 )
			{
				nRes &= ~TVIS_SELECTED;
				if( m_aData.GetValueAt(iIndex) )
					nRes |= TVIS_SELECTED;
			}
		}
		return nRes;
	}

	CTreeItem GetFirstSelectedItem() const
	{
		if( m_aData.GetSize() == 0 )
			return NULL;

		for( int i = 0; i < m_aData.GetSize(); i++ )
		{
			if( m_aData.GetValueAt(i) )
				return m_aData.GetKeyAt(i);
		}
		return NULL;
	}

	CTreeItem GetNextSelectedItem(HTREEITEM hItem) const
	{
		int iIndex = m_aData.FindKey(CTreeItem(hItem, (CTreeViewCtrlEx*)this));
		if( iIndex < 0 )
			return NULL;

		for( int i = iIndex + 1; i < m_aData.GetSize(); i++ )
		{
			if( m_aData.GetValueAt(i) )
				return m_aData.GetKeyAt(i);
		}
		return NULL;
	}

	int GetSelectedCount() const
	{
		int nCount = 0;
		for( int i = 0; i < m_aData.GetSize(); i++ )
		{
			if( m_aData.GetValueAt(i) )
				nCount++;
		}
		return nCount;
	}

	// Implementation

	void _Init()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		ModifyStyle(TVS_SHOWSELALWAYS, 0);
	}

	void _SelectItem(int iIndex, bool bSelect, int action = TVC_UNKNOWN)
	{
		if( iIndex < 0 )
			return;
		bool bSelected = m_aData.GetValueAt(iIndex);
		// Don't change if state is already updated (avoids flicker)
		if( bSelected == bSelect )
			return;
		CTreeItem cItem = m_aData.GetKeyAt(iIndex);
		CWindow parent = GetParent();
		// Send notifications
		NMTREEVIEW nmtv = { 0 };
		nmtv.hdr.code = TVN_ITEMSELECTING;
		nmtv.hdr.hwndFrom = m_hWnd;
		nmtv.hdr.idFrom = GetDlgCtrlID();
		nmtv.action = action;
		nmtv.itemNew.hItem = cItem;
		nmtv.itemNew.lParam = GetItemData(cItem);
		nmtv.itemNew.state = bSelect ? TVIS_SELECTED : 0;
		nmtv.itemNew.stateMask = TVIS_SELECTED;
		if( parent.SendMessage(WM_NOTIFY, nmtv.hdr.idFrom, (LPARAM) &nmtv) != 0 )
			return;
		// Change state
		m_aData.SetAtIndex(iIndex, cItem, bSelect);
		// Repaint item
		CRect rcItem;
		if( GetItemRect(cItem, &rcItem, FALSE) )
			InvalidateRect(&rcItem, TRUE);
		// More notifications
		nmtv.hdr.code = TVN_ITEMSELECTED;
		parent.SendMessage(WM_NOTIFY, nmtv.hdr.idFrom, (LPARAM) &nmtv);
	}

	void _SelectItem(HTREEITEM hItem, bool bSelect, int action = TVC_UNKNOWN)
	{
		_SelectItem(m_aData.FindKey(CTreeItem(hItem, (CTreeViewCtrlEx*)this)), bSelect, action);
	}

	void _SelectTree(HTREEITEM hItem, HTREEITEM hGoal, int action)
	{
		if( !_SelectTreeSub(hItem, hGoal, action) )
			return;
		hItem = GetParentItem(hItem);
		while( (hItem = GetNextSiblingItem(hItem)) != NULL )
		{
			if( !_SelectTreeSub(hItem, hGoal, action) )
				return;
		}
	}

	bool _SelectTreeSub(HTREEITEM hItem, HTREEITEM hGoal, int action)
	{
		while( hItem != NULL )
		{
			_SelectItem(hItem, true, action);
			if( hItem == hGoal )
				return false;
			if( (TBase::GetItemState(hItem, TVIS_EXPANDED) & TVIS_EXPANDED) != 0 )
			{
				if( !_SelectTreeSub(GetChildItem(hItem), hGoal, action) )
					return false;
			}
			hItem = GetNextSiblingItem(hItem);
		}
		return true;
	}

	void _SelectBox(CRect rc)
	{
		CTreeItem hItem = GetFirstVisibleItem();
		while( hItem != NULL )
		{
			CTreeItem cItem(hItem, (CTreeViewCtrlEx*)this);
			int i = m_aData.FindKey(cItem);
			if(i >= 0 && !m_aData.GetValueAt(i)) // ignore already selected
			{
				CRect rcItem, rcTemp;
				GetItemRect(hItem, &rcItem, TRUE);
				_SelectItem(hItem, rcTemp.IntersectRect(&rcItem, &rc) == TRUE, TVC_BYMOUSE);
			}
			hItem = GetNextVisibleItem(hItem);
		}
	}

	void _DrawDragRect(CPoint pt)
	{
		CClientDC dc = m_hWnd;
		CSize szFrame(1, 1);
		CRect rect(m_ptDragStart, pt);
		rect.NormalizeRect();
		CBrush brush = CDCHandle::GetHalftoneBrush();
		if( !brush.IsNull() )
		{
			CBrushHandle hOldBrush = dc.SelectBrush(brush);
			dc.PatBlt(rect.left, rect.top, rect.Width(), szFrame.cy, PATINVERT);
			dc.PatBlt(rect.left, rect.bottom - szFrame.cy, rect.Width(), szFrame.cy, PATINVERT);
			dc.PatBlt(rect.left, rect.top + szFrame.cy, szFrame.cx, rect.Height() - (szFrame.cy * 2), PATINVERT);
			dc.PatBlt(rect.right - szFrame.cx, rect.top + szFrame.cy, szFrame.cx, rect.Height() - (szFrame.cy * 2), PATINVERT);
			dc.SelectBrush(hOldBrush);
		}
	}

   // Message map and handlers

	BEGIN_MSG_MAP_EX(CMultiSelectTreeViewImpl)
		MESSAGE_HANDLER_EX(WM_CREATE, OnCreate)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_KEYDOWN(OnKeyDown)
		MSG_WM_KEYUP(OnKeyUp)
		MSG_WM_CHAR(OnChar)
		MSG_WM_SETFOCUS(OnSetFocus)
		MSG_WM_LBUTTONDOWN(OnLButtonDown)
		MSG_WM_LBUTTONUP(OnLButtonUp)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_CAPTURECHANGED(OnCaptureChanged)
		MESSAGE_HANDLER_EX(TVM_INSERTITEM, OnInsertItem)
		REFLECTED_NOTIFY_CODE_HANDLER_EX(TVN_DELETEITEM, OnDeleteItem)
		CHAIN_MSG_MAP_ALT( CCustomDraw< T >, 1 )
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lRes = DefWindowProc();
		_Init();
		return (int)lRes;
	}

	void OnDestroy()
	{
		m_aData.RemoveAll();
		SetMsgHandled(FALSE);
	}

	void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if( nChar == VK_SHIFT )
			m_hExtSelStart = GetFocusItem();

		if( ::GetAsyncKeyState(VK_SHIFT) < 0 && m_hExtSelStart == GetFocusItem() ) 
		{
			switch( nChar )
			{
			case VK_UP:
			case VK_DOWN:
			case VK_HOME:
			case VK_END:
			case VK_NEXT:
			case VK_PRIOR:
				for( int i = 0; i < m_aData.GetSize(); i++ )
				{
					_SelectItem(i, false, TVC_BYKEYBOARD);
				}
			}
		}
		SetMsgHandled(FALSE);
	}

	void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if( ::GetAsyncKeyState(VK_SHIFT) < 0 )
		{
			switch( nChar )
			{
			case VK_UP:
			case VK_DOWN:
			case VK_HOME:
			case VK_END:
			case VK_NEXT:
			case VK_PRIOR:
				HTREEITEM hItem = GetFocusItem();
				// Is current or first-shift-item the upper item?
				CRect rcItem1, rcItem2;
				GetItemRect(m_hExtSelStart, &rcItem1, TRUE);
				GetItemRect(hItem, &rcItem2, TRUE);
				// Select from current item to item where SHIFT was pressed
				if( rcItem1.top > rcItem2.top )
					_SelectTree(hItem, m_hExtSelStart, TVC_BYKEYBOARD);
				else
					_SelectTree(m_hExtSelStart, hItem, TVC_BYKEYBOARD);
				_SelectItem(hItem, true, TVC_BYKEYBOARD);
			}
		}
		SetMsgHandled(FALSE);
	}

	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		if( nChar == VK_SPACE )
		{
			HTREEITEM hItem = GetFocusItem();
			_SelectItem(hItem, (GetItemState(hItem, TVIS_SELECTED) & TVIS_SELECTED) == 0, TVC_BYKEYBOARD);
			return;
		}
		SetMsgHandled(FALSE);
	}

	void OnSetFocus(CWindow wndOld)
	{
		DefWindowProc();
		// FIX: We really need the focus-rectangle in this control since it
		//      improves the navigation a lot. So let's ask Windows to display it.
		SendMessage(WM_UPDATEUISTATE, MAKEWPARAM(UIS_CLEAR, UISF_HIDEFOCUS));
	}

	void OnLButtonDown(UINT nFlags, CPoint point)
	{
		SetMsgHandled(FALSE);

		// Hit-test and figure out where we're clicking...
		TVHITTESTINFO hti = { 0 };
		hti.pt = point;
		HTREEITEM hItem = HitTest(&hti);
		if( (hItem == NULL || (hti.flags & TVHT_ONITEMRIGHT) != 0) )
		{
			if( (m_dwExStyle & MTVS_EX_NOMARQUEE) == 0 && ::DragDetect(m_hWnd, point) )
			{
				// Great we're dragging a rubber-band
				// Clear selection of CTRL is not down
				if( ::GetAsyncKeyState(VK_CONTROL) >= 0 )
				{
					for( int i = 0; i < m_aData.GetSize(); i++ )
					{
						_SelectItem(i, false, TVC_BYMOUSE);
					}
					UpdateWindow();
				}
				// Now start drawing the rubber-band...
				SetCapture();
				m_ptDragStart = m_ptDragOld = point;
				_DrawDragRect(point);
				m_bMarquee = true;
				SetMsgHandled(TRUE);
				return;
			}
		}

		if( hItem == NULL )
			return;

		if( (hti.flags & TVHT_ONITEMBUTTON) != 0 )
			return;

		// Great, let's do an advanced selection
		if( (hti.flags & TVHT_ONITEMRIGHT) != 0 )
		{
			for( int i = 0; i < m_aData.GetSize(); i++ )
			{
				_SelectItem(i, false, TVC_BYMOUSE);
			}
			return;
		}
		int iIndex = m_aData.FindKey(CTreeItem(hItem, (CTreeViewCtrlEx*)this));
		if( iIndex < 0 )
			return;
		// Simulate drag'n'drop?
		if( m_aData.GetValueAt(iIndex) && (GetStyle() & TVS_DISABLEDRAGDROP) == 0 && ::DragDetect(m_hWnd, point) )
		{
			NMTREEVIEW nmtv = { 0 };
			nmtv.hdr.code = TVN_BEGINDRAG;
			nmtv.hdr.hwndFrom = m_hWnd;
			nmtv.hdr.idFrom = GetDlgCtrlID();
			nmtv.itemNew.hItem = hItem;
			nmtv.itemNew.lParam = GetItemData(hItem);
			CWindow parent = GetParent();
			parent.SendMessage(WM_NOTIFY, nmtv.hdr.idFrom, (LPARAM) &nmtv);
		}
		bool bSelected = m_aData.GetValueAt(iIndex);
		if( ::GetAsyncKeyState(VK_SHIFT) < 0 )
		{
			// Is current or first-shift-item the upper item?
			CRect rcItem1, rcItem2;
			GetItemRect(m_hExtSelStart, &rcItem1, TRUE);
			GetItemRect(hItem, &rcItem2, TRUE);
			// Select from current item to item where SHIFT was pressed
			if( rcItem1.top > rcItem2.top )
				_SelectTree(hItem, m_hExtSelStart, TVC_BYMOUSE);
			else
				_SelectTree(m_hExtSelStart, hItem, TVC_BYMOUSE);
		}
		else if( ::GetAsyncKeyState(VK_CONTROL) < 0 )
		{
			// Just toggle item
			_SelectItem(iIndex, !bSelected, TVC_BYMOUSE);
		}
		else
		{
			// Remove current selection and replace it with clicked item
			for( int i = 0; i < m_aData.GetSize(); i++ )
			{
				_SelectItem(i, i == iIndex, TVC_BYMOUSE);
			}
		}
	}

	void OnLButtonUp(UINT nFlags, CPoint point)
	{
		if( m_bMarquee )
			ReleaseCapture();
		SetMsgHandled(FALSE);
	}

	void OnMouseMove(UINT nFlags, CPoint point)
	{
		if( m_bMarquee )
		{
			CRect rc(m_ptDragStart, point);
			_DrawDragRect(m_ptDragOld);
			rc.NormalizeRect();
			_SelectBox(rc);
			UpdateWindow();
			_DrawDragRect(point);
			m_ptDragOld = point;
		}
		SetMsgHandled(FALSE);
	}

	void OnCaptureChanged(CWindow wnd)
	{
		if( m_bMarquee )
		{
			_DrawDragRect(m_ptDragOld);
			m_bMarquee = false;
		}
		SetMsgHandled(FALSE);
	}

	LRESULT OnInsertItem(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		HTREEITEM hItem = (HTREEITEM) DefWindowProc(uMsg, wParam, lParam);
		if( hItem == NULL )
			return (LRESULT) hItem;
		CTreeItem cItem(hItem, (CTreeViewCtrlEx*)this);
		// We manage a bit of extra information for each item. We'll store
		// this in an ATL::CSimpleMap. Not a particular speedy structure for lookups.
		// Don't keep too many items around in the tree!
		m_aData.Add(cItem, false);
		return (LRESULT) hItem;
	}

	LRESULT OnDeleteItem(NMHDR* pnmh)
	{
		const NMTREEVIEW* lpNMTV = (NMTREEVIEW*) pnmh;
		m_aData.Remove(CTreeItem(lpNMTV->itemNew.hItem, (CTreeViewCtrlEx*)this));
		return 0;
	}

	// Custom Draw

	DWORD OnPrePaint(int /*idCtrl*/, NMCUSTOMDRAW* /*lpNMCustomDraw*/)
	{
		return CDRF_NOTIFYITEMDRAW;   // We need per-item notifications
	}

	DWORD OnItemPrePaint(int /*idCtrl*/, NMCUSTOMDRAW* lpNMCustomDraw)
	{
		NMTVCUSTOMDRAW* lpTVCD = (NMTVCUSTOMDRAW*) lpNMCustomDraw;
		HTREEITEM hItem = (HTREEITEM) lpTVCD->nmcd.dwItemSpec;
		int iIndex = m_aData.FindKey(CTreeItem(hItem, (CTreeViewCtrlEx*)this));
		if( iIndex >= 0 )
		{
			bool bSelected = m_aData.GetValueAt(iIndex);
			// Trick TreeView into displaying correct selection colors
			if( bSelected )
			{
				lpTVCD->clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
				lpTVCD->clrTextBk = ::GetSysColor(COLOR_HIGHLIGHT);            
			}
			else
			{
				// Special case of tree-item actually have selection, but our
				// state says it is currently not selected (CTRL+click on same item twice).
				if( (lpTVCD->nmcd.uItemState & CDIS_SELECTED) != 0 )
				{			
					COLORREF clrText = GetTextColor();
					if( clrText == CLR_NONE )
						clrText = ::GetSysColor(COLOR_WINDOWTEXT);
					COLORREF clrBack = GetBkColor();
					if( clrBack == CLR_NONE )
						clrBack = ::GetSysColor(COLOR_WINDOW);
					//CDCHandle dc = lpTVCD->nmcd.hdc;
					//dc.SetTextColor(clrText);
					//dc.SetBkColor(clrBack);
					lpTVCD->clrText = clrText;
					lpTVCD->clrTextBk = clrBack;  
				}
			}
			return CDRF_NEWFONT;
		}
		return CDRF_DODEFAULT;
	}
};

class CMultiSelectTreeViewCtrl : public CMultiSelectTreeViewImpl<CMultiSelectTreeViewCtrl, CTreeViewCtrlEx, CWinTraitsOR<TVS_SHOWSELALWAYS> >
{
public:
   DECLARE_WND_SUPERCLASS(_T("WTL_MultiSelectTree"), GetWndClassName())  
};
