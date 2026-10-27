#pragma once

#include "DarkMode.h"


// CDialogAbout dialog

class CDialogAbout : public CDialog
{
	DECLARE_DYNAMIC(CDialogAbout)

public:
	CDialogAbout(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDialogAbout();

// Dialog Data
	enum { IDD = IDD_DIALOG_ABOUT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	CDarkTheme	m_oTheme;									// Follows the Windows app theme (dark or light)

	DECLARE_MESSAGE_MAP()
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnNMClickSyslinkUrl1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickSyslinkUrl2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickSyslinkUrl3(NMHDR *pNMHDR, LRESULT *pResult);
};
