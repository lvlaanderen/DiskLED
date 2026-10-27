#pragma once

#include <pdh.h>
#include <PDHMsg.h>
#include "Settings.h"
#include "IconRenderer.h"


#pragma comment(lib, "pdh.lib")

// The name of the program
#define PROGRAM_NAME				TEXT ("DiskLED")

// Minimum time between two tooltip updates
#define	TOOLTIP_UPDATE_MS			500

// Time between attempts to reconnect to the counters after an error
#define	PDH_RETRY_MS				5000

// A value that dips below the alert threshold for less than this long does not restart the alert timer
#define	ALERT_DIP_MS				3000

// Number of consecutive unusable samples after which the counters are reconnected
#define	BAD_SAMPLES_MAX				5

// All counters reading exactly zero for this long is treated as a possibly stale query (e.g. after a network adapter change), the counters are reconnected
#define	STALE_ZERO_MS				60000


// Runtime state of one performance counter
struct CounterRuntime
{
	PDH_HCOUNTER	hCounter;
	double			dDecayPeak;			// Decaying peak of the values
	DWORD			nLastTick;			// Tick count of the previous update, 0 = none yet
	double			dLearned;			// Learned maximum of the values (auto scale)
	double			dLearnedSaved;		// The learned maximum as last written to the INI file
	double			dCeiling;			// Highest possible value (network adapter speed), 0 = unknown
	BOOL			bAbove;				// The value is above the alert threshold
	DWORD			nAboveSince;		// Tick count when it went above
	BOOL			bBelow;				// The value has dropped below the threshold while the alert period was running
	DWORD			nBelowSince;		// Tick count when it dropped below
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
	HICON					m_hIcon1;				// The icons currently set on the tray icons. The tray class keeps using them, so they are owned here.
	HICON					m_hIcon2;
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
	DWORD					m_nIconKey;						// Identifies the icon currently shown (see GetIconKey)
	DWORD					m_nIconKey2;					// The same for the second tray icon
	DWORD					m_nLastTooltipTick;				// Tick count of the last tooltip update

	BOOL					m_bPdhError;					// The counters could not be read, reconnecting every PDH_RETRY_MS
	BOOL					m_bErrorShown;					// The error was already reported with a balloon
	DWORD					m_nRetryTick;					// Tick count of the last (re)connection attempt
	DWORD					m_nLastLearnSaveTick;			// Tick count of the last check whether the learned maximum needs saving
	int						m_nBadSamples;					// Consecutive samples that could not be used
	DWORD					m_nZeroSince;					// Tick count since when all counters read zero, 0 = not all zero
	CString					m_sLastError;

	// Member functions
	void					ShowConfigurationDialog ();
	void					ShowAboutDialog ();
	void					Shutdown ();
	DWORD					Initialize (BOOL bSilent);
	DWORD					StartTimer ();
	DWORD					InitPDH (BOOL bSilent);
	DWORD					ClosePDH ();
	DWORD					ReadSettings ();
	CString					GetCounterLabel (int nCounter);
	CString					GetTooltipText ();
	CString					GetCounterTooltip (int nCounter);
	void					UpdateTrayIcons ();
	void					UpdateTooltips ();
	void					SetTrayIcon (CSystemTray& oTray, HICON& hCurrent, HICON hNew);
	void					ShowErrorState (const CString& sMessage, BOOL bBalloon);
	void					ClearErrorState ();
	void					HandlePdhError (const CString& sCaller, DWORD nError);
	void					SkipSample ();
	void					Reconnect ();
	void					SaveLearnedMax (BOOL bForce);
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
