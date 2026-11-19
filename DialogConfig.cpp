// DialogConfig.cpp : implementation file
//

#include "stdafx.h"
#include "DiskLED.h"
#include "MainFrm.h"
#include "DialogConfig.h"
#include "DialogMessage.h"
#include "DialogWizard.h"
#include "Helpers.h"
#include <math.h>


// CDialogConfig dialog

IMPLEMENT_DYNAMIC(CDialogConfig, CDialog)

CDialogConfig::CDialogConfig(CMainFrame* pMainFrame) : CDialog(CDialogConfig::IDD, pMainFrame)
														, m_nCounterSel(0)
														, m_nLEDType(0), m_nUpdateFrequency(0)
														, m_nValueFormat(0), m_nScaleMode(0), m_nInvert(0), m_nAutoStart(0)
														, m_nAlertPercent(0), m_nAlertSeconds(0)
														, m_nThresholdMid(0), m_nThresholdHigh(0)
														, m_d100Percent(0)
														, m_nTab(0)
														, m_dLinkSpeed(0)
														, m_sCounterPath(_T(""))
														, m_sPerfObject(_T(""))
														, m_sPerfCounter(_T(""))
														, m_sPerfInstance(_T(""))
														, m_sPerfDescription(_T(""))
														, m_dPreviewTime(0)
														, m_bLoading(TRUE)
														, m_bDirty(FALSE)
														, m_bFirstRunImport(FALSE)
														, m_nAutoStartSaved(0)
{
	if (pMainFrame)
	{
		m_pMainFrame	= (CMainFrame*) pMainFrame;
	}
	else
	{
		m_pMainFrame	= NULL;
	}

	SetDefaultSettings (m_oSettings);
	SecureZeroMemory (m_anColors, sizeof (m_anColors));
	SecureZeroMemory (&m_oPreviewStyle, sizeof (m_oPreviewStyle));
	SecureZeroMemory (&m_oPreview, sizeof (m_oPreview));

	// Memory buffers for PdhEnumObjectItems
	m_sBufferCounters	= NULL;
	m_nBufferCounters	= 0;
	m_sBufferInstances	= NULL;
	m_nBufferInstances	= 0;

	// Memory buffer for PdhGetCounterInfo
	m_nBufferInfo		= 0;
	m_oBufferInfo		= NULL;
}

CDialogConfig::~CDialogConfig()
{
	if (m_sBufferCounters)
	{
		HeapFree (GetProcessHeap (), 0, m_sBufferCounters);
		m_sBufferCounters	= NULL;
	}
	if (m_sBufferInstances)
	{
		HeapFree (GetProcessHeap (), 0, m_sBufferInstances);
		m_sBufferInstances	= NULL;
	}
	if (m_oBufferInfo)
	{
		HeapFree (GetProcessHeap (), 0, m_oBufferInfo);
		m_oBufferInfo	= NULL;
	}
}

void CDialogConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_LIST_METERS, m_cMeterList);
	DDX_Control(pDX, IDC_EDIT_METERNAME, m_cMeterName);
	DDX_Text(pDX, IDC_EDIT_METERNAME, m_sMeterName);
	DDX_Control(pDX, IDC_SAVE, m_cSave);
	DDX_Control(pDX, IDC_REVERT, m_cRevert);
	DDX_Control(pDX, IDC_STATUS_TEXT, m_cStatus);
	DDX_Control(pDX, IDC_ADD_METER, m_cAddMeter);
	DDX_Control(pDX, IDC_REMOVE_METER, m_cRemoveMeter);
	DDX_Control(pDX, IDC_COMBO_ICONTYPE, m_cIconType);
	DDX_CBIndex(pDX, IDC_COMBO_ICONTYPE, m_nLEDType);
	DDX_Control(pDX, IDC_COMBO_VALUEFORMAT, m_cValueFormat);
	DDX_CBIndex(pDX, IDC_COMBO_VALUEFORMAT, m_nValueFormat);
	DDX_Control(pDX, IDC_COMBO_SCALEMODE, m_cScaleMode);
	DDX_CBIndex(pDX, IDC_COMBO_SCALEMODE, m_nScaleMode);
	DDX_Control(pDX, IDC_CHECK_INVERT, m_cInvert);
	DDX_Check(pDX, IDC_CHECK_INVERT, m_nInvert);
	DDX_Control(pDX, IDC_EDIT_ALERT_PERCENT, m_cAlertPercent);
	DDX_Control(pDX, IDC_EDIT_ALERT_SECONDS, m_cAlertSeconds);
	DDX_Control(pDX, IDC_CHECK_AUTOSTART, m_cAutoStart);
	DDX_Check(pDX, IDC_CHECK_AUTOSTART, m_nAutoStart);
	DDX_Control(pDX, IDC_COLOR_LOW, m_cColor[0]);
	DDX_Control(pDX, IDC_COLOR_MID, m_cColor[1]);
	DDX_Control(pDX, IDC_COLOR_HIGH, m_cColor[2]);
	DDX_Control(pDX, IDC_EDIT_THRESHOLD_MID, m_cThresholdMid);
	DDX_Control(pDX, IDC_EDIT_THRESHOLD_HIGH, m_cThresholdHigh);

	// The numbers are only shown here. They are read by ReadControls, so a half typed number is no error message.
	if (!pDX->m_bSaveAndValidate)
	{
		DDX_Text(pDX, IDC_EDIT_ALERT_PERCENT, m_nAlertPercent);
		DDX_Text(pDX, IDC_EDIT_ALERT_SECONDS, m_nAlertSeconds);
		DDX_Text(pDX, IDC_EDIT_THRESHOLD_MID, m_nThresholdMid);
		DDX_Text(pDX, IDC_EDIT_THRESHOLD_HIGH, m_nThresholdHigh);
		DDX_Text(pDX, IDC_UPDATEFREQUENCY_EDIT, m_nUpdateFrequency);
	}
	DDX_Control(pDX, IDC_PREVIEW, m_cPreview);
	DDX_Control(pDX, IDC_UPDATEFREQUENCY_TEXT1, m_cUpdateFrequencyText1);
	DDX_Control(pDX, IDC_UPDATEFREQUENCY_TEXT2, m_cUpdateFrequencyText2);
	DDX_Control(pDX, IDC_UPDATEFREQUENCY_EDIT, m_cUpdateFrequencyEdit);
	DDX_Control(pDX, IDC_DEFAULTVALUES, m_cDefaultValues);
	DDX_Control(pDX, IDC_100PERCENT_EDIT, m_c100Percent);
	DDX_Control(pDX, IDC_TAB_MAIN, m_cTab);
	DDX_Control(pDX, IDC_COMBO_QUICKPICK, m_cQuickPick);
	DDX_Control(pDX, IDC_LINKSPEED_TEXT, m_cLinkSpeedText);
	DDX_Control(pDX, IDC_USE_LINKSPEED, m_cUseLinkSpeed);
	DDX_Control(pDX, IDC_COUNTERPATH_EDIT, m_cCounterPath);

	DDX_Text(pDX, IDC_100PERCENT_EDIT, m_s100Percent);
	DDX_Text(pDX, IDC_COUNTERPATH_EDIT, m_sCounterPath);
	DDX_Control(pDX, IDC_PERF_OBJECTS, m_cSelectPerfObjects);
	DDX_CBString(pDX, IDC_PERF_OBJECTS, m_sPerfObject);
	DDX_Control(pDX, IDC_PERF_COUNTERS, m_cSelectPerfCounters);
	DDX_CBString(pDX, IDC_PERF_COUNTERS, m_sPerfCounter);
	DDX_CBString(pDX, IDC_PERF_INSTANCES, m_sPerfInstance);
	DDX_Control(pDX, IDC_PERF_INSTANCES, m_cSelectPerfInstances);
	DDX_Control(pDX, IDC_USE_SELECTED_DATA_SOURCE, m_cBuildPathFromSelection);
	DDX_Control(pDX, IDC_PERF_DESCRIPTION, m_cSelectPerfDescription);
	DDX_Text(pDX, IDC_PERF_DESCRIPTION, m_sPerfDescription);
}


BOOL CDialogConfig::PreTranslateMessage(MSG* pMsg)
{
	if (m_oToolTip)
	{
		m_oToolTip.RelayEvent(pMsg);
	}

	return CDialog::PreTranslateMessage(pMsg);
}


BEGIN_MESSAGE_MAP(CDialogConfig, CDialog)
	ON_COMMAND(IDOK, &CDialogConfig::OnOK)
	ON_COMMAND(IDCANCEL, &CDialogConfig::OnCancel)
	ON_BN_CLICKED(IDC_SAVE, &CDialogConfig::OnBnClickedSave)
	ON_BN_CLICKED(IDC_REVERT, &CDialogConfig::OnBnClickedRevert)
	ON_BN_CLICKED(IDC_DEFAULTVALUES, &CDialogConfig::OnBnClickedDefaultvalues)
	ON_BN_CLICKED(IDC_USE_SELECTED_DATA_SOURCE, &CDialogConfig::OnBnClickedUseSelectedDataSource)
	ON_BN_CLICKED(IDC_USE_LINKSPEED, &CDialogConfig::OnBnClickedUseLinkSpeed)
	ON_CBN_SELCHANGE(IDC_COMBO_QUICKPICK, &CDialogConfig::OnCbnSelchangeQuickPick)
	ON_CONTROL_RANGE(BN_CLICKED, IDC_COLOR_LOW, IDC_COLOR_HIGH, &CDialogConfig::OnColorButton)
	ON_WM_DRAWITEM()
	ON_WM_CTLCOLOR()
	ON_WM_SETTINGCHANGE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB_MAIN, &CDialogConfig::OnTcnSelchangeTab)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_LIST_METERS, &CDialogConfig::OnLbnSelchangeMeterList)
	ON_MESSAGE(WM_IMPORT_WIZARD, &CDialogConfig::OnImportWizard)
	ON_BN_CLICKED(IDC_ADD_METER, &CDialogConfig::OnBnClickedAddMeter)
	ON_BN_CLICKED(IDC_REMOVE_METER, &CDialogConfig::OnBnClickedRemoveMeter)
	ON_CBN_SELCHANGE(IDC_COMBO_VALUEFORMAT, &CDialogConfig::OnCbnSelchangeValueFormat)
	ON_CBN_SELCHANGE(IDC_COMBO_SCALEMODE, &CDialogConfig::OnCbnSelchangeScaleMode)
	ON_CBN_SELCHANGE(IDC_PERF_OBJECTS, &CDialogConfig::OnCbnSelchangePerfObjects)
	ON_CBN_SELCHANGE(IDC_PERF_COUNTERS, &CDialogConfig::OnCbnSelchangePerfCounters)
	ON_CBN_SELCHANGE(IDC_PERF_INSTANCES, &CDialogConfig::OnCbnSelchangePerfInstances)
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////
//
//	Pages: the controls of each page of the tab strip. The settings are all in this one dialog, a page only shows its own controls.
//	The link speed controls are not listed, they are only shown on the scale page when the counter is a network counter.
//
////////////////////////////////////////////////////////////////////////////////

static const UINT g_aPageDataSource[] =
{
	IDC_COUNTERPATH_TEXT2, IDC_PERF_OBJECTS, IDC_COUNTERPATH_TEXT3, IDC_PERF_COUNTERS, IDC_COUNTERPATH_TEXT4, IDC_PERF_INSTANCES,
	IDC_COUNTERPATH_TEXT5, IDC_PERF_DESCRIPTION, IDC_USE_SELECTED_DATA_SOURCE, IDC_COUNTERPATH_TEXT, IDC_COUNTERPATH_EDIT,
	IDC_QUICKPICK_TEXT, IDC_COMBO_QUICKPICK, 0
};

static const UINT g_aPageScale[] =
{
	IDC_100PERCENT_TEXT, IDC_100PERCENT_TEXT2, IDC_100PERCENT_EDIT, IDC_VALUEFORMAT_TEXT, IDC_COMBO_VALUEFORMAT,
	IDC_SCALEMODE_TEXT, IDC_COMBO_SCALEMODE, IDC_CHECK_INVERT, 0
};

static const UINT g_aPageAppearance[] =
{
	IDC_COLORS_TEXT, IDC_COLOR_LOW, IDC_COLOR_MID, IDC_COLOR_HIGH, IDC_MIDFROM_TEXT, IDC_EDIT_THRESHOLD_MID, IDC_HIGHFROM_TEXT,
	IDC_EDIT_THRESHOLD_HIGH, IDC_PERCENT_TEXT, IDC_100PERCENT_TEXT3, IDC_UPDATEFREQUENCY_TEXT1, IDC_UPDATEFREQUENCY_EDIT,
	IDC_UPDATEFREQUENCY_TEXT2, 0
};

static const UINT g_aPageAlerts[] =
{
	IDC_WARN_TEXT1, IDC_EDIT_ALERT_PERCENT, IDC_WARN_TEXT2, IDC_EDIT_ALERT_SECONDS, IDC_WARN_TEXT3, IDC_WARN_TEXT4, 0
};

static const UINT* const g_aPages[] = { g_aPageDataSource, g_aPageScale, g_aPageAppearance, g_aPageAlerts };

void CDialogConfig::ShowPage (int nPage)
{
	for (int i = 0; i < _countof (g_aPages); i++)
	{
		for (const UINT* pID = g_aPages[i]; *pID != 0; pID++)
		{
			CWnd*	pControl	= GetDlgItem (*pID);

			if (pControl)
			{
				pControl->ShowWindow ((i == nPage) ? SW_SHOW : SW_HIDE);
			}
		}
	}

	m_nTab	= nPage;
	UpdateLinkSpeedVisibility ();
}

void CDialogConfig::OnTcnSelchangeTab (NMHDR*, LRESULT* pResult)
{
	int	nPage	= m_cTab.GetCurSel ();

	if (nPage >= 0 && nPage < _countof (g_aPages))
	{
		ShowPage (nPage);
	}

	*pResult	= 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Settings <-> controls: the settings of the meter being edited
//
////////////////////////////////////////////////////////////////////////////////

// The meter: its icon look and its counter
void CDialogConfig::LoadMeterToControls (int nMeter)
{
	const MeterSettings&	oMeter		= m_oSettings.aMeters[nMeter];
	const CounterSettings&	oCounter	= oMeter.oCounter;

	m_sMeterName		= oMeter.sName;
	m_nLEDType			= oMeter.nIconType - 1;
	m_nThresholdMid		= oMeter.nThresholdMid;
	m_nThresholdHigh	= oMeter.nThresholdHigh;
	for (int i = 0; i < 3; i++)
	{
		m_anColors[i]	= oMeter.anColors[i];
	}

	m_sCounterPath		= EnglishPathToLocal (oCounter.sPath);
	m_d100Percent		= oCounter.d100Percent;
	m_s100Percent		= FormatScaleValue (oCounter.nValueFormat, oCounter.d100Percent);
	m_s100PercentShown	= m_s100Percent;
	m_nValueFormat		= oCounter.nValueFormat - 1;
	m_nScaleMode		= oCounter.nScaleMode - 1;
	m_nInvert			= oCounter.bInvert ? 1 : 0;
	m_nAlertPercent		= oCounter.nAlertPercent;
	m_nAlertSeconds		= oCounter.nAlertSeconds;
	m_nUpdateFrequency	= oCounter.nUpdateInterval;
}

void CDialogConfig::SaveControlsToMeter (int nMeter)
{
	MeterSettings&		oMeter		= m_oSettings.aMeters[nMeter];
	CounterSettings&	oCounter	= oMeter.oCounter;

	oMeter.sName			= m_sMeterName;
	oMeter.nIconType		= m_nLEDType + 1;
	oMeter.nThresholdMid	= m_nThresholdMid;
	oMeter.nThresholdHigh	= m_nThresholdHigh;
	for (int i = 0; i < 3; i++)
	{
		oMeter.anColors[i]	= m_anColors[i];
	}

	oCounter.sPath			= m_sCounterPath.IsEmpty () ? m_sCounterPath : LocalPathToEnglish (m_sCounterPath);
	oCounter.d100Percent	= m_d100Percent;
	oCounter.nValueFormat	= m_nValueFormat + 1;
	oCounter.nScaleMode		= m_nScaleMode + 1;
	oCounter.bInvert		= (m_nInvert != 0);
	oCounter.nAlertPercent	= m_nAlertPercent;
	oCounter.nAlertSeconds	= m_nAlertSeconds;
	oCounter.nUpdateInterval	= m_nUpdateFrequency;
}

void CDialogConfig::UpdateSwatches ()
{
	for (int i = 0; i < 3; i++)
	{
		m_cColor[i].Invalidate ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	The 100% value: the text is converted to a number when it was edited. Returns FALSE (after telling the user) if it is not valid.
//
////////////////////////////////////////////////////////////////////////////////

BOOL CDialogConfig::Read100Percent (BOOL bReport)
{
	double	dValue	= 0;

	m_c100Percent.GetWindowText (m_s100Percent);

	// Not edited: the exact value is still in m_d100Percent
	if (m_s100Percent == m_s100PercentShown)
	{
		return TRUE;
	}

	if (!ParseScaleValue (m_nValueFormat + 1, m_s100Percent, dValue) || !(dValue > 0.0) || dValue > MAX_100PERCENT)
	{
		if (bReport)
		{
			m_cTab.SetCurSel (1);
			ShowPage (1);
			ShowMessage (this, TEXT ("The 100% value is not valid. Enter a number greater than 0, optionally with a unit such as Mbit/s or MB/s."), TEXT ("DiskLED: 100% value"), MB_OK | MB_ICONEXCLAMATION);
			m_c100Percent.SetFocus ();
		}
		return FALSE;
	}

	m_d100Percent		= dValue;
	m_s100PercentShown	= m_s100Percent;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The label of the 100% field depends on the scale: with Peak, Decay and Auto the value is only the minimum
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::UpdateScaleLabel ()
{
	int	nMode	= m_cScaleMode.GetCurSel () + 1;

	SetDlgItemText (IDC_100PERCENT_TEXT2, (nMode == SCALEMODE_FIXED) ? TEXT ("&100% utilization:") : TEXT ("&Minimum scale:"));
}


////////////////////////////////////////////////////////////////////////////////
//
//	For network counters: show the speed of the adapter and offer to use it as the 100% value
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::UpdateLinkSpeedHint ()
{
	CString	sPath	= m_sCounterPath.IsEmpty () ? m_sCounterPath : LocalPathToEnglish (m_sCounterPath);

	m_dLinkSpeed	= 0;
	if (!sPath.IsEmpty () && GetNetworkLinkSpeed (sPath, m_dLinkSpeed))
	{
		m_cLinkSpeedText.SetWindowText (TEXT ("Adapter speed: ") + FormatScaleValue (VALUEFORMAT_BITS, m_dLinkSpeed));
	}
	else
	{
		m_dLinkSpeed	= 0;
	}

	UpdateLinkSpeedVisibility ();
}

// The link speed controls belong to the scale page and only make sense for a network counter
void CDialogConfig::UpdateLinkSpeedVisibility ()
{
	int	nShow	= (m_nTab == 1 && m_dLinkSpeed > 0) ? SW_SHOW : SW_HIDE;

	m_cLinkSpeedText.ShowWindow (nShow);
	m_cUseLinkSpeed.ShowWindow (nShow);
}

void CDialogConfig::RefreshScaleControls ()
{
	UpdateScaleLabel ();
	UpdateLinkSpeedHint ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	A new data source was chosen: fill in a sensible value format, scale and 100% value. All can be changed afterwards.
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::ApplyCounterDefaults ()
{
	CounterSettings	oCounter	= m_oSettings.aMeters[m_nCounterSel].oCounter;

	GetCounterDefaults (LocalPathToEnglish (m_sCounterPath), oCounter);

	m_nValueFormat		= oCounter.nValueFormat - 1;
	m_nScaleMode		= oCounter.nScaleMode - 1;
	m_nInvert			= oCounter.bInvert ? 1 : 0;
	m_d100Percent		= oCounter.d100Percent;
	m_s100Percent		= FormatScaleValue (oCounter.nValueFormat, oCounter.d100Percent);
	m_s100PercentShown	= m_s100Percent;
	m_nUpdateFrequency	= oCounter.nUpdateInterval;

	// What was learned belongs to the previous data source
	m_oSettings.aMeters[m_nCounterSel].oCounter.dLearnedMax	= 0;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The value format changed: show the 100% value in the units of the new format
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnCbnSelchangeValueFormat ()
{
	CString	sText;
	double	dValue	= m_d100Percent;
	int		nNew	= m_cValueFormat.GetCurSel ();

	// m_nValueFormat is still the previous format: the text is in its units
	m_c100Percent.GetWindowText (sText);
	if (sText != m_s100PercentShown && ParseScaleValue (m_nValueFormat + 1, sText, dValue) && dValue > 0.0 && dValue <= MAX_100PERCENT)
	{
		m_d100Percent	= dValue;
	}

	if (nNew >= 0)
	{
		m_nValueFormat		= nNew;
		m_s100Percent		= FormatScaleValue (nNew + 1, m_d100Percent);
		m_s100PercentShown	= m_s100Percent;
		m_c100Percent.SetWindowText (m_s100Percent);
	}

	UpdatePreview ();
}

void CDialogConfig::OnCbnSelchangeScaleMode ()
{
	UpdateScaleLabel ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Use the adapter speed as the 100% value
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnBnClickedUseLinkSpeed ()
{
	int	nFormat	= m_cValueFormat.GetCurSel () + 1;

	if (!(m_dLinkSpeed > 0.0) || nFormat < VALUEFORMAT_PERCENT)
	{
		return;
	}

	m_cScaleMode.SetCurSel (SCALEMODE_FIXED - 1);
	m_nScaleMode		= SCALEMODE_FIXED - 1;
	m_d100Percent		= m_dLinkSpeed;
	m_s100Percent		= FormatScaleValue (nFormat, m_d100Percent);
	m_s100PercentShown	= m_s100Percent;
	m_c100Percent.SetWindowText (m_s100Percent);
	UpdateScaleLabel ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Quick pick: a common data source in one step. The counter, value format and scale are filled in like when choosing a counter in the lists.
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnCbnSelchangeQuickPick ()
{
	int		nPick	= m_cQuickPick.GetCurSel ();
	CString	sPath;
	CString	sLocalPath;

	if (nPick <= 0)
	{
		return;
	}
	m_cQuickPick.SetCurSel (0);

	switch (nPick)
	{
	case 1:
	case 2:
		{
			CString	sAdapter	= GetDefaultNetworkInstance ();

			if (!sAdapter.IsEmpty ())
			{
				sPath	= MakePerfPath (TEXT (""), TEXT ("Network Interface"), sAdapter, (nPick == 1) ? TEXT ("Bytes Received/sec") : TEXT ("Bytes Sent/sec"));
			}
		}
		break;
	case 3:
		sPath	= DEFAULT_COUNTER_1_PATH;
		break;
	case 4:
		sPath	= TEXT ("\\Processor(_Total)\\% Processor Time");
		break;
	case 5:
		sPath	= TEXT ("\\Memory\\% Committed Bytes In Use");
		break;
	}

	sLocalPath	= sPath.IsEmpty () ? sPath : EnglishPathToLocal (sPath);
	if (sLocalPath.IsEmpty () || PdhValidatePath (sLocalPath) != ERROR_SUCCESS)
	{
		ShowMessage (this, TEXT ("DiskLED could not find this data source on this computer. Please choose it with the lists below."), TEXT ("DiskLED: Quick pick"), MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	// Read the controls first, then replace the data source and everything that depends on it
	if (!UpdateData (TRUE))
	{
		return;
	}
	m_sCounterPath	= sLocalPath;
	ApplyCounterDefaults ();
	UpdateData (FALSE);
	RefreshScaleControls ();
	SelectPathInBrowser ();
	UpdatePreview ();
	NotifyChanged ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Initialize the dialog
//
////////////////////////////////////////////////////////////////////////////////

BOOL CDialogConfig::OnInitDialog ()
{
	// Call the base class's handler
	CDialog::OnInitDialog ();

	// Dark theme, if Windows apps use it
	m_oTheme.Update (m_hWnd);
	m_cTab.SetDark (m_oTheme.IsDark ());

	// Set up tooltip support
	if (m_oToolTip.Create (this))
	{
		m_oToolTip.AddTool (&m_cMeterList, IDS_TOOLTIP_COUNTERSEL);
		m_oToolTip.AddTool (&m_cMeterName, IDS_TOOLTIP_METERNAME);
		m_oToolTip.AddTool (&m_cSave, IDS_TOOLTIP_SAVE);
		m_oToolTip.AddTool (&m_cRevert, IDS_TOOLTIP_REVERT);
		m_oToolTip.AddTool (&m_cAddMeter, IDS_TOOLTIP_ADD_METER);
		m_oToolTip.AddTool (&m_cRemoveMeter, IDS_TOOLTIP_REMOVE_METER);
		m_oToolTip.AddTool (&m_cIconType, IDS_TOOLTIP_ICONTYPE);
		m_oToolTip.AddTool (&m_cValueFormat, IDS_TOOLTIP_VALUEFORMAT);
		m_oToolTip.AddTool (&m_cScaleMode, IDS_TOOLTIP_SCALEMODE);
		m_oToolTip.AddTool (&m_cInvert, IDS_TOOLTIP_INVERT);
		m_oToolTip.AddTool (&m_cAlertPercent, IDS_TOOLTIP_ALERT);
		m_oToolTip.AddTool (&m_cAlertSeconds, IDS_TOOLTIP_ALERT);
		m_oToolTip.AddTool (&m_cAutoStart, IDS_TOOLTIP_AUTOSTART);
		for (int i = 0; i < 3; i++)
		{
			m_oToolTip.AddTool (&m_cColor[i], IDS_TOOLTIP_COLORS);
		}
		m_oToolTip.AddTool (&m_cThresholdMid, IDS_TOOLTIP_THRESHOLDS);
		m_oToolTip.AddTool (&m_cThresholdHigh, IDS_TOOLTIP_THRESHOLDS);
		m_oToolTip.AddTool (&m_cUpdateFrequencyEdit, IDS_TOOLTIP_UPDATE_FREQUENCY);
		m_oToolTip.AddTool (&m_cDefaultValues, IDS_TOOLTIP_DEFAULT_VALUES);
		m_oToolTip.AddTool (&m_c100Percent, IDS_TOOLTIP_100PERCENT);
		m_oToolTip.AddTool (&m_cUseLinkSpeed, IDS_TOOLTIP_USE_LINKSPEED);
		m_oToolTip.AddTool (&m_cQuickPick, IDS_TOOLTIP_QUICKPICK);
		m_oToolTip.AddTool (&m_cBuildPathFromSelection, IDS_TOOLTIP_COUNTERPATH);
		m_oToolTip.AddTool (&m_cSelectPerfDescription, IDS_TOOLTIP_DESCRIPTION);
		m_oToolTip.AddTool (&m_cSelectPerfObjects, IDS_TOOLTIP_OBJECTS);
		m_oToolTip.AddTool (&m_cSelectPerfCounters, IDS_TOOLTIP_COUNTERS);
		m_oToolTip.AddTool (&m_cSelectPerfInstances, IDS_TOOLTIP_INSTANCES);

		// TODO: For some reason, the tooltip is not displayed on the text controls, although a dialog in VS shows it _can_ work somehow
		m_oToolTip.AddTool (&m_cUpdateFrequencyText1, IDS_TOOLTIP_UPDATE_FREQUENCY);
		m_oToolTip.AddTool (&m_cUpdateFrequencyText2, IDS_TOOLTIP_UPDATE_FREQUENCY);
	}

	// The meters are listed after the settings are loaded, see below

	// Icon type entries, in the order of the LEDTYPE_* constants
	m_cIconType.AddString (TEXT ("LED"));
	m_cIconType.AddString (TEXT ("Bar"));
	m_cIconType.AddString (TEXT ("Clean bar"));
	m_cIconType.AddString (TEXT ("VU meter"));
	m_cIconType.AddString (TEXT ("Graph"));
	m_cIconType.AddString (TEXT ("Number"));

	// Value format entries, in the order of the VALUEFORMAT_* constants
	m_cValueFormat.AddString (TEXT ("Percent"));
	m_cValueFormat.AddString (TEXT ("Number"));
	m_cValueFormat.AddString (TEXT ("Bytes/s"));
	m_cValueFormat.AddString (TEXT ("Bits/s"));
	m_cValueFormat.AddString (TEXT ("MBytes"));

	// Scale mode entries, in the order of the SCALEMODE_* constants
	m_cScaleMode.AddString (TEXT ("Fixed"));
	m_cScaleMode.AddString (TEXT ("Peak"));
	m_cScaleMode.AddString (TEXT ("Decay"));
	m_cScaleMode.AddString (TEXT ("Auto"));

	// Quick pick entries, see OnCbnSelchangeQuickPick
	m_cQuickPick.AddString (TEXT ("Choose..."));
	m_cQuickPick.AddString (TEXT ("Internet download speed"));
	m_cQuickPick.AddString (TEXT ("Internet upload speed"));
	m_cQuickPick.AddString (TEXT ("Disk activity"));
	m_cQuickPick.AddString (TEXT ("Processor use"));
	m_cQuickPick.AddString (TEXT ("Memory in use"));
	m_cQuickPick.SetCurSel (0);

	// Control value initialization. What is running is what was saved (or what the defaults are, if nothing was saved yet).
	if (m_pMainFrame)
	{
		m_pMainFrame->GetSettings (m_oSettings);
	}
	m_nCounterSel		= 0;
	m_nAutoStart		= IsAutoStartEnabled () ? 1 : 0;
	m_nAutoStartSaved	= m_nAutoStart;
	m_oSaved			= m_oSettings;
	m_oApplied			= m_oSettings;
	m_cMeterName.SetLimitText (MAX_METER_NAME);

	m_bLoading	= TRUE;
	LoadMeterToControls (m_nCounterSel);
	FillMeterList ();

	// Update the dialog with the data written to the member variables
	UpdateData (FALSE);
	m_bLoading	= FALSE;
	RefreshScaleControls ();

	// The pages
	m_cTab.InsertItem (0, TEXT ("Data source"));
	m_cTab.InsertItem (1, TEXT ("Scale and values"));
	m_cTab.InsertItem (2, TEXT ("Appearance"));
	m_cTab.InsertItem (3, TEXT ("Alerts"));
	m_cTab.SetCurSel (0);
	ShowPage (0);

	// Set the initial focus to the name of the meter
	m_cMeterName.SetFocus ();

	// The hidden main window is not a place to center on (it may be on another monitor)
	CenterOnPointerMonitor (this);

	// Initialize the performance browser controls
	InitPerfBrowser ();

	// Start the preview
	SetTimer (PREVIEW_TIMER_ID, PREVIEW_INTERVAL_MS, NULL);
	UpdatePreview ();
	UpdateDirtyState ();

	// The first start found older copies: the wizard opens at the import
	if (m_bFirstRunImport)
	{
		PostMessage (WM_IMPORT_WIZARD);
	}

	// The focus was set by hand
	return FALSE;
}


void CDialogConfig::OnDestroy ()
{
	KillTimer (PREVIEW_TIMER_ID);
	KillTimer (CHANGE_TIMER_ID);

	CDialog::OnDestroy ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Dark theme: the dialog colours the background and the texts of its controls, and follows the Windows setting while it is open
//
////////////////////////////////////////////////////////////////////////////////

HBRUSH CDialogConfig::OnCtlColor (CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	int		nID		= pWnd->GetDlgCtrlID ();
	BOOL	bField	= (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX || nID == IDC_COUNTERPATH_EDIT || nID == IDC_PERF_DESCRIPTION);
	HBRUSH	hBrush	= m_oTheme.CtlColor (pDC, bField);

	return hBrush ? hBrush : CDialog::OnCtlColor (pDC, pWnd, nCtlColor);
}

void CDialogConfig::OnSettingChange (UINT uFlags, LPCTSTR lpszSection)
{
	CDialog::OnSettingChange (uFlags, lpszSection);

	if (m_hWnd && lpszSection && _tcscmp (lpszSection, TEXT ("ImmersiveColorSet")) == 0 && m_oTheme.Update (m_hWnd))
	{
		m_cTab.SetDark (m_oTheme.IsDark ());
		UpdatePreview ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Reading the controls. An entry that is not valid (e.g. a number that is still being typed) is reported only when asked to (bReport),
//	otherwise the controls are not used and the tray icons keep what they show.
//
////////////////////////////////////////////////////////////////////////////////

BOOL CDialogConfig::ReadControls (BOOL bReport)
{
	// Combo boxes, check boxes and texts. The numbers are not part of this exchange, see DoDataExchange.
	if (!UpdateData (TRUE))
	{
		return FALSE;
	}

	struct NumberField
	{
		UINT			nID;
		int				nPage;
		const TCHAR*	sName;
	};
	static const NumberField aFields[] =
	{
		{ IDC_EDIT_THRESHOLD_MID,		2,	TEXT ("the mid colour threshold") },
		{ IDC_EDIT_THRESHOLD_HIGH,		2,	TEXT ("the high colour threshold") },
		{ IDC_UPDATEFREQUENCY_EDIT,		2,	TEXT ("the update frequency") },
		{ IDC_EDIT_ALERT_PERCENT,		3,	TEXT ("the warning percentage") },
		{ IDC_EDIT_ALERT_SECONDS,		3,	TEXT ("the warning time") }
	};
	UINT	anValues[_countof (aFields)];

	for (int i = 0; i < _countof (aFields); i++)
	{
		BOOL	bTranslated	= FALSE;

		anValues[i]	= GetDlgItemInt (aFields[i].nID, &bTranslated, FALSE);
		if (!bTranslated)
		{
			if (bReport)
			{
				m_cTab.SetCurSel (aFields[i].nPage);
				ShowPage (aFields[i].nPage);
				ShowMessage (this, CString (TEXT ("Please enter a whole number for ")) + aFields[i].sName + TEXT ("."), TEXT ("DiskLED: Settings"), MB_OK | MB_ICONEXCLAMATION);
				GetDlgItem (aFields[i].nID)->SetFocus ();
			}
			return FALSE;
		}
	}
	m_nThresholdMid		= (int) anValues[0];
	m_nThresholdHigh	= (int) anValues[1];
	m_nUpdateFrequency	= anValues[2];
	m_nAlertPercent		= (int) anValues[3];
	m_nAlertSeconds		= (int) anValues[4];

	return Read100Percent (bReport);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Show the settings in the controls: the meter that is selected and the settings for all meters. This is not a change by the user.
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::ShowMeterInControls ()
{
	m_bLoading	= TRUE;

	LoadMeterToControls (m_nCounterSel);
	FillMeterList ();
	UpdateData (FALSE);
	RefreshScaleControls ();
	UpdateSwatches ();
	SelectPathInBrowser ();
	UpdatePreview ();
	UpdateDirtyState ();

	m_bLoading	= FALSE;
	KillTimer (CHANGE_TIMER_ID);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Live preview: a change in the controls is shown on the tray icons a moment after the last change. Nothing is stored until Save.
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::NotifyChanged ()
{
	if (!m_bLoading && m_hWnd)
	{
		SetTimer (CHANGE_TIMER_ID, CHANGE_DELAY_MS, NULL);
	}
}

BOOL CDialogConfig::CommitChanges ()
{
	KillTimer (CHANGE_TIMER_ID);

	// An entry that is not complete yet: wait for the next change
	if (!ReadControls (FALSE))
	{
		return FALSE;
	}

	SaveControlsToMeter (m_nCounterSel);

	if (m_pMainFrame && !AreSettingsEqual (m_oSettings, m_oApplied))
	{
		m_pMainFrame->ApplySettings (m_oSettings);
		m_oApplied	= m_oSettings;
	}

	// The list shows the name or the counter of the meter
	CString	sText	= GetMeterListText (m_nCounterSel);
	CString	sShown;

	m_cMeterList.GetText (m_nCounterSel, sShown);
	if (sText != sShown)
	{
		m_cMeterList.DeleteString (m_nCounterSel);
		m_cMeterList.InsertString (m_nCounterSel, sText);
		m_cMeterList.SetCurSel (m_nCounterSel);
	}

	UpdateDirtyState ();

	return TRUE;
}

// Are there changes that were not saved? Save and Revert only make sense then.
void CDialogConfig::UpdateDirtyState ()
{
	BOOL	bDirty	= !AreSettingsEqual (m_oSettings, m_oSaved) || m_nAutoStart != m_nAutoStartSaved;

	m_bDirty	= bDirty;

	// A disabled button must not keep the keyboard focus
	HWND	hFocus	= ::GetFocus ();
	if (!bDirty && hFocus != NULL && (hFocus == m_cSave.m_hWnd || hFocus == m_cRevert.m_hWnd))
	{
		m_cMeterList.SetFocus ();
	}

	m_cSave.EnableWindow (bDirty);
	m_cRevert.EnableWindow (bDirty);
	m_cStatus.SetWindowText (bDirty ? TEXT ("Unsaved changes, shown live on the tray icons. Save to keep them.") : TEXT (""));
	SetWindowText (bDirty ? TEXT ("DiskLED Configuration *") : TEXT ("DiskLED Configuration"));
}

// Any change in the controls that belongs to a meter or to the settings is a change to preview. The buttons that manage the meters and the settings do their own work.
BOOL CDialogConfig::OnCommand (WPARAM wParam, LPARAM lParam)
{
	UINT	nID		= LOWORD (wParam);
	UINT	nCode	= HIWORD (wParam);

	if (lParam != 0 && (nCode == EN_CHANGE || nCode == CBN_SELCHANGE || nCode == BN_CLICKED))
	{
		switch (nID)
		{
		case IDOK:
		case IDCANCEL:
		case IDC_SAVE:
		case IDC_REVERT:
		case IDC_ADD_METER:
		case IDC_REMOVE_METER:
		case IDC_LIST_METERS:
		case IDC_DEFAULTVALUES:
			break;
		default:
			NotifyChanged ();
		}
	}

	return CDialog::OnCommand (wParam, lParam);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Save: keep what the tray icons show. Returns FALSE if there is an invalid entry or the file cannot be written.
//
////////////////////////////////////////////////////////////////////////////////

BOOL CDialogConfig::SaveNow ()
{
	if (!ReadControls (TRUE))
	{
		return FALSE;
	}

	SaveControlsToMeter (m_nCounterSel);
	KillTimer (CHANGE_TIMER_ID);

	if (m_pMainFrame)
	{
		// The running settings are the corrected ones, and include what the auto scale has learned meanwhile
		m_pMainFrame->ApplySettings (m_oSettings);
		m_pMainFrame->GetSettings (m_oSettings);
		if (m_nCounterSel >= (int) m_oSettings.aMeters.size ())
		{
			m_nCounterSel	= (int) m_oSettings.aMeters.size () - 1;
		}
		m_oApplied	= m_oSettings;
	}

	// Autostart is stored in the registry, not in the INI file
	DWORD	nAutoStartError	= SetAutoStart (m_nAutoStart != 0);
	if (nAutoStartError != ERROR_SUCCESS && m_pMainFrame)
	{
		m_pMainFrame->LogError (TEXT ("Save"), TEXT ("SetAutoStart"), nAutoStartError);
	}

	if (m_pMainFrame)
	{
		CString	sINIFile;
		m_pMainFrame->GetINIFile (sINIFile);

		DWORD	nError	= SaveSettings (sINIFile, m_oSettings);
		if (nError != ERROR_SUCCESS)
		{
			CString	sExplanation	= (nError == ERROR_ACCESS_DENIED) ? TEXT ("Access denied - could not write to the INI file: <") : TEXT ("Could not write to the INI file: <");

			m_pMainFrame->LogError (TEXT ("Save"), TEXT ("WriteINIValue"), nError, sExplanation + sINIFile + TEXT (">"));
			ShowMeterInControls ();
			return FALSE;
		}
	}

	m_oSaved			= m_oSettings;
	m_nAutoStartSaved	= m_nAutoStart;

	// The entries of the older copies that were imported go with the Save
	RemoveRunEntries (m_asPendingRunRemovals);
	m_asPendingRunRemovals.RemoveAll ();
	ShowMeterInControls ();

	return TRUE;
}

void CDialogConfig::OnBnClickedSave ()
{
	SaveNow ();
}

void CDialogConfig::OnOK ()
{
	// The Enter key
	if (m_bDirty)
	{
		SaveNow ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Revert: back to what was saved last
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnBnClickedRevert ()
{
	m_oSettings		= m_oSaved;
	m_nAutoStart	= m_nAutoStartSaved;
	m_asPendingRunRemovals.RemoveAll ();
	if (m_nCounterSel >= (int) m_oSettings.aMeters.size ())
	{
		m_nCounterSel	= (int) m_oSettings.aMeters.size () - 1;
	}

	if (m_pMainFrame)
	{
		m_pMainFrame->ApplySettings (m_oSettings);
	}
	m_oApplied	= m_oSettings;

	ShowMeterInControls ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	The button "Default values" was clicked
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnBnClickedDefaultvalues()
{
	// Default settings for everything except the autostart, which is not part of the settings. A single meter is left.
	SetDefaultSettings (m_oSettings);
	m_nCounterSel	= 0;
	ShowMeterInControls ();
	CommitChanges ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Close (also the Escape key and the close button of the window): unsaved changes can be saved or discarded
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnCancel ()
{
	// Entries that were just made count
	CommitChanges ();

	if (m_bDirty)
	{
		int	nChoice	= ShowChoice (this, TEXT ("You have unsaved changes. They are shown on the tray icons now."), TEXT ("DiskLED"), IDI_WARNING,
			TEXT ("Save"), TEXT ("Discard"), TEXT ("Cancel"), 3);

		if (nChoice == 1)
		{
			if (!SaveNow ())
			{
				return;
			}
		}
		else if (nChoice == 2)
		{
			// The tray icons go back to what was saved
			if (m_pMainFrame)
			{
				m_pMainFrame->ApplySettings (m_oSaved);
			}
		}
		else
		{
			return;
		}
	}

	CDialog::OnCancel ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	The list of meters: "<number>: <name>", or "<number>: <counter name> (<instance>)" for a meter without a name
//
////////////////////////////////////////////////////////////////////////////////

CString CDialogConfig::GetMeterListText (int nMeter)
{
	const MeterSettings&	oMeter	= m_oSettings.aMeters[nMeter];
	CString					sText;

	if (!oMeter.sName.IsEmpty ())
	{
		sText.Format (TEXT ("%d: %s"), oMeter.nId, (LPCTSTR) oMeter.sName);
		return sText;
	}

	sText.Format (TEXT ("%d: %s"), oMeter.nId, (LPCTSTR) GetCounterDisplayName (oMeter.oCounter, TRUE));

	return sText;
}

void CDialogConfig::FillMeterList ()
{
	m_cMeterList.ResetContent ();

	for (size_t i = 0; i < m_oSettings.aMeters.size (); i++)
	{
		m_cMeterList.AddString (GetMeterListText ((int) i));
	}

	m_cMeterList.SetCurSel (m_nCounterSel);
	UpdateMeterButtons ();
}

void CDialogConfig::UpdateMeterButtons ()
{
	m_cAddMeter.EnableWindow (GetFreeMeterId (m_oSettings) != 0);
	m_cRemoveMeter.EnableWindow (m_oSettings.aMeters.size () > 1);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Another meter was selected: keep the settings of the previous one, show the settings of the new one
//	Stays with the previous meter if its entries are not valid
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnLbnSelchangeMeterList ()
{
	int	nNew	= m_cMeterList.GetCurSel ();

	if (nNew < 0 || nNew >= (int) m_oSettings.aMeters.size () || nNew == m_nCounterSel)
	{
		return;
	}

	if (!ReadControls (TRUE))
	{
		m_cMeterList.SetCurSel (m_nCounterSel);
		return;
	}

	SaveControlsToMeter (m_nCounterSel);
	m_nCounterSel	= nNew;
	ShowMeterInControls ();
	CommitChanges ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Add a meter. It starts as a copy of the current one (e.g. download and upload of the same adapter), which can be changed afterwards.
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnBnClickedAddMeter ()
{
	ShowWizard (FALSE);
}

// Posted when the window opens for the import of the first start
LRESULT CDialogConfig::OnImportWizard (WPARAM, LPARAM)
{
	ShowWizard (TRUE);

	return 0;
}

void CDialogConfig::ShowWizard (BOOL bStartWithImport)
{
	int	nId	= GetFreeMeterId (m_oSettings);

	if (nId == 0)
	{
		ShowMessage (this, TEXT ("There can be at most 16 meters. Remove one to add another."), TEXT ("DiskLED: Add a meter"), MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	if (!ReadControls (TRUE))
	{
		return;
	}
	SaveControlsToMeter (m_nCounterSel);

	// The wizard makes the new meter, or takes over meters of older copies. "Another counter" makes a copy of the current meter, to be set up with the lists.
	CDialogWizard	oWizard (this, m_oSettings, bStartWithImport);

	if (oWizard.DoModal () != IDOK)
	{
		return;
	}

	if (oWizard.m_bImport)
	{
		int	nSkipped	= 0;

		// At the first start the default meter is only a placeholder: the imported meters replace it
		DiskLEDSettings	oDefault;

		SetDefaultSettings (oDefault);
		if (m_bFirstRunImport && AreSettingsEqual (m_oSettings, oDefault))
		{
			m_oSettings.aMeters.clear ();
		}
		m_bFirstRunImport	= FALSE;

		int	nFirstId	= 0;
		for (size_t i = 0; i < oWizard.m_aImported.size (); i++)
		{
			MeterSettings	oMeter	= oWizard.m_aImported[i];

			oMeter.nId	= GetFreeMeterId (m_oSettings);
			if (oMeter.nId == 0)
			{
				nSkipped++;
				continue;
			}
			nFirstId	= (nFirstId == 0) ? oMeter.nId : nFirstId;
			m_oSettings.aMeters.push_back (oMeter);
			NormalizeSettings (m_oSettings);
		}

		// What the user asked for is done when the settings are saved
		for (int i = 0; i < oWizard.m_asRunEntries.GetSize (); i++)
		{
			m_asPendingRunRemovals.Add (oWizard.m_asRunEntries[i]);
		}

		for (size_t i = 0; i < m_oSettings.aMeters.size (); i++)
		{
			if (m_oSettings.aMeters[i].nId == nFirstId)
			{
				m_nCounterSel	= (int) i;
			}
		}
		if (m_nCounterSel >= (int) m_oSettings.aMeters.size ())
		{
			m_nCounterSel	= 0;
		}

		ShowMeterInControls ();
		CommitChanges ();
		if (nSkipped > 0)
		{
			CString	sText;

			sText.Format (TEXT ("There can be at most 16 meters, %d of the imported meters could not be added."), nSkipped);
			ShowMessage (this, sText, TEXT ("DiskLED: Import"), MB_OK | MB_ICONEXCLAMATION);
		}
		m_cMeterList.SetFocus ();
		return;
	}

	MeterSettings	oMeter	= oWizard.m_bManual ? m_oSettings.aMeters[m_nCounterSel] : oWizard.m_oMeter;

	oMeter.nId						= nId;
	oMeter.oCounter.dLearnedMax		= 0;
	if (oWizard.m_bManual)
	{
		oMeter.sName	= TEXT ("");
	}
	m_oSettings.aMeters.push_back (oMeter);
	NormalizeSettings (m_oSettings);

	// The list is sorted by number: find the new one
	for (size_t i = 0; i < m_oSettings.aMeters.size (); i++)
	{
		if (m_oSettings.aMeters[i].nId == nId)
		{
			m_nCounterSel	= (int) i;
		}
	}

	ShowMeterInControls ();
	CommitChanges ();
	if (oWizard.m_bManual)
	{
		m_cTab.SetCurSel (0);
		ShowPage (0);
		m_cQuickPick.SetFocus ();
	}
	else
	{
		m_cMeterName.SetFocus ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Remove the meter that is shown. There is always at least one. No question: Revert brings it back.
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnBnClickedRemoveMeter ()
{
	if (m_oSettings.aMeters.size () <= 1)
	{
		return;
	}

	m_oSettings.aMeters.erase (m_oSettings.aMeters.begin () + m_nCounterSel);
	if (m_nCounterSel >= (int) m_oSettings.aMeters.size ())
	{
		m_nCounterSel	= (int) m_oSettings.aMeters.size () - 1;
	}

	ShowMeterInControls ();
	CommitChanges ();
	m_cMeterList.SetFocus ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	A colour swatch was clicked: let the user pick a colour
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnColorButton (UINT nID)
{
	int				nIndex	= nID - IDC_COLOR_LOW;
	CColorDialog	oDialog (m_anColors[nIndex], CC_FULLOPEN | CC_RGBINIT, this);

	if (oDialog.DoModal () == IDOK)
	{
		m_anColors[nIndex]	= oDialog.GetColor ();
		m_cColor[nIndex].Invalidate ();
		NotifyChanged ();
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Preview: the icon as it would look with the current choices, animated with a test signal
//	Icon type, colours, thresholds, value format and the second counter are taken from the controls as they are right now
//
////////////////////////////////////////////////////////////////////////////////

static double TriangleWave (double dTime, double dPeriod)
{
	double	dPhase	= fmod (dTime, dPeriod) / dPeriod;

	return (dPhase < 0.5) ? dPhase * 2.0 : (1.0 - dPhase) * 2.0;
}

void CDialogConfig::UpdatePreview ()
{
	MeterSettings	oMeter;
	BOOL			bTranslated;

	// Style from the controls, invalid thresholds are corrected like they would be when applying
	SetDefaultMeterSettings (oMeter, 1);
	oMeter.nIconType		= m_cIconType.GetCurSel () + 1;
	oMeter.nThresholdMid	= GetDlgItemInt (IDC_EDIT_THRESHOLD_MID, &bTranslated, FALSE);
	oMeter.nThresholdHigh	= GetDlgItemInt (IDC_EDIT_THRESHOLD_HIGH, &bTranslated, FALSE);
	for (int i = 0; i < 3; i++)
	{
		oMeter.anColors[i]	= m_anColors[i];
	}
	NormalizeMeterSettings (oMeter);
	MakeIconStyle (oMeter, m_oPreviewStyle);

	// A test signal in the units of the selected value format
	DWORD	nValueFormat	= m_cValueFormat.GetCurSel () + 1;
	double	dMax			= 100;
	switch (nValueFormat)
	{
	case VALUEFORMAT_BYTES:
	case VALUEFORMAT_BITS:
		dMax	= 12500000;
		break;
	case VALUEFORMAT_MBYTES:
		dMax	= 16384;
		break;
	}

	m_dPreviewTime		+= PREVIEW_INTERVAL_MS / 1000.0;

	double	dValue	= dMax * TriangleWave (m_dPreviewTime, 10.0);

	memmove (m_oPreview.adHistory, m_oPreview.adHistory + 1, (HISTORY_SIZE - 1) * sizeof (double));
	m_oPreview.adHistory[HISTORY_SIZE - 1]	= dValue;
	m_oPreview.dValue						= dValue;
	m_oPreview.dScale						= dMax;
	m_oPreview.nValueFormat					= nValueFormat;

	if (m_cPreview.m_hWnd)
	{
		m_cPreview.Invalidate (FALSE);
	}
}

void CDialogConfig::OnTimer (UINT_PTR nIDEvent)
{
	if (nIDEvent == PREVIEW_TIMER_ID)
	{
		UpdatePreview ();
		return;
	}
	if (nIDEvent == CHANGE_TIMER_ID)
	{
		CommitChanges ();
		return;
	}

	CDialog::OnTimer (nIDEvent);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Draw the owner-drawn controls: the colour swatches and the preview
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnDrawItem (int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CDC*	pDC		= CDC::FromHandle (lpDrawItemStruct->hDC);
	CRect	oRect	(lpDrawItemStruct->rcItem);

	if (nIDCtl == IDC_PREVIEW)
	{
		int		nSize	= GetSystemMetrics (SM_CXSMICON);
		int		nZoom	= ((oRect.Width () < oRect.Height () ? oRect.Width () : oRect.Height ()) - 2) / nSize;
		CDC		oMemDC;
		CBitmap	oBitmap;

		if (nZoom < 1)
		{
			nZoom	= 1;
		}

		pDC->FillSolidRect (oRect, m_oTheme.IsDark () ? DARK_COLOR_BACK : GetSysColor (COLOR_BTNFACE));

		if (oMemDC.CreateCompatibleDC (pDC) && oBitmap.CreateCompatibleBitmap (pDC, nSize, nSize))
		{
			CBitmap*	pOldBitmap	= oMemDC.SelectObject (&oBitmap);

			// Enlarge without smoothing so the individual pixels are visible
			pDC->SetStretchBltMode (COLORONCOLOR);
			DrawIconImage (&oMemDC, nSize, m_oPreviewStyle, &m_oPreview, 1);
			pDC->StretchBlt (oRect.left + (oRect.Width () - nSize * nZoom) / 2, oRect.top + (oRect.Height () - nSize * nZoom) / 2,
				nSize * nZoom, nSize * nZoom, &oMemDC, 0, 0, nSize, nSize, SRCCOPY);

			oMemDC.SelectObject (pOldBitmap);
		}
		return;
	}

	if (nIDCtl < IDC_COLOR_LOW || nIDCtl > IDC_COLOR_HIGH)
	{
		CDialog::OnDrawItem (nIDCtl, lpDrawItemStruct);
		return;
	}

	pDC->FillSolidRect (oRect, m_anColors[nIDCtl - IDC_COLOR_LOW]);
	pDC->Draw3dRect (oRect, GetSysColor (COLOR_BTNSHADOW), GetSysColor (COLOR_BTNSHADOW));
	if (lpDrawItemStruct->itemState & ODS_FOCUS)
	{
		oRect.DeflateRect (2, 2);
		pDC->DrawFocusRect (oRect);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Enumerate all performance objects and add them to the appropriate list boxes
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::InitPerfBrowser ()
{
	PDH_STATUS		nError			= ERROR_INVALID_FUNCTION;
	TCHAR*			sBuffer			= NULL;
	DWORD			nBuffer			= 0;

	// Get the size of the buffer required
	nError	= PdhEnumObjects (NULL, NULL, sBuffer, &nBuffer, PERF_DETAIL_WIZARD, TRUE);
	if (nError != PDH_MORE_DATA)
	{
		m_pMainFrame->LogError (TEXT ("InitPerfBrowser"), TEXT ("PdhEnumObjects (1)"), nError);
		goto CleanUp;
	}

	// Allocate memory
	nBuffer++;			// For 2000/XP
	sBuffer	= (TCHAR*) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (TCHAR) * nBuffer);
	if (nBuffer == NULL)
	{
		m_pMainFrame->LogError (TEXT ("InitPerfBrowser"), TEXT ("HeapAlloc"), ERROR_OUTOFMEMORY);
		goto CleanUp;
	}

	// Now get the list of perf objects
	nError	= PdhEnumObjects (NULL, NULL, sBuffer, &nBuffer, PERF_DETAIL_WIZARD, FALSE);
	if (nError != ERROR_SUCCESS || sBuffer == NULL)
	{
		m_pMainFrame->LogError (TEXT ("InitPerfBrowser"), TEXT ("PdhEnumObjects (2)"), nError);
		goto CleanUp;
	}

	// Add the object names we got to the UI
	for (TCHAR* sName = sBuffer; *sName != 0; sName += _tcscnlen (sName, nBuffer) + 1)
	{
		m_cSelectPerfObjects.AddString (sName);
	}

	// Select the counter path in the UI, if there is one
	SelectPathInBrowser ();

CleanUp:
	if (sBuffer)
	{
		HeapFree(GetProcessHeap (), 0, sBuffer);
		sBuffer	= NULL;
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	Show the counter path (in the edit box) in the browser: select its object, counter and instance
//	Without a path, nothing is selected
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::SelectPathInBrowser ()
{
	CString	sMachine;
	CString	sObject;
	CString	sInstance;
	CString	sCounter;

	m_cSelectPerfObjects.SetCurSel (-1);
	m_cSelectPerfCounters.ResetContent ();
	m_cSelectPerfInstances.ResetContent ();

	if (!m_sCounterPath.IsEmpty () && ParsePerfPath (m_sCounterPath, sMachine, sObject, sInstance, sCounter))
	{
		// Select the object in the list. This fills and selects counters and instances.
		int	nIndex	= m_cSelectPerfObjects.FindStringExact (-1, sObject);
		if (nIndex != CB_ERR)
		{
			m_cSelectPerfObjects.SetCurSel (nIndex);
			OnCbnSelchangePerfObjects ();
			return;
		}
	}

	// No path, or its object is not in the list: nothing is selected
	m_sPerfObject.Empty ();
	m_sPerfCounter.Empty ();
	m_sPerfInstance.Empty ();
	m_sPerfDescription.Empty ();
	m_sSelectedPerfPath.Empty ();
	UpdateData (FALSE);
}


////////////////////////////////////////////////////////////////////////////////
//
//	When a new perf object is selected, enumerate object items
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnCbnSelchangePerfObjects ()
{
	PDH_STATUS		nError				= ERROR_INVALID_FUNCTION;
	CString			sObject;
	CString			sCounter;
	CString			sInstance;
	DWORD			nBufferCounters		= 0;
	DWORD			nBufferInstances	= 0;
	CMap<CString, LPCTSTR, DWORD, DWORD>	oInstances;

	// Remove all items from the combo boxes
	m_cSelectPerfCounters.ResetContent ();
	m_cSelectPerfInstances.ResetContent ();

	// Get data from the dialog box into our member variables
	if (!UpdateData (TRUE))
	{
		return;
	}

	// Zero the buffer memory allocated earlier
	if (m_sBufferCounters)
	{
		SecureZeroMemory (m_sBufferCounters, m_nBufferCounters);
	}
	if (m_sBufferInstances)
	{
		SecureZeroMemory (m_sBufferInstances, m_nBufferInstances);
	}

	// Is an item selected?
	if (m_sPerfObject.GetLength () == 0)
	{
		return;
	}

	// Get the sizes of the buffers required
	nError	= PdhEnumObjectItems (NULL, NULL, m_sPerfObject, NULL, &nBufferCounters, NULL, &nBufferInstances, PERF_DETAIL_WIZARD, 0);
	if (nError != PDH_MORE_DATA)
	{
		m_pMainFrame->LogError (TEXT ("OnCbnSelchangePerfObjects"), TEXT ("PdhEnumObjectItems (1)"), nError);
		goto CleanUp;
	}

	//
	// Allocate memory if necessary
	//
	if (nBufferCounters > m_nBufferCounters)
	{
		// Free memory of previously allocated
		if (m_sBufferCounters)
		{
			HeapFree (GetProcessHeap (), 0, m_sBufferCounters);
			m_sBufferCounters	= NULL;
		}

		// Allocate more than required
		m_nBufferCounters	= nBufferCounters * 2;
		nBufferCounters		= m_nBufferCounters;

		m_sBufferCounters		= (TCHAR*) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (TCHAR) * m_nBufferCounters);
		if (m_sBufferCounters == NULL)
		{
			m_pMainFrame->LogError (TEXT ("OnCbnSelchangePerfObjects"), TEXT ("HeapAlloc (1) failed!"), 0);
			goto CleanUp;
		}
	}

	if (nBufferInstances > m_nBufferInstances || m_sBufferInstances == NULL)
	{
		// Free memory of previously allocated
		if (m_sBufferInstances)
		{
			HeapFree (GetProcessHeap (), 0, m_sBufferInstances);
			m_sBufferInstances	= NULL;
		}

		// Allocate more than required (objects without instances, e.g. Memory, report a size of 0 but still need a valid buffer)
		m_nBufferInstances	= (nBufferInstances > 1) ? nBufferInstances * 2 : 2;
		nBufferInstances	= m_nBufferInstances;

		m_sBufferInstances		= (TCHAR*) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (TCHAR) * m_nBufferInstances);
		if (m_sBufferInstances == NULL)
		{
			m_pMainFrame->LogError (TEXT ("OnCbnSelchangePerfObjects"), TEXT ("HeapAlloc (2) failed!"), 0);
			goto CleanUp;
		}
	}

	// Now get the list of counters and instances
	nError	= PdhEnumObjectItems (NULL, NULL, m_sPerfObject, m_sBufferCounters, &nBufferCounters, m_sBufferInstances, &nBufferInstances, PERF_DETAIL_WIZARD, 0);
	if (nError != ERROR_SUCCESS || m_sBufferCounters == NULL || m_sBufferInstances == NULL)
	{
		m_pMainFrame->LogError (TEXT ("OnCbnSelchangePerfObjects"), TEXT ("PdhEnumObjectItems (2)"), nError);
		goto CleanUp;
	}

	// Add the counter names we got to the UI
	for (TCHAR* sName = m_sBufferCounters; *sName != 0; sName += _tcscnlen (sName, nBufferCounters) + 1)
	{
		m_cSelectPerfCounters.AddString (sName);
	}

	// Add the instance names we got to the UI
	// Some objects (e.g. processes or threads) can have multiple instances. In that case, we get multiple duplicate instance names. Add a zero-based index.
	// The first instance does not get an index, the second gets #1 appended, the third #2 and so on.
	TCHAR sBuffer[10];
	for (TCHAR* sName = m_sBufferInstances; *sName != 0; sName += _tcscnlen (sName, nBufferCounters) + 1)
	{
		DWORD	nValue	= 0;
		oInstances.Lookup (sName, nValue);
		nValue++;
		oInstances[sName]	= nValue;
		sInstance	= sName;
		if (nValue > 1)
		{
			sInstance	+= TEXT ("#");
			_itot_s (nValue - 1, sBuffer, 10, 10);
			sInstance	+= sBuffer;
		}
		m_cSelectPerfInstances.AddString (sInstance);
	}
	sInstance.Empty ();

	// Make sure initially nothing is selected in the counter and instance combo boxes
	// (This does not trigger the selection change event handler)
	m_cSelectPerfCounters.SetCurSel (-1);
	m_cSelectPerfInstances.SetCurSel (-1);

	// Do we have a counter path to select in the UI?
	if (m_sCounterPath.GetLength () == 0)
	{
		goto CleanUp;
	}

	// Select the items in the lists
	if (SplitPerfPath (m_sCounterPath, sObject, sCounter, sInstance) == ERROR_SUCCESS)
	{
		// Is the object selected the one displayed in the edit box?
		if (sObject.CompareNoCase (m_sPerfObject) == 0)
		{
			// Select the counter
			int	nFound	= m_cSelectPerfCounters.FindStringExact (-1, sCounter);
			if (nFound != CB_ERR)
			{
				m_cSelectPerfCounters.SetCurSel (nFound);
			}

			// Select the instance
			nFound	= sInstance.IsEmpty () ? CB_ERR : m_cSelectPerfInstances.FindStringExact (-1, sInstance);
			if (nFound != CB_ERR)
			{
				m_cSelectPerfInstances.SetCurSel (nFound);
			}
		}
	}

	// Deal with the change of the counter and instance list boxes - the event handler are for some reason not called directly (sad, but documented)
	// Do it manually (sigh)
	CDialogConfig::OnCbnSelchangePerfCounters ();
	CDialogConfig::OnCbnSelchangePerfInstances ();

CleanUp:
	return;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Split a perf path into its components
//
////////////////////////////////////////////////////////////////////////////////

DWORD CDialogConfig::SplitPerfPath (CString sCounterPath, CString& sObject, CString& sCounter, CString& sInstance)
{
	CString	sMachine;

	if (!ParsePerfPath (sCounterPath, sMachine, sObject, sInstance, sCounter))
	{
		m_pMainFrame->LogError (TEXT ("SplitPerfPath"), TEXT ("The counter path could not be split into its elements!"), 0);
		return ERROR_INVALID_FUNCTION;
	}

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Build a perf path out of path elements
//
////////////////////////////////////////////////////////////////////////////////

DWORD CDialogConfig::BuildPerfPath (CString& sCounterPath, CString sObject, CString sCounter, CString sInstance)
{
	PDH_STATUS			nError			= ERROR_INVALID_FUNCTION;
	PDH_HQUERY			hQuery			= NULL;
	PDH_HCOUNTER		hCounter		= NULL;

	if (sInstance.IsEmpty ())
	{
		sCounterPath	= TEXT ("\\") + sObject + TEXT ("\\") + sCounter;
	}
	else
	{
		sCounterPath	= TEXT ("\\") + sObject + TEXT ("(") + sInstance + TEXT (")") + TEXT ("\\") + sCounter;
	}

	// Check the path we built for validity
	// Open a query handle
	nError	= PdhOpenQuery (NULL, 0, &hQuery);
	if(nError != ERROR_SUCCESS)
	{
		m_pMainFrame->LogError (TEXT ("BuildPerfPath"), TEXT ("PdhOpenQuery"), nError);
		goto CleanUp;
	}

	// Add a counter to the query in order to get a handle to the counter
	nError	= PdhAddCounter (hQuery, sCounterPath, 0, &hCounter);
	if (nError != ERROR_SUCCESS)
	{
		if (nError == PDH_CSTATUS_BAD_COUNTERNAME)
		{
			// If instance is empty, try again with instance set to an asterisk
			if (sInstance.IsEmpty ())
			{
				return BuildPerfPath (sCounterPath, sObject, sCounter, TEXT ("*"));
			}
		}
	}

CleanUp:
	if (hQuery != NULL)
	{
		PdhCloseQuery (hQuery);
		hQuery	= NULL;
	}

	return nError;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Use a selected data source: store the path in the edit box
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnBnClickedUseSelectedDataSource()
{
	// Get data from the dialog box into our member variables
	if (!UpdateData (TRUE))
	{
		return;
	}

	// Did the user select a valid data source?
	if (m_sPerfObject.IsEmpty ())
	{
		ShowMessage (this, TEXT ("Please select an object first."), TEXT ("DiskLED: Select an object"), MB_OK | MB_ICONEXCLAMATION);
		m_cSelectPerfObjects.SetFocus ();
		return;
	}
	if (m_sPerfCounter.IsEmpty ())
	{
		ShowMessage (this, TEXT ("Please select a counter first."), TEXT ("DiskLED: Select a counter"), MB_OK | MB_ICONEXCLAMATION);
		m_cSelectPerfCounters.SetFocus ();
		return;
	}
	if (m_sPerfInstance.IsEmpty () && m_cSelectPerfInstances.GetCount () > 0)
	{
		ShowMessage (this, TEXT ("Please select an instance first."), TEXT ("DiskLED: Select an instance"), MB_OK | MB_ICONEXCLAMATION);
		m_cSelectPerfInstances.SetFocus ();
		return;
	}

	// Use the path already built (and tested!)
	BOOL	bNewSource	= (m_sCounterPath != m_sSelectedPerfPath);
	m_sCounterPath	= m_sSelectedPerfPath;
	if (bNewSource)
	{
		ApplyCounterDefaults ();
	}

	// Write the new path to the edit box
	UpdateData (FALSE);
	RefreshScaleControls ();
	UpdatePreview ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	Get the explanatory text / description of a counter
//
////////////////////////////////////////////////////////////////////////////////

DWORD CDialogConfig::GetCounterExplainText (CString sCounterPath, CString& sExplainText)
{
	PDH_STATUS			nError			= ERROR_INVALID_FUNCTION;
	PDH_HQUERY			hQuery			= NULL;
	PDH_HCOUNTER		hCounter		= NULL;
	DWORD				nBufferInfo		= 0;

	// Zero earlier allocated buffers
	if (m_oBufferInfo)
	{
		SecureZeroMemory (m_oBufferInfo, m_nBufferInfo);
	}

	// Open a query handle
	nError	= PdhOpenQuery (NULL, 0, &hQuery);
	if(nError != ERROR_SUCCESS)
	{
		m_pMainFrame->LogError (TEXT ("GetCounterExplainText"), TEXT ("PdhOpenQuery"), nError);
		goto CleanUp;
	}

	// Add a counter to the query in order to get a handle to the counter
	nError	= PdhAddCounter (hQuery, sCounterPath, 0, &hCounter);
	if (nError != ERROR_SUCCESS)
	{
		CString	sExplanation;
		if (nError == PDH_CSTATUS_NO_OBJECT || nError == PDH_CSTATUS_NO_COUNTER)
		{
			sExplanation	= TEXT ("This probably means that the performance counter path is invalid: <") + sCounterPath + TEXT (">");
		}
		else if (nError == PDH_CSTATUS_BAD_COUNTERNAME)
		{
			sExplanation	= TEXT ("This probably means that the counter path built by DiskLED is invalid: <") + sCounterPath + TEXT (">");
		}
		m_pMainFrame->LogError (TEXT ("GetCounterExplainText"), TEXT ("PdhAddCounter"), nError, sExplanation);
		goto CleanUp;
	}

	// Get required buffer size
	nError	= PdhGetCounterInfo (hCounter, TRUE, &nBufferInfo, m_oBufferInfo);
	if (nError != PDH_MORE_DATA)
	{
		m_pMainFrame->LogError (TEXT ("GetCounterExplainText"), TEXT ("PdhGetCounterInfo (1)"), nError);
		goto CleanUp;
	}

	//
	// Allocate memory if necessary
	//
	if (nBufferInfo > m_nBufferInfo)
	{
		// Free memory of previously allocated
		if (m_oBufferInfo)
		{
			HeapFree (GetProcessHeap (), 0, m_oBufferInfo);
			m_oBufferInfo	= NULL;
		}

		// Allocate more than required
		m_nBufferInfo	= nBufferInfo * 2;
		nBufferInfo		= m_nBufferInfo;

		m_oBufferInfo	= (PDH_COUNTER_INFO*) HeapAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, m_nBufferInfo);
		if (m_oBufferInfo == NULL)
		{
			m_pMainFrame->LogError (TEXT ("GetCounterExplainText"), TEXT ("Memory allocation failed!"), 0);
			goto CleanUp;
		}
	}

	// Get information on the counter
	nError	= PdhGetCounterInfo (hCounter, TRUE, &nBufferInfo, m_oBufferInfo);
	if (nError != ERROR_SUCCESS)
	{
		m_pMainFrame->LogError (TEXT ("GetCounterExplainText"), TEXT ("PdhGetCounterInfo (2)"), nError);
		goto CleanUp;
	}

	// Store the out value
	sExplainText	= m_oBufferInfo->szExplainText;

	// For V2 providers (HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib\_V2Providers)
	// the explain text is not returned. With these, nError is ERROR_SUCCESS, but szExplainText is empty.
	if (sExplainText.IsEmpty ())
	{
		sExplainText	= TEXT ("<Could not retrieve the description.>");
	}

CleanUp:
	if (hQuery != NULL)
	{
		PdhCloseQuery (hQuery);
		hQuery	= NULL;
	}

	return nError;
}


////////////////////////////////////////////////////////////////////////////////
//
//	Gets called whenever a new counter is selected.
//	Updates the description
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnCbnSelchangePerfCounters()
{
	PDH_STATUS	nError			= ERROR_INVALID_FUNCTION;
	CString		sCounterPath;
	CString		sExplainText;
	CString		sInstance;

	// Get data from the dialog box into our member variables
	if (!UpdateData (TRUE))
	{
		return;
	}

	// By default, use the instance from the combo box
	sInstance	= m_sPerfInstance;

	// Is this combo box empty? If so, empty the description.
	if (m_sPerfCounter.IsEmpty ())
	{
		nError	= ERROR_INVALID_DATA;
		goto Finished;
	}

	if (sInstance.IsEmpty () && m_cSelectPerfInstances.GetCount () > 0)
	{
		// This counter has instances, but none is selected. Use any instance to build a valid counter path
		m_cSelectPerfInstances.GetLBText (0, sInstance);
	}

	//
	// Get the description of the counter
	//

	// Build a perf path
	nError	= BuildPerfPath (sCounterPath, m_sPerfObject, m_sPerfCounter, sInstance);
	if (nError != ERROR_SUCCESS)
	{
		goto Finished;
	}

	// Set the description
	nError	= GetCounterExplainText (sCounterPath, sExplainText);

Finished:
	if (nError == ERROR_SUCCESS)
	{
		m_sPerfDescription	= sExplainText;
		m_sSelectedPerfPath	= sCounterPath;
	}
	else
	{
		m_sPerfDescription	= TEXT ("");
		m_sSelectedPerfPath	= TEXT ("");
	}

	// Update the UI with data from the member variables
	UpdateData (FALSE);
}


////////////////////////////////////////////////////////////////////////////////
//
//	Gets called whenever a new instance is selected.
//	Updates the description
//
////////////////////////////////////////////////////////////////////////////////

void CDialogConfig::OnCbnSelchangePerfInstances ()
{
	PDH_STATUS				nError	= ERROR_INVALID_FUNCTION;
	CString	sCounterPath;
	CString sExplainText;

	// Get data into member variables
	if (!UpdateData (TRUE))
	{
		return;
	}

	// Is this combo box empty? If so, empty the description.
	if (m_sPerfInstance.IsEmpty () && m_cSelectPerfInstances.GetCount () > 0)
	{
		nError	= ERROR_INVALID_DATA;
		goto Finished;
	}

	if (m_sPerfCounter.IsEmpty ())
	{
		// If no counter is selected, we cannot build a valid counter path
		return;
	}

	//
	// Get the description of the counter
	//

	// Build a perf path
	nError	= BuildPerfPath (sCounterPath, m_sPerfObject, m_sPerfCounter, m_sPerfInstance);
	if (nError != ERROR_SUCCESS)
	{
		goto Finished;
	}

	// Set the description
	nError	= GetCounterExplainText (sCounterPath, sExplainText);

Finished:
	if (nError == ERROR_SUCCESS)
	{
		m_sPerfDescription	= sExplainText;
		m_sSelectedPerfPath	= sCounterPath;
	}
	else
	{
		m_sPerfDescription	= TEXT ("");
		m_sSelectedPerfPath	= TEXT ("");
	}

	// Update the UI with data from the member variables
	UpdateData (FALSE);
}
