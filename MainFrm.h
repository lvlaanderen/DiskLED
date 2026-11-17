#pragma once

#include <pdh.h>
#include <PDHMsg.h>
#include <vector>
#include "Settings.h"
#include "Meter.h"


#pragma comment(lib, "pdh.lib")

// The name of the program
#define PROGRAM_NAME				TEXT ("DiskLED")

// Minimum time between two tooltip updates of a meter
#define	TOOLTIP_UPDATE_MS			500

// The timer wakes up at least this often (retries after errors, failed tray icons), even when no meter is due
#define	TIMER_MAX_WAIT_MS			500

// Time between attempts to reconnect to the counters after an error
#define	PDH_RETRY_MS				5000

// Number of consecutive unusable samples after which the counters are reconnected (at most once per PDH_RETRY_MS)
#define	BAD_SAMPLES_MAX				5

// All counters reading exactly zero for this long is treated as a possibly stale query (e.g. after a network adapter change), the counters are reconnected
#define	STALE_ZERO_MS				60000

// Posted to the main window when DiskLED starts without settings: look for older copies to import
#define	WM_FIRST_RUN				(WM_APP + 11)


// The main window (hidden). It owns the meters, the one PDH query that all their counters are in, and the timer.
// The timer is set to the next meter that is due; the query is collected then and every due meter reads its counter.
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
	std::vector<CMeter*>	m_aMeters;
	UINT_PTR				m_TimerID;				// Timer ID
	PDH_HQUERY				m_hQuery;
	CString					m_sINIFile;

	BOOL					m_bPdhError;					// The query could not be opened or read, reconnecting every PDH_RETRY_MS
	DWORD					m_nRetryTick;					// Tick count of the last (re)connection attempt after an error
	DWORD					m_nMeterRetryTick;				// Tick count of the last attempt to add the counters of meters that failed
	DWORD					m_nReconnectTick;				// Tick count of the last Reconnect
	DWORD					m_nLastLearnSaveTick;			// Tick count of the last check whether the learned maximum needs saving
	DWORD					m_nZeroSince;					// Tick count since when all counters read zero, 0 = not all zero
	BOOL					m_bFirstRun;					// There were no settings yet when DiskLED started
	CString					m_sLastError;

	// Member functions
	void					ShowConfigurationDialog ();
	void					OpenSettings (BOOL bImport);
	void					ShowAboutDialog ();
	void					Shutdown ();
	DWORD					Initialize (const DiskLEDSettings& oSettings, BOOL bSilent);
	DWORD					StartTimer ();
	DWORD					ScheduleTimer (DWORD nDelay);
	void					ScheduleNextSample ();
	void					Sample (DWORD nTick);
	DWORD					InitPDH (BOOL bSilent);
	DWORD					ClosePDH ();
	DWORD					ReadSettings (DiskLEDSettings& oSettings);
	void					UpdateMeters (DWORD nTick);
	void					ShowErrorState (const CString& sMessage, BOOL bBalloon);
	void					ClearErrorState ();
	void					HandlePdhError (const CString& sCaller, DWORD nError);
	void					Reconnect ();
	void					RetryFailedMeters ();
	void					SaveLearnedMax (BOOL bForce);



// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnClose();
	//}}AFX_MSG
	afx_msg LRESULT OnPowerBroadcast(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnShowSettings(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnFirstRun(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
