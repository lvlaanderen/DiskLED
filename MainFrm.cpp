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
	m_hQuery				= NULL;
	m_nCounters				= 1;
	m_nIconKey				= ICONKEY_NONE;
	m_nLastTooltipTick		= 0;

	SetDefaultSettings (m_oSettings);
	MakeIconStyle (m_oSettings, m_oStyle);
	SecureZeroMemory (m_aRuntime, sizeof (m_aRuntime));
	SecureZeroMemory (m_aIcon, sizeof (m_aIcon));
}

CMainFrame::~CMainFrame()
{
	if (m_pTrayIcon2)
	{
		delete m_pTrayIcon2;
		m_pTrayIcon2	= NULL;
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

	// Initialize counters and timer
	Initialize ();

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

	// Display the message
	MessageBox (sError);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Initialize PDH (performance monitor API)
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::InitPDH ()
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
		LogError (TEXT ("InitPDH"), TEXT ("PdhOpenQuery"), nError);
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
			LogError (TEXT ("InitPDH"), (i == 0) ? TEXT ("PdhAddCounter") : TEXT ("PdhAddCounter (second counter)"), nError, sExplanation);
			return nError;
		}
	}

	// Get initial counter value as counter requires difference between two PdhCollectQueryData
	nError	= PdhCollectQueryData (m_hQuery);
	if (nError != ERROR_SUCCESS && nError != PDH_NO_DATA)
	{
		ClosePDH ();
		LogError (TEXT ("InitPDH"), TEXT ("PdhCollectQueryData"), nError);
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


afx_msg LRESULT CMainFrame::OnPowerBroadcast(WPARAM, LPARAM)
{
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
		return m_oSettings.aCounters[nCounter].sPath;
	}

	return sCounter;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The tooltip: the current value(s), and the counter path or name(s)
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
//	Show a balloon when a value has been above its alert threshold for the configured time
//	The balloon is shown once per period above the threshold
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::CheckAlert (int nCounter)
{
	const CounterSettings&	oCounter	= m_oSettings.aCounters[nCounter];
	CounterRuntime&			oRuntime	= m_aRuntime[nCounter];

	if (oCounter.nAlertPercent <= 0 || oCounter.d100Percent <= 0)
	{
		return;
	}

	double	dPercent	= m_aIcon[nCounter].dValue * 100.0 / oCounter.d100Percent;
	DWORD	nTick		= GetTickCount ();

	if (dPercent < oCounter.nAlertPercent)
	{
		// Back to normal: the next period above the threshold raises a new alert
		oRuntime.bAbove		= FALSE;
		oRuntime.bAlerted	= FALSE;
		return;
	}

	if (!oRuntime.bAbove)
	{
		oRuntime.bAbove			= TRUE;
		oRuntime.nAboveSince	= nTick;
	}

	if (!oRuntime.bAlerted && nTick - oRuntime.nAboveSince >= (DWORD) oCounter.nAlertSeconds * 1000)
	{
		CString	sText;

		oRuntime.bAlerted	= TRUE;
		sText.Format (TEXT ("%s: %s, above %d%% of the maximum for %d s."), (LPCTSTR) GetCounterLabel (nCounter),
			(LPCTSTR) FormatValue (oCounter.nValueFormat, m_aIcon[nCounter].dValue, m_aIcon[nCounter].dScale, oCounter.nScaleMode == SCALEMODE_FIXED),
			oCounter.nAlertPercent, oCounter.nAlertSeconds);
		m_TrayIcon.ShowBalloon (sText, PROGRAM_NAME, NIIF_WARNING, 10);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Timer: read the counters and update icon and tooltip
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnTimer (UINT_PTR nIDEvent) 
{
	PDH_STATUS	nError	= ERROR_INVALID_FUNCTION;

	if (m_hQuery == NULL)
	{
		// Not initialized
		return;
	}

	// Get current data
	nError	= PdhCollectQueryData (m_hQuery);
	if (nError != ERROR_SUCCESS)
	{
		ClosePDH ();

		CString	sExplanation;
		if (nError == PDH_NO_DATA)
		{
			sExplanation	= TEXT ("PDH_NO_DATA - No data is available for this performance counter.");
		}
		LogError (TEXT ("OnTimer"), TEXT ("PdhCollectQueryData"), nError, sExplanation);
		return;
	}

	for (int i = 0; i < m_nCounters; i++)
	{
		const CounterSettings&	oCounter	= m_oSettings.aCounters[i];
		CounterRuntime&			oRuntime	= m_aRuntime[i];
		IconCounter&			oIcon		= m_aIcon[i];
		PDH_FMT_COUNTERVALUE	oValue;

		// Get the counter data as floating point number
		nError	= PdhGetFormattedCounterValue (oRuntime.hCounter, PDH_FMT_DOUBLE, 0, &oValue);
		if (nError != ERROR_SUCCESS)
		{
			// Let's have a look at the error message
			if (nError == PDH_CALC_NEGATIVE_VALUE || nError == PDH_CALC_NEGATIVE_DENOMINATOR || nError == PDH_CALC_NEGATIVE_TIMEBASE)
			{
				// This (PDH_CALC_NEGATIVE_VALUE) happens on Windows 7 RTM if a USB flash drive is inserted.
				// The algorithm probably has a bug in that it returns a negative value because of the additional disk which cannot be converted to LONG.
				// Ignore this and do nothing.
				return;
			}
			else
			{
				ClosePDH ();
				LogError (TEXT ("OnTimer"), TEXT ("PdhGetFormattedCounterValue"), nError);
				return;
			}
		}

		double	dValue	= oValue.doubleValue;
		if (oCounter.bInvert)
		{
			dValue	= oCounter.d100Percent - dValue;
		}
		if (dValue < 0)
		{
			dValue	= 0;
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
			// The peak falls by half every DECAY_HALFLIFE_MS, a higher value becomes the new peak
			double	dDecayed	= oRuntime.dDecayPeak * pow (0.5, (double) m_oSettings.nUpdateInterval / DECAY_HALFLIFE_MS);
			oRuntime.dDecayPeak	= (dValue > dDecayed) ? dValue : dDecayed;
			if (oRuntime.dDecayPeak > dScale)
			{
				dScale	= oRuntime.dDecayPeak;
			}
		}
		oIcon.dScale	= dScale;

		CheckAlert (i);
	}

	// Update the icon. Some icon types only change in a few steps and are redrawn when the step changes.
	UpdateTrayIcons ();

	// Show the current value in the tooltip (throttled, the shell does not need to be updated on every tick)
	DWORD	nTick	= GetTickCount ();
	if (nTick - m_nLastTooltipTick >= TOOLTIP_UPDATE_MS)
	{
		m_nLastTooltipTick	= nTick;
		UpdateTooltips ();
	}

	// Call the parent class's function
	CFrameWnd::OnTimer (nIDEvent);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Update the tray icon(s). Some icon types only change in a few steps and are redrawn when the step changes.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateTrayIcons ()
{
	if (m_pTrayIcon2)
	{
		// One icon per counter
		HICON	hIcon1	= CreateTrayIconImage (m_oStyle, &m_aIcon[0], 1);
		HICON	hIcon2	= CreateTrayIconImage (m_oStyle, &m_aIcon[1], 1);

		if (hIcon1)
		{
			m_TrayIcon.SetIcon (hIcon1);
			DestroyIcon (hIcon1);
		}
		if (hIcon2)
		{
			m_pTrayIcon2->SetIcon (hIcon2);
			DestroyIcon (hIcon2);
		}
		return;
	}

	DWORD	nIconKey	= GetIconKey (m_oStyle, m_aIcon, m_nCounters, GetSystemMetrics (SM_CXSMICON));
	if (nIconKey == ICONKEY_ALWAYS || nIconKey != m_nIconKey)
	{
		HICON	hIcon	= CreateTrayIconImage (m_oStyle, m_aIcon, m_nCounters);
		if (hIcon)
		{
			m_TrayIcon.SetIcon (hIcon);
			DestroyIcon (hIcon);
			m_nIconKey	= nIconKey;
		}
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
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::Initialize ()
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
	}
	else if (m_pTrayIcon2)
	{
		m_pTrayIcon2->RemoveIcon ();
		delete m_pTrayIcon2;
		m_pTrayIcon2	= NULL;
	}

	// Start without history
	for (int i = 0; i < 2; i++)
	{
		SecureZeroMemory (&m_aRuntime[i], sizeof (m_aRuntime[i]));
		SecureZeroMemory (&m_aIcon[i], sizeof (m_aIcon[i]));
		m_aIcon[i].dScale		= m_oSettings.aCounters[i].d100Percent;
		m_aIcon[i].nValueFormat	= m_oSettings.aCounters[i].nValueFormat;
	}
	m_nIconKey	= ICONKEY_NONE;

	// Init PDH
	nError	= InitPDH ();
	if (nError != ERROR_SUCCESS)
	{
		KillTimer (m_TimerID);
		m_TimerID	= 0;

		// Set an error icon
		m_TrayIcon.SetIcon (IDI_ICON6);
		if (m_pTrayIcon2)
		{
			m_pTrayIcon2->SetIcon (IDI_ICON6);
		}

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
//	Use new settings
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::ApplySettings (const DiskLEDSettings& oSettings)
{
	m_oSettings	= oSettings;

	return Initialize ();
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
