// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "DiskLED.h"
#include "MainFrm.h"
#include "Helpers.h"
#include "DialogAbout.h"
#include "DialogConfig.h"
#include <math.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////////////////////////////////////////////////////////
//
// CMainFrame
//
////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP

	ON_COMMAND(ID_POPUP_CONFIGURE,	ShowConfigurationDialog)
	ON_COMMAND(ID_POPUP_ABOUT,		ShowAboutDialog)
	ON_COMMAND(ID_POPUP_EXIT,		Shutdown)
	ON_MESSAGE(WM_ICON_NOTIFY,		OnTrayNotification)
	ON_MESSAGE(WM_POWERBROADCAST,	OnPowerBroadcast)
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
//
// CMainFrame construction/destruction
//
////////////////////////////////////////////////////////////////////////////////

CMainFrame::CMainFrame()
{
	m_TimerID				= 0;
	m_pTrayIcon2			= NULL;
	m_hIcon1				= NULL;
	m_hIcon2				= NULL;
	m_hQuery				= NULL;
	m_nCounters				= 1;
	m_nIconKey				= ICONKEY_NONE;
	m_nIconKey2				= ICONKEY_NONE;
	m_nLastTooltipTick		= 0;
	m_bPdhError				= FALSE;
	m_bErrorShown			= FALSE;
	m_nRetryTick			= 0;
	m_nLastLearnSaveTick	= GetTickCount ();
	m_nBadSamples			= 0;
	m_nZeroSince			= 0;

	SetDefaultSettings (m_oSettings);
	MakeIconStyle (m_oSettings, m_oStyle);
	SecureZeroMemory (m_aRuntime, sizeof (m_aRuntime));
	SecureZeroMemory (m_aIcon, sizeof (m_aIcon));
}

CMainFrame::~CMainFrame()
{
	if (m_pTrayIcon2)
	{
		m_pTrayIcon2->RemoveIcon ();
		delete m_pTrayIcon2;
		m_pTrayIcon2	= NULL;
	}

	// The icons are only released once the tray icons are gone
	m_TrayIcon.RemoveIcon ();
	if (m_hIcon1)
	{
		DestroyIcon (m_hIcon1);
		m_hIcon1	= NULL;
	}
	if (m_hIcon2)
	{
		DestroyIcon (m_hIcon2);
		m_hIcon2	= NULL;
	}
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) 
{	
    cs.style &= ~WS_VISIBLE;

	return CFrameWnd::PreCreateWindow(cs);
}

int CMainFrame::OnCreate (LPCREATESTRUCT lpCreateStruct) 
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	if (!m_TrayIcon.Create (this, WM_ICON_NOTIFY, PROGRAM_NAME, theApp.m_hIcon, IDR_POPUP_MENU))
	{
		return -1;
	}
	
	SetWindowText(theApp.m_strWindowName);

	// Read settings (from INI, if present)
	ReadSettings ();

	// Initialize counters and timer. Without a message box: the counters may not be ready right after logon, the timer keeps trying.
	Initialize (TRUE);

	return 0;
}

DWORD CMainFrame::StartTimer ()
{
    // Check if it's already going
	if (m_TimerID) 
	{
		// Re-initialize the timer
        KillTimer (m_TimerID);
		m_TimerID	= 0;
	}

	m_TimerID	= SetTimer (ID_TIMER, m_oSettings.nUpdateInterval, (TIMERPROC) NULL);

    if (! m_TimerID)
	{
		MessageBox (_T("Unable to set timer!"));
		return ERROR_INVALID_FUNCTION;
	}

	return ERROR_SUCCESS;
}



////////////////////////////////////////////////////////////////////////////////
//
//	Close all PDH handles and reset variables etc.
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::ClosePDH ()
{
	// Check if initialized
	if (m_hQuery == NULL)
	{
		// Not initialized
		return ERROR_SUCCESS;
	}

	PdhCloseQuery (m_hQuery);
	m_hQuery	= NULL;

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Log an error message
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::LogError (CString sCaller, CString sFailedComponent, DWORD nError, CString sHelpMessage)
{
	CString	sError;

	if (nError)
	{
		// Report an API error
		sError.Format (TEXT ("%s: %s failed with: 0x%x\n"), sCaller, sFailedComponent, nError);
	}
	else
	{
		// Report an internal (logic) error
		sError.Format (TEXT ("%s: %s\n"), sCaller, sFailedComponent);
	}

	// Add an optional explanatory help message
	if (!sHelpMessage.IsEmpty ())
	{
		sError	+= sHelpMessage;
	}

	// Display the message. The frame window is hidden and the config dialog may be open, so make sure the box is seen.
	MessageBox (sError, PROGRAM_NAME, MB_OK | MB_ICONERROR | MB_SETFOREGROUND | MB_TOPMOST);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Initialize PDH (performance monitor API)
//	With bSilent, errors are only stored in m_sLastError, otherwise they are also shown in a message box
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::InitPDH (BOOL bSilent)
{
	PDH_STATUS	nError	= ERROR_INVALID_FUNCTION;

	// Check if already initialized
	if (m_hQuery)
	{
		// Re-initialize
		ClosePDH ();
		m_hQuery	= NULL;
	}
		
	// Open a query handle
	nError	= PdhOpenQuery (NULL, 0, &m_hQuery);
	if(nError != ERROR_SUCCESS)
	{
		ClosePDH ();
		m_sLastError.Format (TEXT ("PdhOpenQuery failed with 0x%x"), nError);
		if (!bSilent)
		{
			LogError (TEXT ("InitPDH"), TEXT ("PdhOpenQuery"), nError);
		}
		return nError;
	}

	// Add the counters to the query
	for (int i = 0; i < m_nCounters; i++)
	{
		const CString&	sPath	= m_oSettings.aCounters[i].sPath;

		// Prefer the English path, which works on any system language. Older INI files contain paths in the system language.
		nError	= PdhAddEnglishCounter (m_hQuery, sPath, 0, &m_aRuntime[i].hCounter);
		if (nError != ERROR_SUCCESS)
		{
			nError	= PdhAddCounter (m_hQuery, sPath, 0, &m_aRuntime[i].hCounter);
		}
		if (nError != ERROR_SUCCESS)
		{
			CString	sExplanation;
			if (nError == PDH_CSTATUS_NO_OBJECT || nError == PDH_CSTATUS_NO_COUNTER || nError == PDH_CSTATUS_BAD_COUNTERNAME)
			{
				sExplanation	= TEXT ("This probably means that the performance counter path is invalid: <") + sPath + TEXT (">");
			}
			ClosePDH ();
			m_sLastError.Format (TEXT ("PdhAddCounter failed with 0x%x for %s"), nError, (LPCTSTR) sPath);
			if (!bSilent)
			{
				LogError (TEXT ("InitPDH"), (i == 0) ? TEXT ("PdhAddCounter") : TEXT ("PdhAddCounter (second counter)"), nError, sExplanation);
			}
			return nError;
		}

		// The auto scale cannot learn a maximum above the speed of the network adapter
		m_aRuntime[i].dCeiling	= 0;
		if (m_oSettings.aCounters[i].nScaleMode == SCALEMODE_AUTO)
		{
			double	dSpeed	= 0;

			if (GetNetworkLinkSpeed (sPath, dSpeed))
			{
				m_aRuntime[i].dCeiling	= dSpeed;
			}
		}
	}

	// Get initial counter value as counter requires difference between two PdhCollectQueryData
	nError	= PdhCollectQueryData (m_hQuery);
	if (nError != ERROR_SUCCESS && nError != PDH_NO_DATA)
	{
		ClosePDH ();
		m_sLastError.Format (TEXT ("PdhCollectQueryData failed with 0x%x"), nError);
		if (!bSilent)
		{
			LogError (TEXT ("InitPDH"), TEXT ("PdhCollectQueryData"), nError);
		}
		return nError;
	}

	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////
//
// CMainFrame message handlers
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::ShowConfigurationDialog ()
{
	// This is initialized to NULL
	static CDialog*	pdlgConfig;

	// Create the dialog object if necessary
	if (pdlgConfig == NULL)
	{
		pdlgConfig	= new CDialogConfig (this);
	}

	// Make sure the window is not already shown
	if (pdlgConfig->m_hWnd == NULL)
	{
		// Show the config dialog
		pdlgConfig->DoModal ();
		delete pdlgConfig;
		pdlgConfig	= NULL;
	}
	else
	{
		pdlgConfig->SetFocus ();
	}
}


void CMainFrame::ShowAboutDialog ()
{
	// This is initialized to NULL
	static CDialog*	pdlgAbout;

	// Create the dialog object if necessary
	if (pdlgAbout == NULL)
	{
		pdlgAbout	= new CDialogAbout ();
	}

	// Make sure the window is not already shown
	if (pdlgAbout->m_hWnd == NULL)
	{
		// Show the about dialog
		pdlgAbout->DoModal ();
		delete pdlgAbout;
		pdlgAbout	= NULL;
	}
	else
	{
		pdlgAbout->SetFocus ();
	}
}


void CMainFrame::Shutdown ()
{
	PostMessage (WM_CLOSE, 0, 0);
}


afx_msg LRESULT CMainFrame::OnPowerBroadcast(WPARAM wParam, LPARAM)
{
	// The performance query can be stale after standby/hibernation (adapters and disks come back)
	if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND)
	{
		Reconnect ();
	}

	return 0L;
}

LRESULT CMainFrame::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
	// Each tray icon ignores notifications for the other one
	LRESULT	nResult	= m_TrayIcon.OnTrayNotification (wParam, lParam);

	if (nResult == 0 && m_pTrayIcon2)
	{
		nResult	= m_pTrayIcon2->OnTrayNotification (wParam, lParam);
	}

	return nResult;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The name of a counter for tooltips and alerts: the last part of its path
//
////////////////////////////////////////////////////////////////////////////////

CString CMainFrame::GetCounterLabel (int nCounter)
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	if (!ParsePerfPath (m_oSettings.aCounters[nCounter].sPath, sMachine, sObject, sInstance, sCounter))
	{
		sCounter	= m_oSettings.aCounters[nCounter].sPath;
	}

	// Keep tooltips and balloons within their length limits
	sCounter.Truncate (48);

	return sCounter;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The tooltip of the tray icon that shows several counters: the current values, and the counter names
//
////////////////////////////////////////////////////////////////////////////////

CString CMainFrame::GetTooltipText ()
{
	CString	sText;

	for (int i = 0; i < m_nCounters; i++)
	{
		const CounterSettings&	oCounter	= m_oSettings.aCounters[i];
		CString					sValue		= FormatValue (oCounter.nValueFormat, m_aIcon[i].dValue, m_aIcon[i].dScale, oCounter.nScaleMode == SCALEMODE_FIXED);

		if (m_nCounters == 1)
		{
			sText	= sValue + TEXT ("\n") + PROGRAM_NAME + TEXT (" - ") + oCounter.sPath;
		}
		else
		{
			sText	+= GetCounterLabel (i) + TEXT (": ") + sValue + TEXT ("\n");
		}
	}

	if (m_nCounters > 1)
	{
		sText	+= PROGRAM_NAME;
	}

	return sText;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The tooltip of one counter's own tray icon: the current value and the counter path
//
////////////////////////////////////////////////////////////////////////////////

CString CMainFrame::GetCounterTooltip (int nCounter)
{
	const CounterSettings&	oCounter	= m_oSettings.aCounters[nCounter];

	return FormatValue (oCounter.nValueFormat, m_aIcon[nCounter].dValue, m_aIcon[nCounter].dScale, oCounter.nScaleMode == SCALEMODE_FIXED)
		+ TEXT ("\n") + PROGRAM_NAME + TEXT (" - ") + oCounter.sPath;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Show a balloon when a value has been above its alert threshold for the configured time
//	The balloon is shown once per period above the threshold. Short dips below the threshold do not end the period.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::CheckAlert (int nCounter)
{
	const CounterSettings&	oCounter	= m_oSettings.aCounters[nCounter];
	CounterRuntime&			oRuntime	= m_aRuntime[nCounter];
	double					dScale		= m_aIcon[nCounter].dScale;

	// The percentage is of the scale, the value that fills the icon. With the Peak, Decay and Auto scale, the 100% value is only a minimum.
	if (oCounter.nAlertPercent <= 0 || dScale <= 0)
	{
		return;
	}

	double	dPercent	= m_aIcon[nCounter].dValue * 100.0 / dScale;
	DWORD	nTick		= GetTickCount ();

	if (!(dPercent >= oCounter.nAlertPercent))
	{
		if (oRuntime.bAbove)
		{
			if (!oRuntime.bBelow)
			{
				oRuntime.bBelow			= TRUE;
				oRuntime.nBelowSince	= nTick;
			}
			else if (nTick - oRuntime.nBelowSince >= ALERT_DIP_MS)
			{
				// Back to normal: the next period above the threshold raises a new alert
				oRuntime.bAbove		= FALSE;
				oRuntime.bBelow		= FALSE;
				oRuntime.bAlerted	= FALSE;
			}
		}
		return;
	}

	oRuntime.bBelow	= FALSE;
	if (!oRuntime.bAbove)
	{
		oRuntime.bAbove			= TRUE;
		oRuntime.nAboveSince	= nTick;
	}

	if (!oRuntime.bAlerted && (ULONGLONG) (nTick - oRuntime.nAboveSince) >= (ULONGLONG) oCounter.nAlertSeconds * 1000)
	{
		CString	sText;

		oRuntime.bAlerted	= TRUE;
		sText.Format (TEXT ("%s: %s, above %d%% of the scale for %d s."), (LPCTSTR) GetCounterLabel (nCounter),
			(LPCTSTR) FormatValue (oCounter.nValueFormat, m_aIcon[nCounter].dValue, m_aIcon[nCounter].dScale, oCounter.nScaleMode == SCALEMODE_FIXED),
			oCounter.nAlertPercent, oCounter.nAlertSeconds);

		// The balloon text is limited to 255 characters
		sText.Truncate (255);
		m_TrayIcon.ShowBalloon (sText, PROGRAM_NAME, NIIF_WARNING, 10);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Set a new icon on a tray icon. The tray class keeps using the icon (it re-adds it when Explorer restarts),
//	so the previous icon is only destroyed once the new one is set.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetTrayIcon (CSystemTray& oTray, HICON& hCurrent, HICON hNew)
{
	if (hNew == NULL)
	{
		return;
	}

	oTray.SetIcon (hNew);
	if (hCurrent)
	{
		DestroyIcon (hCurrent);
	}
	hCurrent	= hNew;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The counters cannot be read: show an error icon and the problem in the tooltip. The timer keeps trying to reconnect.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::ShowErrorState (const CString& sMessage, BOOL bBalloon)
{
	int		nSize	= GetSystemMetrics (SM_CXSMICON);

	m_bPdhError		= TRUE;
	m_nRetryTick	= GetTickCount ();

	SetTrayIcon (m_TrayIcon, m_hIcon1, (HICON) LoadImage (AfxGetResourceHandle (), MAKEINTRESOURCE (IDI_ICON6), IMAGE_ICON, nSize, nSize, 0));
	m_TrayIcon.SetTooltipText (CString (PROGRAM_NAME) + TEXT (" - ") + sMessage);
	if (m_pTrayIcon2)
	{
		SetTrayIcon (*m_pTrayIcon2, m_hIcon2, (HICON) LoadImage (AfxGetResourceHandle (), MAKEINTRESOURCE (IDI_ICON6), IMAGE_ICON, nSize, nSize, 0));
		m_pTrayIcon2->SetTooltipText (CString (PROGRAM_NAME) + TEXT (" - ") + sMessage);
	}

	// Report the problem once per error period
	if (bBalloon && !m_bErrorShown)
	{
		CString	sText	= sMessage + TEXT (". DiskLED keeps trying.");

		sText.Truncate (255);
		m_bErrorShown	= TRUE;
		m_TrayIcon.ShowBalloon (sText, PROGRAM_NAME, NIIF_ERROR, 10);
	}
}


void CMainFrame::ClearErrorState ()
{
	m_bPdhError			= FALSE;
	m_bErrorShown		= FALSE;
	m_nBadSamples		= 0;
	m_nZeroSince		= 0;
	m_nIconKey			= ICONKEY_NONE;
	m_nIconKey2			= ICONKEY_NONE;
	m_nLastTooltipTick	= 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	A sample could not be used: tolerate a few, then assume the query is stale and open a new one
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::SkipSample ()
{
	if (++m_nBadSamples >= BAD_SAMPLES_MAX)
	{
		Reconnect ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Open a new query. The history is kept. On failure the error state is shown and the timer keeps retrying.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::Reconnect ()
{
	m_nBadSamples	= 0;
	m_nZeroSince	= 0;
	m_aRuntime[0].nLastTick	= 0;
	m_aRuntime[1].nLastTick	= 0;

	if (InitPDH (TRUE) != ERROR_SUCCESS)
	{
		ShowErrorState (m_sLastError, FALSE);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Keep the learned maximum of the auto scale in the INI file, so it survives a restart
//	Only an existing INI file is updated, and only when the value changed noticeably. Write errors are ignored.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::SaveLearnedMax (BOOL bForce)
{
	DWORD	nTick	= GetTickCount ();

	if (!bForce && nTick - m_nLastLearnSaveTick < LEARN_SAVE_MS)
	{
		return;
	}
	m_nLastLearnSaveTick	= nTick;

	if (m_sINIFile.IsEmpty () || !IsFileExistent (m_sINIFile))
	{
		return;
	}

	for (int i = 0; i < m_nCounters; i++)
	{
		CounterRuntime&	oRuntime	= m_aRuntime[i];

		if (m_oSettings.aCounters[i].nScaleMode != SCALEMODE_AUTO || !(oRuntime.dLearned > 0))
		{
			continue;
		}
		if (fabs (oRuntime.dLearned - oRuntime.dLearnedSaved) <= 0.02 * oRuntime.dLearned)
		{
			continue;
		}

		if (WriteINIValue (m_sINIFile, (i == 0) ? INI_COUNTER_1_SECTION : INI_COUNTER_2_SECTION, INI_VALUE_LEARNEDMAX, (double) oRuntime.dLearned) == ERROR_SUCCESS)
		{
			oRuntime.dLearnedSaved	= oRuntime.dLearned;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Reading the counters failed while running: stop the query, show the error and reconnect later
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::HandlePdhError (const CString& sCaller, DWORD nError)
{
	ClosePDH ();
	m_sLastError.Format (TEXT ("%s failed with 0x%x"), (LPCTSTR) sCaller, nError);
	ShowErrorState (m_sLastError, TRUE);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Timer: read the counters and update icon and tooltip
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnTimer (UINT_PTR nIDEvent) 
{
	PDH_STATUS	nError	= ERROR_INVALID_FUNCTION;
	DWORD		nTick	= GetTickCount ();

	if (m_hQuery == NULL)
	{
		// Not connected. After an error, try again from time to time.
		if (m_bPdhError && nTick - m_nRetryTick >= PDH_RETRY_MS)
		{
			m_nRetryTick	= nTick;
			if (InitPDH (TRUE) == ERROR_SUCCESS)
			{
				ClearErrorState ();
			}
		}
		return;
	}

	// Get current data
	nError	= PdhCollectQueryData (m_hQuery);
	if (nError != ERROR_SUCCESS)
	{
		HandlePdhError (TEXT ("PdhCollectQueryData"), nError);
		return;
	}

	BOOL	bAllZero	= TRUE;

	for (int i = 0; i < m_nCounters; i++)
	{
		const CounterSettings&	oCounter	= m_oSettings.aCounters[i];
		CounterRuntime&			oRuntime	= m_aRuntime[i];
		IconCounter&			oIcon		= m_aIcon[i];
		PDH_FMT_COUNTERVALUE	oValue;

		// Get the counter data as floating point number. Percentage counters are not capped at 100 (e.g. a process using several cores).
		nError	= PdhGetFormattedCounterValue (oRuntime.hCounter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, 0, &oValue);
		if (nError != ERROR_SUCCESS)
		{
			// Let's have a look at the error message
			if (nError == PDH_CALC_NEGATIVE_VALUE || nError == PDH_CALC_NEGATIVE_DENOMINATOR || nError == PDH_CALC_NEGATIVE_TIMEBASE)
			{
				// This (PDH_CALC_NEGATIVE_VALUE) happens on Windows 7 RTM if a USB flash drive is inserted.
				// The algorithm probably has a bug in that it returns a negative value because of the additional disk which cannot be converted to LONG.
				// A single such sample is ignored, a persistent one means the query is stale.
				SkipSample ();
				return;
			}
			else
			{
				HandlePdhError (TEXT ("PdhGetFormattedCounterValue"), nError);
				return;
			}
		}

		// The call can succeed with a value that is not valid (e.g. the network adapter instance is gone)
		if (oValue.CStatus != PDH_CSTATUS_VALID_DATA && oValue.CStatus != PDH_CSTATUS_NEW_DATA)
		{
			SkipSample ();
			return;
		}

		double	dValue	= oValue.doubleValue;
		if (oCounter.bInvert)
		{
			dValue	= oCounter.d100Percent - dValue;
		}
		if (!(dValue > 0))
		{
			// Also catches NaN
			dValue	= 0;
		}
		if (dValue > 0)
		{
			bAllZero	= FALSE;
		}

		// Remember the sample (oldest first)
		memmove (oIcon.adHistory, oIcon.adHistory + 1, (HISTORY_SIZE - 1) * sizeof (double));
		oIcon.adHistory[HISTORY_SIZE - 1]	= dValue;
		oIcon.dValue						= dValue;
		oIcon.nValueFormat					= oCounter.nValueFormat;

		// The scale is the value that corresponds to the full icon. The 100% value is the minimum.
		double	dScale	= oCounter.d100Percent;
		if (oCounter.nScaleMode == SCALEMODE_PEAK)
		{
			for (int j = 0; j < HISTORY_SIZE; j++)
			{
				if (oIcon.adHistory[j] > dScale)
				{
					dScale	= oIcon.adHistory[j];
				}
			}
		}
		else if (oCounter.nScaleMode == SCALEMODE_DECAY)
		{
			// The peak falls by half every DECAY_HALFLIFE_MS (measured, the timer is not exact), a higher value becomes the new peak
			DWORD	nElapsed	= (oRuntime.nLastTick != 0) ? nTick - oRuntime.nLastTick : m_oSettings.nUpdateInterval;
			if (nElapsed > 60000)
			{
				nElapsed	= 60000;
			}

			double	dDecayed	= oRuntime.dDecayPeak * pow (0.5, (double) nElapsed / DECAY_HALFLIFE_MS);
			oRuntime.dDecayPeak	= (dValue > dDecayed) ? dValue : dDecayed;
			if (oRuntime.dDecayPeak > dScale)
			{
				dScale	= oRuntime.dDecayPeak;
			}
		}
		else if (oCounter.nScaleMode == SCALEMODE_AUTO)
		{
			// The learned maximum rises with every new high and falls very slowly. Nothing above the adapter speed can be real.
			DWORD	nElapsed	= (oRuntime.nLastTick != 0) ? nTick - oRuntime.nLastTick : m_oSettings.nUpdateInterval;
			if (nElapsed > 60000)
			{
				nElapsed	= 60000;
			}

			double	dLearned	= oRuntime.dLearned * pow (0.5, (double) nElapsed / LEARN_HALFLIFE_MS);
			double	dSample		= dValue;
			if (oRuntime.dCeiling > 0)
			{
				dSample		= (dSample > oRuntime.dCeiling) ? oRuntime.dCeiling : dSample;
				dLearned	= (dLearned > oRuntime.dCeiling) ? oRuntime.dCeiling : dLearned;
			}
			if (dSample > dLearned)
			{
				dLearned	= dSample;
			}

			oRuntime.dLearned						= dLearned;
			m_oSettings.aCounters[i].dLearnedMax	= dLearned;
			if (dLearned > dScale)
			{
				dScale	= dLearned;
			}
		}
		oRuntime.nLastTick	= nTick;
		oIcon.dScale		= dScale;

		CheckAlert (i);
	}

	// A usable sample
	m_nBadSamples	= 0;

	// Counters that read zero for a long time may be stale: open a new query
	if (!bAllZero)
	{
		m_nZeroSince	= 0;
	}
	else if (m_nZeroSince == 0)
	{
		m_nZeroSince	= nTick;
	}
	else if (nTick - m_nZeroSince >= STALE_ZERO_MS)
	{
		Reconnect ();
		if (m_hQuery == NULL)
		{
			return;
		}
	}

	// Update the icon(s). Icons are only redrawn when what they show changes.
	UpdateTrayIcons ();

	// Show the current value in the tooltip (throttled, the shell does not need to be updated on every tick)
	if (nTick - m_nLastTooltipTick >= TOOLTIP_UPDATE_MS)
	{
		m_nLastTooltipTick	= nTick;
		UpdateTooltips ();
	}

	SaveLearnedMax (FALSE);

	// Call the parent class's function
	CFrameWnd::OnTimer (nIDEvent);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Update the tray icon(s)
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateTrayIcons ()
{
	int		nSize	= GetSystemMetrics (SM_CXSMICON);

	if (m_pTrayIcon2)
	{
		// One icon per counter
		DWORD	nKey1	= GetIconKey (m_oStyle, &m_aIcon[0], 1, nSize);
		DWORD	nKey2	= GetIconKey (m_oStyle, &m_aIcon[1], 1, nSize);

		if (nKey1 != m_nIconKey)
		{
			SetTrayIcon (m_TrayIcon, m_hIcon1, CreateTrayIconImage (m_oStyle, &m_aIcon[0], 1));
			m_nIconKey	= nKey1;
		}
		if (nKey2 != m_nIconKey2)
		{
			SetTrayIcon (*m_pTrayIcon2, m_hIcon2, CreateTrayIconImage (m_oStyle, &m_aIcon[1], 1));
			m_nIconKey2	= nKey2;
		}
		return;
	}

	DWORD	nKey	= GetIconKey (m_oStyle, m_aIcon, m_nCounters, nSize);
	if (nKey != m_nIconKey)
	{
		SetTrayIcon (m_TrayIcon, m_hIcon1, CreateTrayIconImage (m_oStyle, m_aIcon, m_nCounters));
		m_nIconKey	= nKey;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Update the tooltip(s) with the current values
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateTooltips ()
{
	if (m_pTrayIcon2)
	{
		m_TrayIcon.SetTooltipText (GetCounterTooltip (0));
		m_pTrayIcon2->SetTooltipText (GetCounterTooltip (1));
	}
	else
	{
		m_TrayIcon.SetTooltipText (GetTooltipText ());
	}
}


void CMainFrame::OnClose() 
{
	SaveLearnedMax (TRUE);

	// Clean up the PDH variables
	if (m_hQuery)
	{
		PdhCloseQuery (m_hQuery);
		m_hQuery	= NULL;
	}

	m_TrayIcon.RemoveIcon();
	if (m_pTrayIcon2)
	{
		m_pTrayIcon2->RemoveIcon ();
	}
	
	// Call the parent class's function
	CFrameWnd::OnClose();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read settings (from INI or potentially other sources)
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::ReadSettings ()
{
	DWORD	nError	= ERROR_INVALID_FUNCTION;

	// Determine and store the path to our executable and the installation directory
	nError	=  GetModuleFileName(NULL, m_sExecutablePath.GetBuffer (MAX_PATH), MAX_PATH);
	m_sExecutablePath.ReleaseBuffer ();
	if (nError == 0)
	{
		return ERROR_INVALID_FUNCTION;
	}
	m_sInstallationDirectory	=	m_sExecutablePath.Left (m_sExecutablePath.ReverseFind('\\'));

	// Build the path to the INI file
	m_sINIFile	= m_sInstallationDirectory + TEXT ("\\") + INI_SETTINGS_FILE;

	return LoadSettings (m_sINIFile, m_oSettings);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Initialize the performance counters and the timer
//	If the counters cannot be read, the timer keeps running and tries again
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::Initialize (BOOL bSilent)
{
	DWORD	nError	= ERROR_INVALID_FUNCTION;

	NormalizeSettings (m_oSettings);
	MakeIconStyle (m_oSettings, m_oStyle);
	m_nCounters	= m_oSettings.bSecondCounter ? 2 : 1;

	// The number icon gets its own tray icon for each counter
	if (UsesSeparateIcons (m_oSettings.nIconType, m_nCounters))
	{
		if (m_pTrayIcon2 == NULL)
		{
			m_pTrayIcon2	= new CSystemTray;
			if (!m_pTrayIcon2->Create (this, WM_ICON_NOTIFY, PROGRAM_NAME, theApp.m_hIcon, IDR_POPUP_MENU2))
			{
				delete m_pTrayIcon2;
				m_pTrayIcon2	= NULL;
			}
		}

		// Without a second tray icon only the first counter is shown, two numbers do not fit in one icon
		if (m_pTrayIcon2 == NULL)
		{
			m_nCounters	= 1;
		}
	}
	else if (m_pTrayIcon2)
	{
		m_pTrayIcon2->RemoveIcon ();
		delete m_pTrayIcon2;
		m_pTrayIcon2	= NULL;
		if (m_hIcon2)
		{
			DestroyIcon (m_hIcon2);
			m_hIcon2	= NULL;
		}
	}

	// Start without history
	for (int i = 0; i < 2; i++)
	{
		SecureZeroMemory (&m_aRuntime[i], sizeof (m_aRuntime[i]));
		SecureZeroMemory (&m_aIcon[i], sizeof (m_aIcon[i]));
		m_aIcon[i].dScale		= m_oSettings.aCounters[i].d100Percent;
		m_aIcon[i].nValueFormat	= m_oSettings.aCounters[i].nValueFormat;
		m_aRuntime[i].dLearned		= m_oSettings.aCounters[i].dLearnedMax;
		m_aRuntime[i].dLearnedSaved	= m_oSettings.aCounters[i].dLearnedMax;
		if (m_oSettings.aCounters[i].nScaleMode == SCALEMODE_AUTO && m_aRuntime[i].dLearned > m_aIcon[i].dScale)
		{
			m_aIcon[i].dScale	= m_aRuntime[i].dLearned;
		}
	}
	ClearErrorState ();

	// Init PDH
	nError	= InitPDH (bSilent);
	if (nError != ERROR_SUCCESS)
	{
		ShowErrorState (m_sLastError, bSilent);

		// Keep the timer running: it retries
		StartTimer ();

		return nError;
	}

	// Start the timer
	nError	= StartTimer ();

	// Set a new info tip including the counter path(s)
	if (m_pTrayIcon2)
	{
		m_TrayIcon.SetTooltipText (CString (PROGRAM_NAME) + TEXT (" - ") + m_oSettings.aCounters[0].sPath);
		m_pTrayIcon2->SetTooltipText (CString (PROGRAM_NAME) + TEXT (" - ") + m_oSettings.aCounters[1].sPath);
		return nError;
	}

	CString	sToolTipText	= PROGRAM_NAME;
	sToolTipText			+= TEXT (" - ") + ((m_nCounters == 1) ? m_oSettings.aCounters[0].sPath : GetCounterLabel (0) + TEXT (" / ") + GetCounterLabel (1));
	m_TrayIcon.SetTooltipText (sToolTipText);

	return nError;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The current settings
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::GetSettings (DiskLEDSettings& oSettings)
{
	oSettings	= m_oSettings;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Use new settings. If they do not work, the previous settings are restored.
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::ApplySettings (const DiskLEDSettings& oSettings)
{
	DiskLEDSettings	oPrevious	= m_oSettings;

	m_oSettings	= oSettings;

	// The dialog works on a copy: do not lose what was learned meanwhile
	for (int i = 0; i < 2; i++)
	{
		if (oSettings.aCounters[i].sPath.CompareNoCase (oPrevious.aCounters[i].sPath) == 0 && oPrevious.aCounters[i].dLearnedMax > m_oSettings.aCounters[i].dLearnedMax)
		{
			m_oSettings.aCounters[i].dLearnedMax	= oPrevious.aCounters[i].dLearnedMax;
		}
	}

	DWORD	nError	= Initialize (FALSE);
	if (nError != ERROR_SUCCESS)
	{
		m_oSettings	= oPrevious;
		Initialize (TRUE);
	}

	return nError;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Get the path to the INI file
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::GetINIFile (CString& sINIFile)
{
	sINIFile	= m_sINIFile;
}
