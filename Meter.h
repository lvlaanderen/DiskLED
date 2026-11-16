#pragma once

#include <pdh.h>
#include "Settings.h"
#include "IconRenderer.h"
#include "SystemTray.h"


// The ID of the tray icon of a meter is this plus the number of the meter. Windows remembers the position of an icon by its ID.
#define	TRAY_ICON_ID_BASE			1000

// A value that dips below the alert threshold for less than this long does not restart the alert timer
#define	ALERT_DIP_MS				3000

// Result of reading a counter
#define	SAMPLE_OK					0
#define	SAMPLE_UNUSABLE				1		// Invalid for now, e.g. right after (re)connecting. A persistent problem is detected with GetBadSamples.
#define	SAMPLE_ERROR				2		// PDH reported an error: the query is stale


// Runtime state of one performance counter
struct CounterRuntime
{
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


// A meter: one performance counter shown as one tray icon, with its own scale, alert and error state.
// The PDH query is shared and owned by the main window, the meter only adds its counter to it.
class CMeter
{
public:
	CMeter (CWnd* pOwner, const MeterSettings& oSettings, DWORD nUpdateInterval);
	~CMeter ();

	// The tray icon. Create does not fail when the shell is not ready yet, the icon is added later.
	void	Create ();
	void	RemoveTrayIcon ();
	LRESULT	OnTrayNotification (WPARAM wParam, LPARAM lParam);

	int						GetId () const			{ return m_oSettings.nId; }
	const MeterSettings&	GetSettings () const	{ return m_oSettings; }

	// New settings: the history starts again. A learned maximum is kept if the counter is the same.
	void	SetSettings (const MeterSettings& oSettings, DWORD nUpdateInterval);

	// The counter in the shared query. Connect returns a PDH error, with the reason in sError.
	DWORD	Connect (PDH_HQUERY hQuery, CString& sError);
	void	Disconnect ();
	BOOL	IsConnected () const	{ return m_hCounter != NULL; }

	// Read the counter, update scale and alert. Returns SAMPLE_*, for SAMPLE_ERROR the PDH error is in nPdhError.
	int		Update (DWORD nTick, DWORD nUpdateInterval, DWORD& nPdhError);
	double	GetValue () const		{ return m_oIcon.dValue; }
	int		GetBadSamples () const	{ return m_nBadSamples; }
	void	ResetSampling ()		{ m_nBadSamples = 0; m_oRuntime.nLastTick = 0; }

	void	UpdateIcon ();
	void	UpdateTooltip ();
	void	SetInitialTooltip ();

	// Error state: an error icon, the problem as tooltip and (once) as balloon
	void	ShowError (const CString& sMessage, BOOL bBalloon);
	void	ClearError ();
	BOOL	HasError () const		{ return m_bError; }

	// The learned maximum if it has changed enough to be worth writing to the INI file
	BOOL	GetLearnedToSave (double& dLearned) const;
	void	SetLearnedSaved (double dLearned)	{ m_oRuntime.dLearnedSaved = dLearned; }

private:
	CWnd*			m_pOwner;
	CSystemTray*	m_pTray;
	HICON			m_hIcon;			// The icon currently set on the tray icon. The tray class keeps using it, so it is owned here.
	DWORD			m_nIconKey;			// Identifies the icon currently shown (see GetIconKey)

	MeterSettings	m_oSettings;
	IconStyle		m_oStyle;
	CounterRuntime	m_oRuntime;
	IconCounter		m_oIcon;
	PDH_HCOUNTER	m_hCounter;
	int				m_nBadSamples;		// Consecutive samples that could not be used
	BOOL			m_bError;
	BOOL			m_bErrorShown;		// The error was already reported with a balloon

	void	ResetState ();
	CString	GetCounterLabel () const;
	CString	GetTooltip () const;
	void	SetTrayIcon (HICON hNew);
	void	CheckAlert ();
};
