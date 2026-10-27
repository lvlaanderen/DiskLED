// DarkMode.cpp : dark theme for the dialogs (best effort, plain MFC controls do not support it fully)
//

#include "stdafx.h"
#include "DarkMode.h"
#include <uxtheme.h>
#include <dwmapi.h>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE	20
#endif

#define	DWMWA_DARK_MODE_BEFORE_20H1		19

#define	BUILD_WIN10_1809		17763
#define	BUILD_WIN10_1903		18362
#define	BUILD_WIN10_20H1		18985

#define	APPMODE_ALLOWDARK		1


// The Windows build number, 0 if unknown
static DWORD GetWindowsBuild ()
{
	typedef LONG (WINAPI* PFN_RtlGetVersion) (PRTL_OSVERSIONINFOW);

	HMODULE	hNtDll	= GetModuleHandle (TEXT ("ntdll.dll"));
	if (hNtDll == NULL)
	{
		return 0;
	}

	PFN_RtlGetVersion	pRtlGetVersion	= (PFN_RtlGetVersion) GetProcAddress (hNtDll, "RtlGetVersion");
	if (pRtlGetVersion == NULL)
	{
		return 0;
	}

	RTL_OSVERSIONINFOW	oVersion;
	SecureZeroMemory (&oVersion, sizeof (oVersion));
	oVersion.dwOSVersionInfoSize	= sizeof (oVersion);

	return (pRtlGetVersion (&oVersion) == 0) ? oVersion.dwBuildNumber : 0;
}

BOOL IsDarkModeEnabled ()
{
	DWORD	nLight	= 1;
	DWORD	nSize	= sizeof (nLight);

	if (GetWindowsBuild () < BUILD_WIN10_1809)
	{
		return FALSE;
	}
	if (RegGetValue (HKEY_CURRENT_USER, TEXT ("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), TEXT ("AppsUseLightTheme"),
		RRF_RT_REG_DWORD, NULL, &nLight, &nSize) != ERROR_SUCCESS)
	{
		return FALSE;
	}

	return nLight == 0;
}

// uxtheme.dll ordinals 135 and 136 are undocumented: AllowDarkModeForApp(BOOL) before 1903, SetPreferredAppMode(int) from 1903, and FlushMenuThemes()
void InitDarkMode ()
{
	HMODULE	hUxTheme	= GetModuleHandle (TEXT ("uxtheme.dll"));
	DWORD	nBuild		= GetWindowsBuild ();

	if (hUxTheme == NULL || nBuild < BUILD_WIN10_1809)
	{
		return;
	}

	FARPROC	pSetMode	= GetProcAddress (hUxTheme, MAKEINTRESOURCEA (135));
	FARPROC	pFlushMenus	= GetProcAddress (hUxTheme, MAKEINTRESOURCEA (136));
	if (pSetMode == NULL)
	{
		return;
	}

	if (nBuild >= BUILD_WIN10_1903)
	{
		((int (WINAPI*) (int)) pSetMode) (APPMODE_ALLOWDARK);
	}
	else
	{
		((BOOL (WINAPI*) (BOOL)) pSetMode) (TRUE);
	}
	if (pFlushMenus)
	{
		((void (WINAPI*) ()) pFlushMenus) ();
	}
}

static BOOL CALLBACK ThemeChildWindow (HWND hChild, LPARAM lDark)
{
	TCHAR			sClass[64]	= { 0 };
	LONG			nStyle		= GetWindowLong (hChild, GWL_STYLE);
	BOOL			bDark		= (BOOL) lDark;
	const WCHAR*	sTheme		= NULL;

	GetClassName (hChild, sClass, _countof (sClass));

	if (_tcsicmp (sClass, TEXT ("Button")) == 0)
	{
		// The colour swatches are drawn by the dialog
		if ((nStyle & BS_TYPEMASK) == BS_OWNERDRAW)
		{
			return TRUE;
		}
		sTheme	= L"DarkMode_Explorer";
	}
	else if (_tcsicmp (sClass, TEXT ("ComboBox")) == 0)
	{
		sTheme	= L"DarkMode_CFD";
	}
	else if (_tcsicmp (sClass, TEXT ("Edit")) == 0)
	{
		// A multi-line edit has a scroll bar
		sTheme	= (nStyle & ES_MULTILINE) ? L"DarkMode_Explorer" : L"DarkMode_CFD";
	}
	else
	{
		return TRUE;
	}

	// No theme name: back to the default
	SetWindowTheme (hChild, bDark ? sTheme : NULL, NULL);

	return TRUE;
}

void ApplyDarkMode (HWND hDialog, BOOL bDark)
{
	// The title bar attribute has another number before Windows 10 20H1
	DWORD	nAttribute	= (GetWindowsBuild () >= BUILD_WIN10_20H1) ? DWMWA_USE_IMMERSIVE_DARK_MODE : DWMWA_DARK_MODE_BEFORE_20H1;

	DwmSetWindowAttribute (hDialog, nAttribute, &bDark, sizeof (bDark));
	EnumChildWindows (hDialog, ThemeChildWindow, (LPARAM) bDark);
	RedrawWindow (hDialog, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_FRAME);
}


////////////////////////////////////////////////////////////////////////////////
//
//	The dark theme of a dialog
//
////////////////////////////////////////////////////////////////////////////////

BOOL CDarkTheme::Update (HWND hDialog)
{
	BOOL	bDark	= IsDarkModeEnabled ();

	if (bDark && m_brBack.GetSafeHandle () == NULL)
	{
		m_brBack.CreateSolidBrush (DARK_COLOR_BACK);
		m_brEdit.CreateSolidBrush (DARK_COLOR_EDIT);
	}

	// A light dialog that was never dark already has the default look
	if (bDark == m_bDark && !bDark)
	{
		return FALSE;
	}

	BOOL	bChanged	= (bDark != m_bDark);

	m_bDark	= bDark;
	ApplyDarkMode (hDialog, bDark);

	return bChanged;
}

HBRUSH CDarkTheme::CtlColor (CDC* pDC, BOOL bField)
{
	if (!m_bDark)
	{
		return NULL;
	}

	pDC->SetTextColor (DARK_COLOR_TEXT);
	pDC->SetBkColor (bField ? DARK_COLOR_EDIT : DARK_COLOR_BACK);

	return bField ? (HBRUSH) m_brEdit : (HBRUSH) m_brBack;
}


////////////////////////////////////////////////////////////////////////////////
//
//	A tab strip that draws itself dark
//
////////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CDarkTabCtrl, CTabCtrl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

void CDarkTabCtrl::SetDark (BOOL bDark)
{
	m_bDark	= bDark;
	Invalidate ();
}

BOOL CDarkTabCtrl::OnEraseBkgnd (CDC* pDC)
{
	// Dark: everything is drawn in OnPaint
	return m_bDark ? TRUE : CTabCtrl::OnEraseBkgnd (pDC);
}

void CDarkTabCtrl::OnPaint ()
{
	if (!m_bDark)
	{
		Default ();
		return;
	}

	CPaintDC	dc (this);
	CRect		oClient;
	CFont*		pFont		= GetFont ();
	CFont*		pOldFont	= pFont ? dc.SelectObject (pFont) : NULL;
	int			nSelected	= GetCurSel ();

	GetClientRect (oClient);
	dc.FillSolidRect (oClient, DARK_COLOR_BACK);
	dc.SetBkMode (TRANSPARENT);

	for (int i = 0; i < GetItemCount (); i++)
	{
		CRect	oItemRect;
		TCHAR	sText[128]	= { 0 };
		TCITEM	oItem;

		SecureZeroMemory (&oItem, sizeof (oItem));
		oItem.mask			= TCIF_TEXT;
		oItem.pszText		= sText;
		oItem.cchTextMax	= _countof (sText) - 1;
		GetItem (i, &oItem);
		GetItemRect (i, oItemRect);

		if (i == nSelected)
		{
			dc.FillSolidRect (oItemRect, DARK_COLOR_EDIT);
			dc.FillSolidRect (oItemRect.left, oItemRect.bottom - 2, oItemRect.Width (), 2, GetSysColor (COLOR_HOTLIGHT));
		}
		dc.SetTextColor ((i == nSelected) ? DARK_COLOR_TEXT : DARK_COLOR_TEXT_DIM);
		dc.DrawText (sText, oItemRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
	}

	dc.FillSolidRect (oClient.left, oClient.bottom - 1, oClient.Width (), 1, DARK_COLOR_LINE);

	if (pOldFont)
	{
		dc.SelectObject (pOldFont);
	}
}
