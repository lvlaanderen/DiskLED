#pragma once

// Colours of the dark dialogs
#define	DARK_COLOR_BACK				RGB (32, 32, 32)
#define	DARK_COLOR_EDIT				RGB (45, 45, 45)
#define	DARK_COLOR_TEXT				RGB (240, 240, 240)
#define	DARK_COLOR_TEXT_DIM			RGB (170, 170, 170)
#define	DARK_COLOR_LINE				RGB (80, 80, 80)

// Are Windows apps set to the dark theme, and does this version of Windows support dark controls (Windows 10 1809 or later)?
BOOL	IsDarkModeEnabled ();

// Let the process follow the Windows app theme: dark menus (the tray menu) when apps are dark. Call once at startup.
void	InitDarkMode ();

// Dark or normal title bar and control themes for a dialog. The dialog colours its background and texts itself (see CDarkTheme).
void	ApplyDarkMode (HWND hDialog, BOOL bDark);


// The dark theme of one dialog: follows the Windows app theme, and supplies the colours for WM_CTLCOLOR*
class CDarkTheme
{
public:
	CDarkTheme () : m_bDark (FALSE) {}

	BOOL	IsDark () const { return m_bDark; }

	// Read the Windows setting and apply it to the dialog and its controls. Returns TRUE if the theme changed.
	BOOL	Update (HWND hDialog);

	// The brush for a control, or NULL when not dark (use the default). bField: an edit-like control.
	HBRUSH	CtlColor (CDC* pDC, BOOL bField);

private:
	BOOL	m_bDark;
	CBrush	m_brBack;
	CBrush	m_brEdit;
};


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
