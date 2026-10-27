#pragma once

// Colours of the dark configuration dialog
#define	DARK_COLOR_BACK				RGB (32, 32, 32)
#define	DARK_COLOR_EDIT				RGB (45, 45, 45)
#define	DARK_COLOR_TEXT				RGB (240, 240, 240)
#define	DARK_COLOR_TEXT_DIM			RGB (170, 170, 170)
#define	DARK_COLOR_LINE				RGB (80, 80, 80)

// Are Windows apps set to the dark theme, and does this version of Windows support dark controls (Windows 10 1809 or later)?
BOOL	IsDarkModeEnabled ();

// Make a dialog dark: title bar, and the themes of its buttons, combo boxes and edit fields. The caller colours the background and the texts (WM_CTLCOLOR*).
void	EnableDarkMode (HWND hDialog);

// Undo what EnableDarkMode did to the process. Call when the dialog is closed.
void	ReleaseDarkMode ();


// A tab strip that draws itself dark. Behaves like the normal tab control when not dark.
class CDarkTabCtrl : public CTabCtrl
{
public:
	CDarkTabCtrl () : m_bDark (FALSE) {}

	void	SetDark (BOOL bDark);

protected:
	BOOL	m_bDark;

	afx_msg void	OnPaint ();
	afx_msg BOOL	OnEraseBkgnd (CDC* pDC);

	DECLARE_MESSAGE_MAP()
};
