// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "DiskLED.h"
#include "MainFrm.h"
#include "Helpers.h"
#include "DialogAbout.h"
#include "DialogConfig.h"
#include "DialogMessage.h"
#include "Import.h"
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
	ON_REGISTERED_MESSAGE(g_nMsgShowSettings, OnShowSettings)
	ON_MESSAGE(WM_FIRST_RUN,		OnFirstRun)
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////////////////
//
// CMainFrame construction/destruction
//
////////////////////////////////////////////////////////////////////////////////

CMainFrame::CMainFrame()
{
	m_TimerID				= 0;
	m_hQuery				= NULL;
	m_bPdhError				= FALSE;
	m_nRetryTick			= 0;
	m_nMeterRetryTick		= GetTickCount ();
	m_nReconnectTick		= GetTickCount () - PDH_RETRY_MS;
	m_nLastLearnSaveTick	= GetTickCount ();
	m_nZeroSince			= 0;
	m_bFirstRun				= FALSE;
}

CMainFrame::~CMainFrame()
{
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		delete m_aMeters[i];
	}
	m_aMeters.clear ();
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

	SetWindowText(theApp.m_strWindowName);

	// Read settings (from INI, if present)
	DiskLEDSettings	oSettings;

	ReadSettings (oSettings);

	// Initialize counters and timer. Without a message box: the counters may not be ready right after logon, the timer keeps trying.
	Initialize (oSettings, TRUE);

	// Without settings this is the first start: older copies of DiskLED can be offered for import, once the program is up
	if (m_bFirstRun)
	{
		PostMessage (WM_FIRST_RUN);
	}

	return 0;
}

// Start the timer: the meters are read (or start with their first raw value) right away
DWORD CMainFrame::StartTimer ()
{
	DWORD	nError	= ScheduleTimer (MIN_UPDATE_INTERVAL_MS);

	if (nError != ERROR_SUCCESS)
	{
		ShowMessage (this, TEXT ("Unable to set timer!"), PROGRAM_NAME, MB_OK | MB_ICONERROR);
	}

	return nError;
}

// Set the timer to fire once after nDelay ms. Setting it again replaces the previous time.
DWORD CMainFrame::ScheduleTimer (DWORD nDelay)
{
	m_TimerID	= SetTimer (ID_TIMER, nDelay, (TIMERPROC) NULL);

	return m_TimerID ? ERROR_SUCCESS : ERROR_INVALID_FUNCTION;
}

// The timer fires when the next meter is due, and at least every TIMER_MAX_WAIT_MS
void CMainFrame::ScheduleNextSample ()
{
	DWORD	nTick	= GetTickCount ();
	DWORD	nDelay	= TIMER_MAX_WAIT_MS;

	if (m_hQuery)
	{
		for (size_t i = 0; i < m_aMeters.size (); i++)
		{
			if (m_aMeters[i]->IsConnected ())
			{
				DWORD	nLeft	= m_aMeters[i]->GetTimeToNextSample (nTick);

				if (nLeft < nDelay)
				{
					nDelay	= nLeft;
				}
			}
		}
	}
	if (nDelay < MIN_UPDATE_INTERVAL_MS)
	{
		nDelay	= MIN_UPDATE_INTERVAL_MS;
	}

	// If this fails, the old timer is still there: it fires again (too early or too late), which tries again
	ScheduleTimer (nDelay);
}



////////////////////////////////////////////////////////////////////////////////
//
//	Close the PDH query (which frees all counters) and reset variables etc.
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::ClosePDH ()
{
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		m_aMeters[i]->Disconnect ();
	}

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

	// Display the message. The frame window is hidden and the config dialog may be open, the message makes sure it is seen.
	ShowMessage (this, sError, PROGRAM_NAME, MB_OK | MB_ICONERROR);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Initialize PDH (performance monitor API): open the query and add the counters of all meters
//	A counter that cannot be added only affects its own meter (error icon, retried later). The query itself failing affects all meters.
//	With bSilent, errors are only stored in m_sLastError, otherwise a failing counter is also shown in a message box and returned as error.
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
	DWORD	nFirstError	= ERROR_SUCCESS;
	CString	sFirstError;

	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		CMeter*	pMeter	= m_aMeters[i];
		CString	sError;

		nError	= pMeter->Connect (m_hQuery, sError);
		if (nError == ERROR_SUCCESS)
		{
			pMeter->ClearError ();
			continue;
		}

		if (nFirstError == ERROR_SUCCESS)
		{
			nFirstError	= nError;
			sFirstError	= sError;
		}
		pMeter->ShowError (sError, bSilent);
	}

	if (nFirstError != ERROR_SUCCESS && !bSilent)
	{
		ClosePDH ();
		m_sLastError	= sFirstError;
		LogError (TEXT ("InitPDH"), TEXT ("PdhAddCounter"), nFirstError, sFirstError);
		return nFirstError;
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
	OpenSettings (FALSE);
}

// bImport: the window opens with the wizard at the import of older copies
void CMainFrame::OpenSettings (BOOL bImport)
{
	// This is initialized to NULL
	static CDialogConfig*	pdlgConfig;

	// Create the dialog object if necessary
	if (pdlgConfig == NULL)
	{
		pdlgConfig	= new CDialogConfig (this);
		if (bImport)
		{
			pdlgConfig->StartWithImport ();
		}
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
		pdlgConfig->SetForegroundWindow ();
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

// DiskLED was started again: the running one shows its settings
LRESULT CMainFrame::OnShowSettings (WPARAM, LPARAM)
{
	PostMessage (WM_COMMAND, ID_POPUP_CONFIGURE, 0);

	return 0L;
}

// The first start, without settings: if older copies of DiskLED are around, offer to take over their meters
LRESULT CMainFrame::OnFirstRun (WPARAM, LPARAM)
{
	std::vector<OldCopy>	aCopies;

	m_bFirstRun	= FALSE;
	FindOldCopies (aCopies, m_sINIFile);

	// The first start is over, whatever the answer is: store the settings that are running, so this is not asked again
	DiskLEDSettings	oSettings;

	GetSettings (oSettings);
	SaveSettings (m_sINIFile, oSettings);

	if (aCopies.empty ())
	{
		return 0L;
	}

	BOOL	bSeveral	= (aCopies.size () > 1);
	CString	sText		= bSeveral ? TEXT ("DiskLED found older copies of itself on this computer:\n\n") : TEXT ("DiskLED found an older copy of itself on this computer:\n\n");

	for (size_t i = 0; i < aCopies.size () && i < 4; i++)
	{
		sText	+= aCopies[i].sIni.Left (aCopies[i].sIni.ReverseFind ('\\')) + TEXT ("\n");
	}
	if (aCopies.size () > 4)
	{
		sText	+= TEXT ("...\n");
	}
	sText	+= bSeveral ? TEXT ("\nDo you want to take over their meters? You can do this later with Add meter, in the settings window.")
		: TEXT ("\nDo you want to take over its meters? You can do this later with Add meter, in the settings window.");

	if (ShowChoice (this, sText, PROGRAM_NAME, IDI_INFORMATION, TEXT ("Import..."), TEXT ("Not now"), TEXT (""), 2) == 1)
	{
		OpenSettings (TRUE);
	}

	return 0L;
}

LRESULT CMainFrame::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
	// Each tray icon ignores notifications for the others
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		LRESULT	nResult	= m_aMeters[i]->OnTrayNotification (wParam, lParam);

		if (nResult != 0)
		{
			return nResult;
		}
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The query cannot be used: all meters show an error icon and the problem in the tooltip. The timer keeps trying to reconnect.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::ShowErrorState (const CString& sMessage, BOOL bBalloon)
{
	m_bPdhError		= TRUE;
	m_nRetryTick	= GetTickCount ();

	// One balloon is enough
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		m_aMeters[i]->ShowError (sMessage, bBalloon && i == 0);
	}
}


void CMainFrame::ClearErrorState ()
{
	m_bPdhError			= FALSE;
	m_nZeroSince		= 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Open a new query. The history is kept. On failure the error state is shown and the timer keeps retrying.
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::Reconnect ()
{
	m_nZeroSince	= 0;
	m_nReconnectTick	= GetTickCount ();
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		m_aMeters[i]->ResetSampling ();
	}

	if (InitPDH (TRUE) != ERROR_SUCCESS)
	{
		ShowErrorState (m_sLastError, FALSE);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Meters whose counter could not be added (e.g. a network adapter that is not there) are tried again from time to time,
//	without disturbing the other meters
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::RetryFailedMeters ()
{
	DWORD	nTick	= GetTickCount ();

	if (m_hQuery == NULL || nTick - m_nMeterRetryTick < PDH_RETRY_MS)
	{
		return;
	}
	m_nMeterRetryTick	= nTick;

	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		CMeter*	pMeter	= m_aMeters[i];
		CString	sError;

		if (!pMeter->IsConnected () && pMeter->Connect (m_hQuery, sError) == ERROR_SUCCESS)
		{
			pMeter->ClearError ();
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Keep the learned maximum of the auto scale in the INI file, so it survives a restart
//	Only an existing INI file is updated, only the section of a meter that was saved with this counter, and only when the value changed noticeably.
//	Write errors are ignored.
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

	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		CMeter*	pMeter	= m_aMeters[i];
		double	dLearned	= 0;
		CString	sSection	= GetMeterSection (pMeter->GetId ());
		CString	sPath;

		if (!pMeter->GetLearnedToSave (dLearned))
		{
			continue;
		}

		// The file may hold other settings than what is running now (changes that were not saved)
		if (ReadINIValue (m_sINIFile, sSection, INI_VALUE_PATH, &sPath) != ERROR_SUCCESS || sPath.CompareNoCase (pMeter->GetSettings ().oCounter.sPath) != 0)
		{
			continue;
		}

		if (WriteINIValue (m_sINIFile, sSection, INI_VALUE_LEARNEDMAX, dLearned) == ERROR_SUCCESS)
		{
			pMeter->SetLearnedSaved (dLearned);
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
//	Timer: read the meters that are due, update icons and tooltips, and set the timer to the next meter that is due
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnTimer (UINT_PTR nIDEvent) 
{
	if (nIDEvent == ID_TIMER)
	{
		Sample (GetTickCount ());
		ScheduleNextSample ();
	}

	// Call the parent class's function
	CFrameWnd::OnTimer (nIDEvent);
}

void CMainFrame::Sample (DWORD nTick)
{
	PDH_STATUS	nError	= ERROR_INVALID_FUNCTION;

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

	// The query is only collected when a meter needs a new value
	BOOL	bDue	= FALSE;

	for (size_t i = 0; i < m_aMeters.size () && !bDue; i++)
	{
		bDue	= (m_aMeters[i]->IsConnected () && m_aMeters[i]->GetTimeToNextSample (nTick) == 0);
	}

	if (bDue)
	{
		nError	= PdhCollectQueryData (m_hQuery);
		if (nError != ERROR_SUCCESS)
		{
			HandlePdhError (TEXT ("PdhCollectQueryData"), nError);
			return;
		}

		BOOL	bAnySample	= FALSE;
		BOOL	bStale		= FALSE;

		for (size_t i = 0; i < m_aMeters.size (); i++)
		{
			CMeter*	pMeter		= m_aMeters[i];
			DWORD	nPdhError	= ERROR_SUCCESS;

			if (!pMeter->IsConnected ())
			{
				continue;
			}

			int		nResult	= pMeter->Update (nTick, nPdhError);

			if (nResult == SAMPLE_ERROR)
			{
				HandlePdhError (TEXT ("PdhGetRawCounterValue"), nPdhError);
				return;
			}
			if (nResult == SAMPLE_OK)
			{
				bAnySample	= TRUE;
			}
			else if (nResult == SAMPLE_UNUSABLE && pMeter->GetBadSamples () >= BAD_SAMPLES_MAX)
			{
				// A persistent problem means the query is stale
				bStale	= TRUE;
			}
		}

		// Not more often than the retry time, a counter that cannot recover must not keep the query busy
		if (bStale && nTick - m_nReconnectTick >= PDH_RETRY_MS)
		{
			Reconnect ();
			return;
		}

		// Counters that all read zero for a long time may be stale: open a new query. Meters read at different times, their last values count.
		if (bAnySample)
		{
			BOOL	bAllZero	= TRUE;

			for (size_t i = 0; i < m_aMeters.size () && bAllZero; i++)
			{
				if (m_aMeters[i]->IsConnected () && m_aMeters[i]->GetValue () > 0)
				{
					bAllZero	= FALSE;
				}
			}

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
		}
	}

	// Icons are only redrawn when what they show changes, the tooltips are throttled (the shell does not need to be updated on every reading).
	// Updating also re-adds a tray icon that the shell lost.
	UpdateMeters (nTick);

	SaveLearnedMax (FALSE);
	RetryFailedMeters ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Update the tray icons (and tooltips) of the meters that work
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateMeters (DWORD nTick)
{
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		CMeter*	pMeter	= m_aMeters[i];

		if (!pMeter->IsConnected () || pMeter->HasError ())
		{
			continue;
		}

		pMeter->UpdateIcon ();
		pMeter->UpdateTooltip (nTick);
	}
}


void CMainFrame::OnClose() 
{
	SaveLearnedMax (TRUE);

	// Clean up the PDH variables
	ClosePDH ();

	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		m_aMeters[i]->RemoveTrayIcon ();
	}
	
	// Call the parent class's function
	CFrameWnd::OnClose();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read settings (from INI or potentially other sources)
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::ReadSettings (DiskLEDSettings& oSettings)
{
	// The INI file. This also moves the file of an older version from the installation folder to its new place.
	m_sINIFile		= GetSettingsFile ();
	m_bFirstRun		= !IsFileExistent (m_sINIFile);

	return LoadSettings (m_sINIFile, oSettings);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Use the settings: create the meters (a meter with the same number is kept, with its tray icon), initialize the counters and start the timer
//	If the counters cannot be read, the timer keeps running and tries again
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::Initialize (const DiskLEDSettings& oSettings, BOOL bSilent)
{
	DWORD			nError	= ERROR_INVALID_FUNCTION;
	DiskLEDSettings	oNormalized	= oSettings;

	NormalizeSettings (oNormalized);

	// The query is opened again below, all counters are added again
	ClosePDH ();

	std::vector<CMeter*>	aMeters;

	for (size_t i = 0; i < oNormalized.aMeters.size (); i++)
	{
		const MeterSettings&	oMeter	= oNormalized.aMeters[i];
		CMeter*					pMeter	= NULL;

		for (size_t j = 0; j < m_aMeters.size () && pMeter == NULL; j++)
		{
			if (m_aMeters[j] != NULL && m_aMeters[j]->GetId () == oMeter.nId)
			{
				pMeter			= m_aMeters[j];
				m_aMeters[j]	= NULL;
				pMeter->SetSettings (oMeter);
				pMeter->ClearError ();
			}
		}

		if (pMeter == NULL)
		{
			pMeter	= new CMeter (this, oMeter);
			pMeter->Create ();
		}
		aMeters.push_back (pMeter);
	}

	// The meters that are no longer there remove their tray icons
	for (size_t j = 0; j < m_aMeters.size (); j++)
	{
		delete m_aMeters[j];
	}
	m_aMeters	= aMeters;

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

	// Set a new info tip including the counter path
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		if (!m_aMeters[i]->HasError ())
		{
			m_aMeters[i]->SetInitialTooltip ();
		}
	}

	return nError;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The current settings (including what the auto scale has learned meanwhile)
//
////////////////////////////////////////////////////////////////////////////////

void CMainFrame::GetSettings (DiskLEDSettings& oSettings)
{
	oSettings.aMeters.clear ();
	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		oSettings.aMeters.push_back (m_aMeters[i]->GetSettings ());
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Use new settings, e.g. while the settings window previews changes. Problems with a counter are shown on its tray icon, not in a message.
//	If the meters and their counters are the same, only their look, scale, alerts and names change: nothing is reconnected and the history is kept.
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMainFrame::ApplySettings (const DiskLEDSettings& oSettings)
{
	DiskLEDSettings	oNormalized	= oSettings;
	BOOL			bSame		= TRUE;

	NormalizeSettings (oNormalized);

	if (oNormalized.aMeters.size () != m_aMeters.size ())
	{
		bSame	= FALSE;
	}
	for (size_t i = 0; bSame && i < m_aMeters.size (); i++)
	{
		const MeterSettings&	oCurrent	= m_aMeters[i]->GetSettings ();
		const MeterSettings&	oNew		= oNormalized.aMeters[i];

		bSame	= (oCurrent.nId == oNew.nId && oCurrent.oCounter.sPath.CompareNoCase (oNew.oCounter.sPath) == 0);
	}

	if (!bSame)
	{
		return Initialize (oNormalized, TRUE);
	}

	for (size_t i = 0; i < m_aMeters.size (); i++)
	{
		m_aMeters[i]->UpdateSettings (oNormalized.aMeters[i]);
	}

	// A changed update interval applies right away
	if (m_hQuery)
	{
		ScheduleNextSample ();
	}

	return ERROR_SUCCESS;
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
