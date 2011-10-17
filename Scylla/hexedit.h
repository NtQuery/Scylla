#pragma once

#ifndef __cplusplus
  #error WTL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLMISC_H__
  #error hexedit.h requires atlmisc.h to be included first
#endif

#ifndef __ATLCTRLS_H__
  #error hexedit.h requires atlmisc.h to be included first requires atlctrls.h to be included first
#endif

/*
#ifdef _WIN64
	address = _wcstoui64(hexString, NULL, 16);
#else
	address = wcstoul(hexString, NULL, 16);
#endif
*/

template< class T, typename NUM_T, class TBase = CEdit, class TWinTraits = CControlWinTraits >
class ATL_NO_VTABLE CHexEditImpl : public CWindowImpl< T, TBase, TWinTraits >
{
public:

	static const short int BASE = 16;
	static const size_t  DIGITS = sizeof(NUM_T) * 2; // 2 digits / byte
	static const size_t STRSIZE = DIGITS + 1;

	static const TCHAR Digits[];
	static const TCHAR OldProcProp[];

	// Operations

	BOOL SubclassWindow(HWND hWnd)
	{
		ATLASSERT(m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));
		BOOL bRet = CWindowImpl< T, TBase, TWinTraits >::SubclassWindow(hWnd);
		if( bRet ) _Init();
		return bRet;
	}

	NUM_T GetValue() const
	{
		ATLASSERT(::IsWindow(m_hWnd));

		TCHAR String[STRSIZE] = { 0 };
		GetWindowText(String, _countof(String));
		return _StringToNum(String);
	}

	void SetValue(NUM_T Num, bool Fill = true)
	{
		ATLASSERT(::IsWindow(m_hWnd));

		TCHAR String[STRSIZE] = { 0 };
		_NumToString(Num, String, Fill);
		SetWindowText(String);
	}

	// Implementation

	void _Init()
	{
		ATLASSERT(::IsWindow(m_hWnd));

		LimitText(DIGITS);
	}

	bool _IsValidChar(TCHAR Char) const
	{
		return ((NUM_T)-1 != _CharToNum(Char));
	}

	bool _IsValidString(const TCHAR String[STRSIZE]) const
	{
		for(int i = 0; String[i]; i++)
		{
			if(!_IsValidChar(String[i]))
				return false;
		}
		return true;
	}

	NUM_T _CharToNum(TCHAR Char) const
	{
		Char = _totupper(Char);

		for(int i = 0; Digits[i]; i++)
		{
			if(Char == Digits[i])
				return i;
		}

		return -1;
	}

	NUM_T _StringToNum(const TCHAR String[STRSIZE]) const
	{
	NUM_T CharNum;
	NUM_T Num = 0;

		for(int i = 0; String[i]; i++)
		{
			CharNum = _CharToNum(String[i]);
			if(CharNum == (NUM_T)-1)
				break;

			Num *= BASE;
			Num += CharNum;
		}

		return Num;
	}

	TCHAR _NumToChar(NUM_T Num) const
	{
		return Digits[Num % BASE];
	}

	void _NumToString(NUM_T Num, TCHAR String[STRSIZE], bool Fill) const
	{
	NUM_T Nums[DIGITS];
	int i, j;

		for(i = DIGITS-1; i >= 0; i--)
		{
			Nums[i] = Num % BASE;
			Num /= BASE;
		}
		for(i = j = 0; i < DIGITS; i++)
		{
			// Only copy num if : non-null OR Fill OR non-null encountered before OR last num
			if(Nums[i] || Fill || j || i == DIGITS-1)
			{
				String[j++] = _NumToChar(Nums[i]);
			}
		}
		String[j] = '\0';
	}

	bool _GetClipboardText(TCHAR String[STRSIZE])
	{
	#ifdef UNICODE
	const UINT Format = CF_UNICODETEXT;
	#else
	const UINT Format = CF_TEXT;
	#endif

		bool RetVal = false;

		if(IsClipboardFormatAvailable(Format) && OpenClipboard())
		{
			HANDLE HMem = GetClipboardData(Format);
			if(HMem)
			{
				_tcsncpy_s(String, STRSIZE, (TCHAR *)GlobalLock(HMem), STRSIZE-1);
				String[STRSIZE-1] = '\0';
				GlobalUnlock(HMem);
				RetVal = true;
			}
			CloseClipboard();
		}

		return RetVal;
	}

	// Message map and handlers

	BEGIN_MSG_MAP_EX(CHexEditImpl)
		MESSAGE_HANDLER_EX(WM_CREATE, OnCreate)
		MSG_WM_SETTEXT(OnSetText)
		MSG_WM_PASTE(OnPaste)
		MSG_WM_CHAR(OnChar)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT lRes = DefWindowProc();
		_Init();
		return lRes;
	}

	int OnSetText(LPCTSTR lpstrText)
	{
		bool PassThrough = (_tcslen(lpstrText) <= DIGITS) && _IsValidString(lpstrText);
		if(!PassThrough)
		{
			MessageBeep(-1);
			return FALSE;
		}

		SetMsgHandled(FALSE);
		return TRUE;
	}

	void OnPaste()
	{
		TCHAR String[STRSIZE];
		bool PassThrough = !_GetClipboardText(String) || _IsValidString(String);
		if(!PassThrough)
		{
			MessageBeep(-1);
			return;
		}

		SetMsgHandled(FALSE);
		return;
	}

	void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		// ignore all printable chars (incl. space) which are not valid digits
		bool PassThrough = !_istprint((TCHAR)nChar) || _IsValidChar((TCHAR)nChar);
		if(!PassThrough)
		{
			MessageBeep(-1);
			return;
		}

		SetMsgHandled(FALSE);
		return;
	}
};

template< class T, typename NUM_T, class TBase, class TWinTraits > const TCHAR CHexEditImpl< T, NUM_T, TBase, TWinTraits >::Digits[] = _T("0123456789ABCDEF");

template<typename NUM_T> class CHexEdit : public CHexEditImpl<CHexEdit<NUM_T>, NUM_T>
{
public:
  DECLARE_WND_CLASS(_T("WTL_HexEdit"))
};
