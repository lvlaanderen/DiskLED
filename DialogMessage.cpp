// DialogMessage.cpp : a message box that follows the Windows app theme
//

#include "stdafx.h"
#include "DiskLED.h"
#include "DialogMessage.h"


IMPLEMENT_DYNAMIC(CDialogMessage, CDialog)

BEGIN_MESSAGE_MAP(CDialogMessage, CDialog)
	ON_COMMAND(IDCANCEL, &CDialogMessage::OnCancel)
	ON_WM_CTLCOLOR()
	ON_WM_SETTINGCHANGE()
	ON_CONTROL_RANGE(BN_CLICKED, IDC_MSG_BTN1, IDC_MSG_BTN3, &CDialogMessage::OnButton)
END_MESSAGE_MAP()


CDialogMessage::CDialogMessage (CWnd* pParent, const CString& sText, const CString& sTitle, LPCTSTR sIcon)
	: CDialog (CDialogMessage::IDD, pParent), m_sText (sText), m_sTitle (sTitle), m_sIcon (sIcon), m_nEscape (1)
{
	m_asButtons[0]	= TEXT ("OK");
}

void CDialogMessage::SetButtons (const CString& s1, const CString& s2, const CString& s3, int nEscape)
{
	m_asButtons[0]	= s1;
	m_asButtons[1]	= s2;
	m_asButtons[2]	= s3;
	m_nEscape		= nEscape;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The window is sized to its text: icon and text on top, the buttons below, right aligned
//
////////////////////////////////////////////////////////////////////////////////

BOOL CDialogMessage::OnInitDialog ()
{
	CDialog::OnInitDialog ();

	SetWindowText (m_sTitle);

	// Sizes in dialog units: margin, button width and height, space between buttons
	CRect	oUnits (0, 0, 10, 14);
	CRect	oButtonUnits (0, 0, 56, 4);
	MapDialogRect (oUnits);
	MapDialogRect (oButtonUnits);

	int		nMargin		= oUnits.right;
	int		nButtonH	= oUnits.bottom;
	int		nButtonW	= oButtonUnits.right;
	int		nGap		= oButtonUnits.bottom;

	// Icon
	CWnd*	pIcon		= GetDlgItem (IDC_MSG_ICON);
	CWnd*	pText		= GetDlgItem (IDC_MSG_TEXT);
	int		nIconW		= GetSystemMetrics (SM_CXICON);
	int		nIconH		= GetSystemMetrics (SM_CYICON);
	int		nTextLeft	= nMargin;

	if (m_sIcon != NULL)
	{
		pIcon->SendMessage (STM_SETICON, (WPARAM) LoadIcon (NULL, m_sIcon), 0);
		pIcon->SetWindowPos (NULL, nMargin, nMargin, nIconW, nIconH, SWP_NOZORDER);
		nTextLeft	= nMargin * 2 + nIconW;
	}
	else
	{
		pIcon->ShowWindow (SW_HIDE);
	}

	// Text: the width of the template, as high as it needs to be
	CRect	oText;
	pText->GetWindowRect (oText);
	int		nTextW		= oText.Width ();
	CRect	oMeasure (0, 0, nTextW, 0);
	CDC*	pDC			= pText->GetDC ();
	CFont*	pOldFont	= pDC->SelectObject (pText->GetFont ());

	pDC->DrawText (m_sText, oMeasure, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
	pDC->SelectObject (pOldFont);
	pText->ReleaseDC (pDC);

	int		nTextH		= oMeasure.Height ();
	int		nBlockH		= (m_sIcon != NULL && nIconH > nTextH) ? nIconH : nTextH;

	pText->SetWindowText (m_sText);
	pText->SetWindowPos (NULL, nTextLeft, nMargin + (nBlockH - nTextH) / 2, nTextW, nTextH, SWP_NOZORDER);

	// Buttons
	int		nButtons	= 0;
	for (int i = 0; i < 3; i++)
	{
		if (!m_asButtons[i].IsEmpty ())
		{
			nButtons++;
		}
	}

	int		nButtonsW	= nButtons * nButtonW + (nButtons - 1) * nGap;
	int		nClientW	= nTextLeft + nTextW + nMargin;
	if (nButtonsW + 2 * nMargin > nClientW)
	{
		nClientW	= nButtonsW + 2 * nMargin;
	}
	int		nButtonsTop	= nMargin + nBlockH + nMargin;
	int		nLeft		= nClientW - nMargin - nButtonsW;

	for (int i = 0; i < 3; i++)
	{
		CWnd*	pButton	= GetDlgItem (IDC_MSG_BTN1 + i);

		if (m_asButtons[i].IsEmpty ())
		{
			pButton->ShowWindow (SW_HIDE);
			continue;
		}

		pButton->SetWindowText (m_asButtons[i]);
		pButton->SetWindowPos (NULL, nLeft, nButtonsTop, nButtonW, nButtonH, SWP_NOZORDER);
		nLeft	+= nButtonW + nGap;
	}

	// The window
	CRect	oWindow (0, 0, nClientW, nButtonsTop + nButtonH + nMargin);
	AdjustWindowRectEx (oWindow, GetStyle (), FALSE, GetExStyle ());
	SetWindowPos (NULL, 0, 0, oWindow.Width (), oWindow.Height (), SWP_NOMOVE | SWP_NOZORDER);

	// A parent that cannot be seen (the hidden main window) is nothing to center on, and the message must not stay behind other windows
	CWnd*	pParent		= GetParent ();
	BOOL	bParentSeen	= (pParent != NULL && pParent->IsWindowVisible ());

	CenterWindow (bParentSeen ? pParent : GetDesktopWindow ());
	if (!bParentSeen)
	{
		SetWindowPos (&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	SetForegroundWindow ();

	// Dark theme, if Windows apps use it
	m_oTheme.Update (m_hWnd);

	if (m_sIcon == IDI_ERROR)
	{
		MessageBeep (MB_ICONERROR);
	}
	else if (m_sIcon == IDI_EXCLAMATION)
	{
		MessageBeep (MB_ICONEXCLAMATION);
	}

	GetDlgItem (IDC_MSG_BTN1)->SetFocus ();
	SetDefID (IDC_MSG_BTN1);

	// The focus was set by hand
	return FALSE;
}

void CDialogMessage::OnButton (UINT nID)
{
	EndDialog (nID - IDC_MSG_BTN1 + 1);
}

void CDialogMessage::OnCancel ()
{
	EndDialog (m_nEscape);
}

HBRUSH CDialogMessage::OnCtlColor (CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH	hBrush	= m_oTheme.CtlColor (pDC, FALSE);

	return hBrush ? hBrush : CDialog::OnCtlColor (pDC, pWnd, nCtlColor);
}

void CDialogMessage::OnSettingChange (UINT uFlags, LPCTSTR lpszSection)
{
	CDialog::OnSettingChange (uFlags, lpszSection);

	if (m_hWnd && lpszSection && _tcscmp (lpszSection, TEXT ("ImmersiveColorSet")) == 0)
	{
		m_oTheme.Update (m_hWnd);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	The functions to use
//
////////////////////////////////////////////////////////////////////////////////

int ShowChoice (CWnd* pParent, const CString& sText, const CString& sTitle, LPCTSTR sIcon, const CString& s1, const CString& s2, const CString& s3, int nEscape)
{
	CDialogMessage	oDialog (pParent, sText, sTitle, sIcon);

	oDialog.SetButtons (s1, s2, s3, nEscape);

	return (int) oDialog.DoModal ();
}

int ShowMessage (CWnd* pParent, const CString& sText, const CString& sTitle, UINT nType)
{
	LPCTSTR	sIcon	= NULL;
	int		nResult	= 0;

	switch (nType & MB_ICONMASK)
	{
	case MB_ICONERROR:
		sIcon	= IDI_ERROR;
		break;
	case MB_ICONEXCLAMATION:
		sIcon	= IDI_EXCLAMATION;
		break;
	case MB_ICONQUESTION:
		sIcon	= IDI_QUESTION;
		break;
	case MB_ICONINFORMATION:
		sIcon	= IDI_INFORMATION;
		break;
	}

	switch (nType & MB_TYPEMASK)
	{
	case MB_OKCANCEL:
		nResult	= ShowChoice (pParent, sText, sTitle, sIcon, TEXT ("OK"), TEXT ("Cancel"), TEXT (""), 2);
		return (nResult == 1) ? IDOK : IDCANCEL;
	case MB_YESNO:
		nResult	= ShowChoice (pParent, sText, sTitle, sIcon, TEXT ("Yes"), TEXT ("No"), TEXT (""), 2);
		return (nResult == 1) ? IDYES : IDNO;
	case MB_YESNOCANCEL:
		nResult	= ShowChoice (pParent, sText, sTitle, sIcon, TEXT ("Yes"), TEXT ("No"), TEXT ("Cancel"), 3);
		return (nResult == 1) ? IDYES : ((nResult == 2) ? IDNO : IDCANCEL);
	}

	ShowChoice (pParent, sText, sTitle, sIcon, TEXT ("OK"), TEXT (""), TEXT (""), 1);

	return IDOK;
}
