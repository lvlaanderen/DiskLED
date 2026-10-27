// DialogAbout.cpp : implementation file
//

#include "stdafx.h"
#include "DiskLED.h"
#include "DialogAbout.h"
#include "commctrl.h"


// CDialogAbout dialog

IMPLEMENT_DYNAMIC(CDialogAbout, CDialog)

CDialogAbout::CDialogAbout(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogAbout::IDD, pParent)
{

}

CDialogAbout::~CDialogAbout()
{
}

void CDialogAbout::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDialogAbout, CDialog)
	ON_WM_CTLCOLOR()
	ON_WM_SETTINGCHANGE()
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_URL1, &CDialogAbout::OnNMClickSyslinkUrl1)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_URL2, &CDialogAbout::OnNMClickSyslinkUrl2)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_URL3, &CDialogAbout::OnNMClickSyslinkUrl3)
END_MESSAGE_MAP()


// CDialogAbout message handlers

BOOL CDialogAbout::OnInitDialog ()
{
	CDialog::OnInitDialog ();

	// Dark theme, if Windows apps use it
	m_oTheme.Update (m_hWnd);

	return TRUE;
}

HBRUSH CDialogAbout::OnCtlColor (CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH	hBrush	= m_oTheme.CtlColor (pDC, FALSE);

	return hBrush ? hBrush : CDialog::OnCtlColor (pDC, pWnd, nCtlColor);
}

void CDialogAbout::OnSettingChange (UINT uFlags, LPCTSTR lpszSection)
{
	CDialog::OnSettingChange (uFlags, lpszSection);

	if (m_hWnd && lpszSection && _tcscmp (lpszSection, TEXT ("ImmersiveColorSet")) == 0)
	{
		m_oTheme.Update (m_hWnd);
	}
}

void CDialogAbout::OnNMClickSyslinkUrl1(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMLINK pNMLink = (PNMLINK) pNMHDR;

	ShellExecute (NULL, TEXT ("open"), pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);

	*pResult = 0;
}

void CDialogAbout::OnNMClickSyslinkUrl3(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMLINK pNMLink = (PNMLINK) pNMHDR;

	ShellExecute (NULL, TEXT ("open"), pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);

	*pResult = 0;
}

void CDialogAbout::OnNMClickSyslinkUrl2(NMHDR *pNMHDR, LRESULT *pResult)
{
	PNMLINK pNMLink = (PNMLINK) pNMHDR;

	ShellExecute (NULL, TEXT ("open"), pNMLink->item.szUrl, NULL, NULL, SW_SHOWNORMAL);

	*pResult = 0;
}
