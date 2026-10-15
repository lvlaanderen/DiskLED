
#include <pdh.h>
#include <PDHMsg.h>
#include "Settings.h"
#include "IconRenderer.h"


#pragma comment(lib, "pdh.lib")

// The name of the program
#define PROGRAM_NAME				TEXT ("DiskLED")

// Minimum time between two tooltip updates
#define	TOOLTIP_UPDATE_MS			500


// Runtime state of one performance counter
struct CounterRuntime
{
	PDH_HCOUNTER	hCounter;
	double			dDecayPeak;			// Decaying peak of the values
	BOOL			bAbove;				// The value is above the alert threshold
	DWORD			nAboveSince;		// Tick count when it went above
	BOOL			bAlerted;			// The alert was shown for this period above the threshold
};


class CMainFrame : public CFrameWnd
{
// Operations
public:
	CMainFrame();
	virtual ~CMainFrame();

	DECLARE_DYNCREATE(CMainFrame)

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	// Member functions
	void	GetSettings (DiskLEDSettings& oSettings);
	DWORD	ApplySettings (const DiskLEDSettings& oSettings);
	void	GetINIFile (CString& sINIFile);
	void	LogError (CString sCaller, CString sFailedComponent, DWORD nError, CString sHelpMessage = TEXT(""));
	
private:

	// Member variables
	CSystemTray				m_TrayIcon;
	CSystemTray*			m_pTrayIcon2;			// Second tray icon, only with separate icons for two counters
	CMenu					m_Menu;					// popup menu
	UINT_PTR				m_TimerID;				// Timer ID
	PDH_HQUERY				m_hQuery;
	CString					m_sInstallationDirectory;
	CString					m_sExecutablePath;
	CString					m_sINIFile;

	DiskLEDSettings			m_oSettings;
	IconStyle				m_oStyle;
	int						m_nCounters;					// 1 or 2
	CounterRuntime			m_aRuntime[2];
	IconCounter				m_aIcon[2];
	DWORD					m_nIconKey;						// Identifies the icon currently shown (see ICONKEY_*)
	DWORD					m_nLastTooltipTick;				// Tick count of the last tooltip update

	// Member functions
	void					ShowConfigurationDialog ();
	void					ShowAboutDialog ();
	void					Shutdown ();
	DWORD					Initialize ();
	DWORD					StartTimer ();
	DWORD					InitPDH ();
	DWORD					ClosePDH ();
	DWORD					ReadSettings ();
	CString					GetCounterLabel (int nCounter);
	CString					GetTooltipText ();
	CString					GetCounterTooltip (int nCounter);
	void					UpdateTrayIcons ();
	void					UpdateTooltips ();
	void					CheckAlert (int nCounter);



// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg LRESULT OnPowerBroadcast(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
