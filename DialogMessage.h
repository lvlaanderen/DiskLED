#pragma once

#include "resource.h"
#include "DarkMode.h"


// A message box that follows the Windows app theme (dark or light) like the other dialogs. The system message box is always light.
class CDialogMessage : public CDialog
{
	DECLARE_DYNAMIC(CDialogMessage)

public:
	// sIcon: IDI_INFORMATION, IDI_EXCLAMATION, IDI_ERROR, IDI_QUESTION or NULL for no icon
	CDialogMessage (CWnd* pParent, const CString& sText, const CString& sTitle, LPCTSTR sIcon);

	// The buttons, left to right. DoModal returns the number (1 to 3) of the button that was clicked.
	// Escape and the close button of the window return nEscape.
	void	SetButtons (const CString& s1, const CString& s2, const CString& s3, int nEscape);

	enum { IDD = IDD_DIALOG_MESSAGE };

protected:
	virtual BOOL	OnInitDialog ();
	virtual void	OnCancel ();

	afx_msg HBRUSH	OnCtlColor (CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void	OnSettingChange (UINT uFlags, LPCTSTR lpszSection);
	afx_msg void	OnButton (UINT nID);

	DECLARE_MESSAGE_MAP()

private:
	CString		m_sText;
	CString		m_sTitle;
	LPCTSTR		m_sIcon;
	CString		m_asButtons[3];
	int			m_nEscape;
	CDarkTheme	m_oTheme;
};


// Like MessageBox, with the same arguments and results. Supports MB_OK, MB_OKCANCEL, MB_YESNO and MB_YESNOCANCEL, and the icons MB_ICONINFORMATION, MB_ICONEXCLAMATION, MB_ICONERROR and MB_ICONQUESTION.
int	ShowMessage (CWnd* pParent, const CString& sText, const CString& sTitle = TEXT ("DiskLED"), UINT nType = MB_OK | MB_ICONINFORMATION);

// A message with buttons of your own (up to three, left to right). Returns the number (1 to 3) of the clicked button, nEscape for Escape.
int	ShowChoice (CWnd* pParent, const CString& sText, const CString& sTitle, LPCTSTR sIcon, const CString& s1, const CString& s2, const CString& s3, int nEscape);
