// Meter.cpp : one performance counter shown as one tray icon
//

#include "stdafx.h"
#include "DiskLED.h"
#include "MainFrm.h"
#include "Meter.h"
#include "Helpers.h"
#include "GpuSensor.h"
#include <math.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CMeter::CMeter (CWnd* pOwner, const MeterSettings& oSettings)
{
	m_pOwner		= pOwner;
	m_pTray			= NULL;
	m_hIcon			= NULL;
	m_nIconKey		= ICONKEY_NONE;
	m_hCounter		= NULL;
	m_bHaveRaw		= FALSE;
	m_dFactor		= 1;
	m_dOffset		= 0;
	m_bGpu			= FALSE;
	m_bGpuConnected	= FALSE;
	m_nNextSampleTick	= 0;
	m_nTooltipTick	= 0;
	m_nBadSamples	= 0;
	m_bError		= FALSE;
	m_bErrorShown	= FALSE;

	SecureZeroMemory (&m_oRawPrevious, sizeof (m_oRawPrevious));
	SetDefaultMeterSettings (m_oSettings, oSettings.nId);
	SetSettings (oSettings);
}

CMeter::~CMeter ()
{
	RemoveTrayIcon ();
	delete m_pTray;
	m_pTray	= NULL;

	// The icon is only released once the tray icon is gone
	if (m_hIcon)
	{
		DestroyIcon (m_hIcon);
		m_hIcon	= NULL;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	The tray icon
//
////////////////////////////////////////////////////////////////////////////////

void CMeter::Create ()
{
	if (m_pTray)
	{
		return;
	}

	m_pTray	= new CSystemTray;
	m_pTray->SetMenuID (IDR_POPUP_MENU);

	// Windows 11 treats icons of one program that are added with the same tooltip as one icon: they would be promoted and moved together.
	// Every meter's icon is added with its own tooltip. If the shell is not ready yet, the icon is added later (when its icon or tooltip is set).
	m_pTray->Create (m_pOwner, WM_ICON_NOTIFY, GetUniqueTooltip (), theApp.m_hIcon, TRAY_ICON_ID_BASE + m_oSettings.nId);
}

void CMeter::RemoveTrayIcon ()
{
	if (m_pTray)
	{
		m_pTray->RemoveIcon ();
	}
}

LRESULT CMeter::OnTrayNotification (WPARAM wParam, LPARAM lParam)
{
	return m_pTray ? m_pTray->OnTrayNotification (wParam, lParam) : 0;
}

// Set a new icon on the tray icon. The tray class keeps using the icon (it re-adds it when Explorer restarts),
// so the previous icon is only destroyed once the new one is set.
void CMeter::SetTrayIcon (HICON hNew)
{
	if (hNew == NULL || m_pTray == NULL)
	{
		return;
	}

	m_pTray->SetIcon (hNew);
	if (m_hIcon)
	{
		DestroyIcon (m_hIcon);
	}
	m_hIcon	= hNew;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Settings. The history starts again.
//
////////////////////////////////////////////////////////////////////////////////

void CMeter::SetSettings (const MeterSettings& oSettings)
{
	MeterSettings	oPrevious	= m_oSettings;

	m_oSettings	= oSettings;

	// The settings come from the dialog, which works on a copy: do not lose what was learned meanwhile
	if (oSettings.oCounter.sPath.CompareNoCase (oPrevious.oCounter.sPath) == 0 && oPrevious.oCounter.dLearnedMax > m_oSettings.oCounter.dLearnedMax)
	{
		m_oSettings.oCounter.dLearnedMax	= oPrevious.oCounter.dLearnedMax;
	}

	MakeIconStyle (m_oSettings, m_oStyle);
	UpdateSource ();
	ResetState ();
	m_nIconKey	= ICONKEY_NONE;
}

// Where the value comes from: a graphics adapter, or a performance counter. Temperature counters are in Kelvin, the meter works in Celsius.
void CMeter::UpdateSource ()
{
	m_bGpu	= IsGpuPath (m_oSettings.oCounter.sPath, &m_sGpuAdapter);
	if (!m_bGpu)
	{
		m_sGpuAdapter.Empty ();
		m_bGpuConnected	= FALSE;
	}

	if (!IsTemperatureFormat (m_oSettings.oCounter.nValueFormat) || !GetCelsiusConversion (m_oSettings.oCounter.sPath, m_dFactor, m_dOffset))
	{
		m_dFactor	= 1;
		m_dOffset	= 0;
	}
}

void CMeter::ResetState ()
{
	const CounterSettings&	oCounter	= m_oSettings.oCounter;

	SecureZeroMemory (&m_oRuntime, sizeof (m_oRuntime));
	SecureZeroMemory (&m_oIcon, sizeof (m_oIcon));

	m_oIcon.dScale				= oCounter.d100Percent;
	m_oIcon.nValueFormat		= oCounter.nValueFormat;
	m_oRuntime.dLearned			= oCounter.dLearnedMax;
	m_oRuntime.dLearnedSaved	= oCounter.dLearnedMax;
	if (oCounter.nScaleMode == SCALEMODE_AUTO && m_oRuntime.dLearned > m_oIcon.dScale)
	{
		m_oIcon.dScale	= m_oRuntime.dLearned;
	}

	m_nBadSamples	= 0;
}

void CMeter::UpdateSettings (const MeterSettings& oSettings)
{
	double	dLearned	= m_oRuntime.dLearned;
	DWORD	nInterval	= m_oSettings.oCounter.nUpdateInterval;

	m_oSettings	= oSettings;
	m_oSettings.oCounter.dLearnedMax	= dLearned;

	MakeIconStyle (m_oSettings, m_oStyle);
	UpdateSource ();
	m_oIcon.nValueFormat	= m_oSettings.oCounter.nValueFormat;
	UpdateCeiling ();

	// A new interval applies from now on, not only after the previous (possibly long) one
	if (m_oSettings.oCounter.nUpdateInterval != nInterval)
	{
		m_nNextSampleTick	= GetTickCount () + m_oSettings.oCounter.nUpdateInterval;
	}

	// Draw the icon again, the new scale and look apply with the next sample
	m_nIconKey	= ICONKEY_NONE;
}

// The auto scale cannot learn a maximum above the speed of the network adapter
void CMeter::UpdateCeiling ()
{
	m_oRuntime.dCeiling	= 0;
	if (m_oSettings.oCounter.nScaleMode == SCALEMODE_AUTO)
	{
		double	dSpeed	= 0;

		if (GetNetworkLinkSpeed (m_oSettings.oCounter.sPath, dSpeed))
		{
			m_oRuntime.dCeiling	= dSpeed;
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	The counter in the shared query
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMeter::Connect (PDH_HQUERY hQuery, CString& sError)
{
	const CString&	sPath	= m_oSettings.oCounter.sPath;

	m_hCounter			= NULL;
	m_bHaveRaw			= FALSE;
	m_bGpuConnected		= FALSE;

	// A graphics adapter temperature is read directly, it is not in the query
	if (m_bGpu)
	{
		double	dCelsius	= 0;

		if (!ReadGpuTemperature (m_sGpuAdapter, dCelsius))
		{
			sError.Format (TEXT ("The temperature of the graphics adapter %s cannot be read"), (LPCTSTR) m_sGpuAdapter);
			return ERROR_DEVICE_NOT_AVAILABLE;
		}
		m_bGpuConnected	= TRUE;

		return ERROR_SUCCESS;
	}

	// Prefer the English path, which works on any system language. Older INI files contain paths in the system language.
	PDH_STATUS	nError	= PdhAddEnglishCounter (hQuery, sPath, 0, &m_hCounter);
	if (nError != ERROR_SUCCESS)
	{
		nError	= PdhAddCounter (hQuery, sPath, 0, &m_hCounter);
	}
	if (nError != ERROR_SUCCESS)
	{
		m_hCounter	= NULL;
		sError.Format (TEXT ("PdhAddCounter failed with 0x%x for %s"), nError, (LPCTSTR) sPath);
		if (nError == PDH_CSTATUS_NO_OBJECT || nError == PDH_CSTATUS_NO_COUNTER || nError == PDH_CSTATUS_BAD_COUNTERNAME)
		{
			sError	+= TEXT (". The performance counter path is probably invalid");
		}
		return nError;
	}

	// The auto scale cannot learn a maximum above the speed of the network adapter
	UpdateCeiling ();

	return ERROR_SUCCESS;
}

// The query was closed, which also frees the counter
void CMeter::Disconnect ()
{
	m_hCounter			= NULL;
	m_bHaveRaw			= FALSE;
	m_bGpuConnected		= FALSE;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The name of the counter for tooltips and alerts: the last part of its path
//
////////////////////////////////////////////////////////////////////////////////

// The name for tooltips and alerts: the name of the meter, or the last part of the counter path
CString CMeter::GetName () const
{
	return m_oSettings.sName.IsEmpty () ? GetCounterLabel () : m_oSettings.sName;
}

CString CMeter::GetCounterLabel () const
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	if (!ParsePerfPath (m_oSettings.oCounter.sPath, sMachine, sObject, sInstance, sCounter))
	{
		sCounter	= m_oSettings.oCounter.sPath;
	}

	// Keep tooltips and balloons within their length limits
	sCounter.Truncate (48);

	return sCounter;
}

// The current value and the counter path, with the name of the meter if it has one
CString CMeter::GetTooltip () const
{
	const CounterSettings&	oCounter	= m_oSettings.oCounter;
	CString					sValue		= FormatValue (oCounter.nValueFormat, m_oIcon.dValue, m_oIcon.dScale, oCounter.nScaleMode == SCALEMODE_FIXED);

	if (!m_oSettings.sName.IsEmpty ())
	{
		return m_oSettings.sName + TEXT (": ") + sValue + TEXT ("\n") + oCounter.sPath;
	}

	return sValue + TEXT ("\n") + PROGRAM_NAME + TEXT (" - ") + oCounter.sPath;
}

CString CMeter::GetTooltipPrefix () const
{
	return m_oSettings.sName.IsEmpty () ? CString (PROGRAM_NAME) : m_oSettings.sName;
}

// Different for every meter, also for meters with the same name and counter
CString CMeter::GetUniqueTooltip () const
{
	CString	sTooltip;

	sTooltip.Format (TEXT ("%s meter %d - %s"), PROGRAM_NAME, m_oSettings.nId, (LPCTSTR) GetName ());
	sTooltip.Truncate (127);

	return sTooltip;
}

void CMeter::SetInitialTooltip ()
{
	// Unique, like the tooltip the icon is added with: if the shell was not ready, the icon is added later with this one
	if (m_pTray)
	{
		m_pTray->SetTooltipText (GetUniqueTooltip ());
	}
}

// The tooltip shows the value as text: the shell does not need it more often than TOOLTIP_UPDATE_MS
void CMeter::UpdateTooltip (DWORD nTick)
{
	if (m_pTray == NULL || (m_nTooltipTick != 0 && nTick - m_nTooltipTick < TOOLTIP_UPDATE_MS))
	{
		return;
	}

	m_nTooltipTick	= nTick ? nTick : 1;
	m_pTray->SetTooltipText (GetTooltip ());
}

// Icons are only redrawn when what they show changes
void CMeter::UpdateIcon ()
{
	int		nSize	= GetSystemMetrics (SM_CXSMICON);
	DWORD	nKey	= GetIconKey (m_oStyle, &m_oIcon, 1, nSize);

	if (nKey != m_nIconKey)
	{
		SetTrayIcon (CreateTrayIconImage (m_oStyle, &m_oIcon, 1));
		m_nIconKey	= nKey;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	The counter cannot be read: show an error icon and the problem in the tooltip. The main window keeps trying to reconnect.
//
////////////////////////////////////////////////////////////////////////////////

void CMeter::ShowError (const CString& sMessage, BOOL bBalloon)
{
	int		nSize	= GetSystemMetrics (SM_CXSMICON);

	m_bError	= TRUE;

	SetTrayIcon ((HICON) LoadImage (AfxGetResourceHandle (), MAKEINTRESOURCE (IDI_ICON6), IMAGE_ICON, nSize, nSize, 0));
	if (m_pTray)
	{
		m_pTray->SetTooltipText (GetTooltipPrefix () + TEXT (" - ") + sMessage);
	}

	// Report the problem once per error period
	if (bBalloon && !m_bErrorShown && m_pTray)
	{
		CString	sText	= sMessage + TEXT (". DiskLED keeps trying.");

		sText.Truncate (255);
		m_bErrorShown	= TRUE;
		m_pTray->ShowBalloon (sText, PROGRAM_NAME, NIIF_ERROR, 10);
	}
}

void CMeter::ClearError ()
{
	m_bError		= FALSE;
	m_bErrorShown	= FALSE;
	m_nBadSamples	= 0;
	m_nIconKey		= ICONKEY_NONE;
	m_nTooltipTick	= 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Show a balloon when the value has been above its alert threshold for the configured time
//	The balloon is shown once per period above the threshold. Short dips below the threshold do not end the period.
//
////////////////////////////////////////////////////////////////////////////////

void CMeter::CheckAlert ()
{
	const CounterSettings&	oCounter	= m_oSettings.oCounter;
	double					dScale		= m_oIcon.dScale;

	// The percentage is of the scale, the value that fills the icon. With the Peak, Decay and Auto scale, the 100% value is only a minimum.
	if (oCounter.nAlertPercent <= 0 || dScale <= 0)
	{
		return;
	}

	double	dPercent	= m_oIcon.dValue * 100.0 / dScale;
	DWORD	nTick		= GetTickCount ();

	if (!(dPercent >= oCounter.nAlertPercent))
	{
		if (m_oRuntime.bAbove)
		{
			if (!m_oRuntime.bBelow)
			{
				m_oRuntime.bBelow		= TRUE;
				m_oRuntime.nBelowSince	= nTick;
			}
			else if (nTick - m_oRuntime.nBelowSince >= ALERT_DIP_MS)
			{
				// Back to normal: the next period above the threshold raises a new alert
				m_oRuntime.bAbove	= FALSE;
				m_oRuntime.bBelow	= FALSE;
				m_oRuntime.bAlerted	= FALSE;
			}
		}
		return;
	}

	m_oRuntime.bBelow	= FALSE;
	if (!m_oRuntime.bAbove)
	{
		m_oRuntime.bAbove		= TRUE;
		m_oRuntime.nAboveSince	= nTick;
	}

	if (!m_oRuntime.bAlerted && (ULONGLONG) (nTick - m_oRuntime.nAboveSince) >= (ULONGLONG) oCounter.nAlertSeconds * 1000)
	{
		CString	sText;

		m_oRuntime.bAlerted	= TRUE;
		sText.Format (TEXT ("%s: %s, above %d%% of the scale for %d s."), (LPCTSTR) GetName (),
			(LPCTSTR) FormatValue (oCounter.nValueFormat, m_oIcon.dValue, m_oIcon.dScale, oCounter.nScaleMode == SCALEMODE_FIXED),
			oCounter.nAlertPercent, oCounter.nAlertSeconds);

		// The balloon text is limited to 255 characters
		sText.Truncate (255);
		if (m_pTray)
		{
			m_pTray->ShowBalloon (sText, PROGRAM_NAME, NIIF_WARNING, 10);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	When the counter is read next. A counter without a previous raw value is read as soon as possible, to have a starting point.
//
////////////////////////////////////////////////////////////////////////////////

DWORD CMeter::GetTimeToNextSample (DWORD nTick) const
{
	if (!m_bHaveRaw)
	{
		return 0;
	}

	int	nLeft	= (int) (m_nNextSampleTick - nTick);

	return (nLeft <= SAMPLE_SLACK_MS) ? 0 : (DWORD) nLeft;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Read the counter and update the scale and the alert
//	The value is computed from the raw values of this and the previous reading of this meter, so it covers the meter's own interval
//	even though the shared query is collected more often for faster meters
//
////////////////////////////////////////////////////////////////////////////////

int CMeter::Update (DWORD nTick, DWORD& nPdhError)
{
	const CounterSettings&	oCounter	= m_oSettings.oCounter;
	DWORD					nInterval	= oCounter.nUpdateInterval;
	PDH_RAW_COUNTER			oRaw;
	PDH_FMT_COUNTERVALUE	oValue;

	nPdhError	= ERROR_SUCCESS;
	if (GetTimeToNextSample (nTick) > 0)
	{
		return SAMPLE_NOT_DUE;
	}

	// Keep a steady rhythm. After a pause (sleep, a busy system) start again from now.
	m_nNextSampleTick	+= nInterval;
	if (!m_bHaveRaw || (int) (m_nNextSampleTick - nTick) <= 0)
	{
		m_nNextSampleTick	= nTick + nInterval;
	}

	SecureZeroMemory (&oRaw, sizeof (oRaw));

	// A graphics adapter is read directly. A reading that fails is ignored for a while, the adapter may be waking up. A persistent failure is an error.
	if (m_bGpu)
	{
		double	dCelsius	= 0;

		if (!ReadGpuTemperature (m_sGpuAdapter, dCelsius))
		{
			m_nBadSamples++;
			if (m_nBadSamples >= BAD_SAMPLES_MAX)
			{
				m_bGpuConnected	= FALSE;
				ShowError (TEXT ("The temperature of the graphics adapter ") + m_sGpuAdapter + TEXT (" cannot be read"), TRUE);
			}
			return SAMPLE_UNUSABLE;
		}
		m_nBadSamples	= 0;
		m_bHaveRaw		= TRUE;

		return ApplyValue (dCelsius, nTick);
	}

	nPdhError	= PdhGetRawCounterValue (m_hCounter, NULL, &oRaw);
	if (nPdhError != ERROR_SUCCESS)
	{
		return SAMPLE_ERROR;
	}

	// The call can succeed with a value that is not valid (e.g. the network adapter instance is gone)
	if (oRaw.CStatus != PDH_CSTATUS_VALID_DATA && oRaw.CStatus != PDH_CSTATUS_NEW_DATA)
	{
		m_nBadSamples++;
		return SAMPLE_UNUSABLE;
	}

	// The first reading is only the starting point
	if (!m_bHaveRaw)
	{
		m_oRawPrevious	= oRaw;
		m_bHaveRaw		= TRUE;
		return SAMPLE_NOT_DUE;
	}

	// Get the counter data as floating point number. Percentage counters are not capped at 100 (e.g. a process using several cores).
	PDH_STATUS	nCalcError	= PdhCalculateCounterFromRawValue (m_hCounter, PDH_FMT_DOUBLE | PDH_FMT_NOCAP100, &oRaw, &m_oRawPrevious, &oValue);

	m_oRawPrevious	= oRaw;
	if (nCalcError != ERROR_SUCCESS)
	{
		// PDH_CALC_NEGATIVE_VALUE happens on Windows 7 RTM if a USB flash drive is inserted.
		// The algorithm probably has a bug in that it returns a negative value because of the additional disk which cannot be converted to LONG.
		// A single such sample is ignored, a persistent one means the query is stale.
		m_nBadSamples++;
		return SAMPLE_UNUSABLE;
	}

	// The calculation can succeed with a value that is not valid
	if (oValue.CStatus != PDH_CSTATUS_VALID_DATA && oValue.CStatus != PDH_CSTATUS_NEW_DATA)
	{
		m_nBadSamples++;
		return SAMPLE_UNUSABLE;
	}

	m_nBadSamples	= 0;

	return ApplyValue (oValue.doubleValue * m_dFactor + m_dOffset, nTick);
}


////////////////////////////////////////////////////////////////////////////////
//
//	A new value: remember it, update the scale and the alert
//
////////////////////////////////////////////////////////////////////////////////

int CMeter::ApplyValue (double dValue, DWORD nTick)
{
	const CounterSettings&	oCounter	= m_oSettings.oCounter;
	DWORD					nInterval	= oCounter.nUpdateInterval;

	if (oCounter.bInvert)
	{
		dValue	= oCounter.d100Percent - dValue;
	}
	if (!(dValue > 0))
	{
		// Also catches NaN
		dValue	= 0;
	}

	// Remember the sample (oldest first)
	memmove (m_oIcon.adHistory, m_oIcon.adHistory + 1, (HISTORY_SIZE - 1) * sizeof (double));
	m_oIcon.adHistory[HISTORY_SIZE - 1]	= dValue;
	m_oIcon.dValue						= dValue;
	m_oIcon.nValueFormat				= oCounter.nValueFormat;

	// The scale is the value that corresponds to the full icon. The 100% value is the minimum.
	double	dScale	= oCounter.d100Percent;
	if (oCounter.nScaleMode == SCALEMODE_PEAK)
	{
		for (int j = 0; j < HISTORY_SIZE; j++)
		{
			if (m_oIcon.adHistory[j] > dScale)
			{
				dScale	= m_oIcon.adHistory[j];
			}
		}
	}
	else if (oCounter.nScaleMode == SCALEMODE_DECAY)
	{
		// The peak falls by half every DECAY_HALFLIFE_MS (measured, the timer is not exact), a higher value becomes the new peak
		DWORD	nElapsed	= (m_oRuntime.nLastTick != 0) ? nTick - m_oRuntime.nLastTick : nInterval;
		if (nElapsed > 60000)
		{
			nElapsed	= 60000;
		}

		double	dDecayed			= m_oRuntime.dDecayPeak * pow (0.5, (double) nElapsed / DECAY_HALFLIFE_MS);
		m_oRuntime.dDecayPeak		= (dValue > dDecayed) ? dValue : dDecayed;
		if (m_oRuntime.dDecayPeak > dScale)
		{
			dScale	= m_oRuntime.dDecayPeak;
		}
	}
	else if (oCounter.nScaleMode == SCALEMODE_AUTO)
	{
		// The learned maximum rises with every new high and falls very slowly. Nothing above the adapter speed can be real.
		DWORD	nElapsed	= (m_oRuntime.nLastTick != 0) ? nTick - m_oRuntime.nLastTick : nInterval;
		if (nElapsed > 60000)
		{
			nElapsed	= 60000;
		}

		double	dLearned	= m_oRuntime.dLearned * pow (0.5, (double) nElapsed / LEARN_HALFLIFE_MS);
		double	dSample		= dValue;
		if (m_oRuntime.dCeiling > 0)
		{
			dSample		= (dSample > m_oRuntime.dCeiling) ? m_oRuntime.dCeiling : dSample;
			dLearned	= (dLearned > m_oRuntime.dCeiling) ? m_oRuntime.dCeiling : dLearned;
		}
		if (dSample > dLearned)
		{
			dLearned	= dSample;
		}

		m_oRuntime.dLearned					= dLearned;
		m_oSettings.oCounter.dLearnedMax	= dLearned;
		if (dLearned > dScale)
		{
			dScale	= dLearned;
		}
	}
	m_oRuntime.nLastTick	= nTick;
	m_oIcon.dScale			= dScale;

	CheckAlert ();

	return SAMPLE_OK;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The learned maximum of the auto scale, if it changed noticeably since it was last written to the INI file
//
////////////////////////////////////////////////////////////////////////////////

BOOL CMeter::GetLearnedToSave (double& dLearned) const
{
	if (m_oSettings.oCounter.nScaleMode != SCALEMODE_AUTO || !(m_oRuntime.dLearned > 0))
	{
		return FALSE;
	}
	if (fabs (m_oRuntime.dLearned - m_oRuntime.dLearnedSaved) <= 0.02 * m_oRuntime.dLearned)
	{
		return FALSE;
	}

	dLearned	= m_oRuntime.dLearned;

	return TRUE;
}
