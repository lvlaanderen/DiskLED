// DarkMode.cpp : dark theme for the configuration dialog (best effort, plain MFC controls do not support it fully)
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

#define	BUILD_WIN10_1809		17763
#define	BUILD_WIN10_1903		18362

#define	APPMODE_DEFAULT			0
#define	APPMODE_FORCEDARK		2


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

// uxtheme.dll ordinal 135 lets the process use the dark control themes. It is undocumented: AllowDarkModeForApp(BOOL) before 1903, SetPreferredAppMode(int) from 1903.
static void SetProcessDarkMode (BOOL bDark)
{
	HMODULE	hUxTheme	= GetModuleHandle (TEXT ("uxtheme.dll"));
	DWORD	nBuild		= GetWindowsBuild ();

	if (hUxTheme == NULL || nBuild < BUILD_WIN10_1809)
	{
		return;
	}

	FARPROC	pFunction	= GetProcAddress (hUxTheme, MAKEINTRESOURCEA (135));
	if (pFunction == NULL)
	{
		return;
	}

	if (nBuild >= BUILD_WIN10_1903)
	{
		((int (WINAPI*) (int)) pFunction) (bDark ? APPMODE_FORCEDARK : APPMODE_DEFAULT);
	}
	else
	{
		((BOOL (WINAPI*) (BOOL)) pFunction) (bDark);
	}
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

static BOOL CALLBACK ThemeChildWindow (HWND hChild, LPARAM)
{
	TCHAR	sClass[64]	= { 0 };
	LONG	nStyle		= GetWindowLong (hChild, GWL_STYLE);

	GetClassName (hChild, sClass, _countof (sClass));

	if (_tcsicmp (sClass, TEXT ("Button")) == 0)
	{
		// The colour swatches are drawn by the dialog
		if ((nStyle & BS_TYPEMASK) != BS_OWNERDRAW)
		{
			SetWindowTheme (hChild, L"DarkMode_Explorer", NULL);
		}
	}
	else if (_tcsicmp (sClass, TEXT ("ComboBox")) == 0)
	{
		SetWindowTheme (hChild, L"DarkMode_CFD", NULL);
	}
	else if (_tcsicmp (sClass, TEXT ("Edit")) == 0)
	{
		// A multi-line edit has a scroll bar
		SetWindowTheme (hChild, (nStyle & ES_MULTILINE) ? L"DarkMode_Explorer" : L"DarkMode_CFD", NULL);
	}

	return TRUE;
}

void EnableDarkMode (HWND hDialog)
{
	BOOL	bDark	= TRUE;

	SetProcessDarkMode (TRUE);
	DwmSetWindowAttribute (hDialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &bDark, sizeof (bDark));
	EnumChildWindows (hDialog, ThemeChildWindow, 0);
}

void ReleaseDarkMode ()
{
	SetProcessDarkMode (FALSE);
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
