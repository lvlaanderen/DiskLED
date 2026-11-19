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
#define	SAMPLE_NOT_DUE				3		// The meter's update interval has not passed yet, nothing was read

// A meter whose next reading is at most this far away is read now: timers do not fire exactly on time
#define	SAMPLE_SLACK_MS				8


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


// A meter: one performance counter shown as one tray icon, with its own scale, alert, error state and update interval.
// The PDH query is shared and owned by the main window, the meter only adds its counter to it. The query is collected as often as the
// fastest meter needs; every meter computes its value from its own previous raw sample, so it covers the meter's whole interval.
class CMeter
{
public:
	CMeter (CWnd* pOwner, const MeterSettings& oSettings);
	~CMeter ();

	// The tray icon. Create does not fail when the shell is not ready yet, the icon is added later.
	void	Create ();
	void	RemoveTrayIcon ();
	LRESULT	OnTrayNotification (WPARAM wParam, LPARAM lParam);

	int						GetId () const			{ return m_oSettings.nId; }
	const MeterSettings&	GetSettings () const	{ return m_oSettings; }

	// New settings: the history starts again. A learned maximum is kept if the counter is the same.
	void	SetSettings (const MeterSettings& oSettings);

	// New settings for the same counter (the look, scale, alerts, name, interval): the history and what was learned are kept
	void	UpdateSettings (const MeterSettings& oSettings);

	// The counter in the shared query. Connect returns a PDH error, with the reason in sError.
	// A graphics adapter temperature is not in the query: Connect only checks that the adapter can be read (the error is then a Windows error code).
	DWORD	Connect (PDH_HQUERY hQuery, CString& sError);
	void	Disconnect ();
	BOOL	IsConnected () const	{ return m_bGpu ? m_bGpuConnected : m_hCounter != NULL; }

	// Does the meter use the shared PDH query? (Not so a graphics adapter temperature.)
	BOOL	UsesPdh () const		{ return !m_bGpu; }

	// Ticks until the next reading is due (0 = due now)
	DWORD	GetTimeToNextSample (DWORD nTick) const;

	// After the query was collected: read the counter if it is due, update scale and alert.
	// Returns SAMPLE_*, for SAMPLE_ERROR the PDH error is in nPdhError. The first call after connecting only remembers the raw value.
	int		Update (DWORD nTick, DWORD& nPdhError);
	double	GetValue () const		{ return m_oIcon.dValue; }
	int		GetBadSamples () const	{ return m_nBadSamples; }
	void	ResetSampling ()		{ m_nBadSamples = 0; m_oRuntime.nLastTick = 0; m_bHaveRaw = FALSE; }

	void	UpdateIcon ();
	void	UpdateTooltip (DWORD nTick);
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
	PDH_RAW_COUNTER	m_oRawPrevious;		// The raw value of the previous reading, the value is computed from it and the current one
	BOOL			m_bHaveRaw;			// m_oRawPrevious is valid
	double			m_dFactor;			// The counter value is multiplied by this and m_dOffset is added (a temperature in Kelvin becomes Celsius)
	double			m_dOffset;
	BOOL			m_bGpu;				// The meter shows the temperature of a graphics adapter (read directly, not with PDH)
	BOOL			m_bGpuConnected;	// ... and the adapter could be read the last time
	CString			m_sGpuAdapter;
	DWORD			m_nNextSampleTick;	// Tick count when the next reading is due
	DWORD			m_nTooltipTick;		// Tick count of the last tooltip update, 0 = update with the next reading
	int				m_nBadSamples;		// Consecutive samples that could not be used
	BOOL			m_bError;
	BOOL			m_bErrorShown;		// The error was already reported with a balloon
	CString			m_sDisplayName;		// The counter name with instance, see GetCounterDisplayName. Kept because the tooltip is updated often.

	void	ResetState ();
	int		ApplyValue (double dValue, DWORD nTick);
	void	UpdateSource ();
	void	UpdateCeiling ();
	CString	GetName () const;
	CString	GetTooltipPrefix () const;
	CString	GetUniqueTooltip () const;
	CString	GetCounterLabel () const;
	CString	GetTooltip () const;
	void	SetTrayIcon (HICON hNew);
	void	CheckAlert ();
};
