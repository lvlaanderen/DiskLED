// DialogWizard.cpp : the wizard that adds a meter
//

#include "stdafx.h"
#include "DiskLED.h"
#include "DialogWizard.h"
#include "DialogMessage.h"
#include "Helpers.h"
#include <math.h>


// What can be monitored, in the order of the list of step 1
#define WHAT_DOWNLOAD		0
#define WHAT_UPLOAD			1
#define WHAT_DISK			2
#define WHAT_PROCESSOR		3
#define WHAT_MEMORY			4
#define WHAT_OTHER			5

struct WhatEntry
{
	const TCHAR*	sName;
	const TCHAR*	sDescription;
	const TCHAR*	sDefaultName;		// The suggested name of the meter
	DWORD			nIconType;			// The suggested icon
};

static const WhatEntry g_aWhat[] =
{
	{ TEXT ("Internet download speed"),	TEXT ("How fast data comes in from the internet, for example while you download a file or watch a video."),	TEXT ("Download"),	LEDTYPE_GRAPH },
	{ TEXT ("Internet upload speed"),	TEXT ("How fast data goes out to the internet, for example while you upload files or send a backup."),		TEXT ("Upload"),	LEDTYPE_GRAPH },
	{ TEXT ("Disk activity"),			TEXT ("How busy a disk is, like the activity light of a computer."),										TEXT ("Disk"),		LEDTYPE_FLICKER },
	{ TEXT ("Processor use"),			TEXT ("How hard the processor is working."),																TEXT ("Processor"),	LEDTYPE_COLOREDBAR },
	{ TEXT ("Memory in use"),			TEXT ("How much of the memory of the computer is in use."),													TEXT ("Memory"),	LEDTYPE_CLEANBAR },
	{ TEXT ("Another counter..."),		TEXT ("Choose any performance counter yourself, with the lists in the settings window."),					TEXT (""),			LEDTYPE_FLICKER }
};


IMPLEMENT_DYNAMIC(CDialogWizard, CDialog)

BEGIN_MESSAGE_MAP(CDialogWizard, CDialog)
	ON_COMMAND(IDOK, &CDialogWizard::OnNext)
	ON_COMMAND(IDCANCEL, &CDialogWizard::OnCancel)
	ON_COMMAND(IDC_WIZ_BACK, &CDialogWizard::OnBack)
	ON_COMMAND(IDC_WIZ_NEXT, &CDialogWizard::OnNext)
	ON_LBN_SELCHANGE(IDC_WIZ_LIST, &CDialogWizard::OnLbnSelchangeList)
	ON_CBN_SELCHANGE(IDC_WIZ_ICON, &CDialogWizard::OnChangedLook)
	ON_WM_CTLCOLOR()
	ON_WM_SETTINGCHANGE()
	ON_WM_DRAWITEM()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()


CDialogWizard::CDialogWizard (CWnd* pParent)
	: CDialog (CDialogWizard::IDD, pParent), m_bManual (FALSE), m_nStep (1), m_nWhat (WHAT_DOWNLOAD), m_bLoading (TRUE), m_dPreviewTime (0)
{
	SetDefaultMeterSettings (m_oMeter, 0);
	SecureZeroMemory (&m_oStyle, sizeof (m_oStyle));
	SecureZeroMemory (&m_oPreview, sizeof (m_oPreview));
}


////////////////////////////////////////////////////////////////////////////////
//
//	The dialog
//
////////////////////////////////////////////////////////////////////////////////

BOOL CDialogWizard::OnInitDialog ()
{
	CDialog::OnInitDialog ();

	m_cList.SubclassDlgItem (IDC_WIZ_LIST, this);
	m_cName.SubclassDlgItem (IDC_WIZ_NAME, this);
	m_cIcon.SubclassDlgItem (IDC_WIZ_ICON, this);
	m_cPreview.SubclassDlgItem (IDC_WIZ_PREVIEW, this);
	m_cName.SetLimitText (MAX_METER_NAME);

	// The title is larger than the text
	LOGFONT	oFont;

	GetFont ()->GetLogFont (&oFont);
	oFont.lfHeight	= oFont.lfHeight * 13 / 10;
	oFont.lfWeight	= FW_SEMIBOLD;
	if (m_oTitleFont.CreateFontIndirect (&oFont))
	{
		GetDlgItem (IDC_WIZ_TITLE)->SetFont (&m_oTitleFont);
	}

	// Icon type entries, in the order of the LEDTYPE_* constants
	m_cIcon.AddString (TEXT ("LED"));
	m_cIcon.AddString (TEXT ("Bar"));
	m_cIcon.AddString (TEXT ("Clean bar"));
	m_cIcon.AddString (TEXT ("VU meter"));
	m_cIcon.AddString (TEXT ("Graph"));
	m_cIcon.AddString (TEXT ("Number"));
	m_cIcon.SetCurSel (0);

	// Dark theme, if Windows apps use it
	m_oTheme.Update (m_hWnd);

	CenterOnPointerMonitor (this);

	ShowStep (1);
	m_bLoading	= FALSE;

	SetTimer (WIZARD_TIMER_ID, WIZARD_INTERVAL_MS, NULL);

	// The focus is set by ShowStep
	return FALSE;
}

void CDialogWizard::OnDestroy ()
{
	KillTimer (WIZARD_TIMER_ID);

	CDialog::OnDestroy ();
}

HBRUSH CDialogWizard::OnCtlColor (CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	BOOL	bField	= (nCtlColor == CTLCOLOR_EDIT || nCtlColor == CTLCOLOR_LISTBOX);
	HBRUSH	hBrush	= m_oTheme.CtlColor (pDC, bField);

	return hBrush ? hBrush : CDialog::OnCtlColor (pDC, pWnd, nCtlColor);
}

void CDialogWizard::OnSettingChange (UINT uFlags, LPCTSTR lpszSection)
{
	CDialog::OnSettingChange (uFlags, lpszSection);

	if (m_hWnd && lpszSection && _tcscmp (lpszSection, TEXT ("ImmersiveColorSet")) == 0 && m_oTheme.Update (m_hWnd))
	{
		m_cPreview.Invalidate (FALSE);
	}
}

void CDialogWizard::OnCancel ()
{
	EndDialog (IDCANCEL);
}

void CDialogWizard::OnOK ()
{
	OnNext ();
}


////////////////////////////////////////////////////////////////////////////////
//
//	The steps. Step 1 and 2 show a list, step 3 shows the name and the icon.
//
////////////////////////////////////////////////////////////////////////////////

void CDialogWizard::ShowStep (int nStep)
{
	static const UINT aListControls[]	= { IDC_WIZ_LIST, IDC_WIZ_DESC };
	static const UINT aLookControls[]	= { IDC_WIZ_NAME_LABEL, IDC_WIZ_NAME, IDC_WIZ_ICON_LABEL, IDC_WIZ_ICON, IDC_WIZ_PREVIEW };

	m_nStep	= nStep;

	for (int i = 0; i < _countof (aListControls); i++)
	{
		GetDlgItem (aListControls[i])->ShowWindow (nStep < 3 ? SW_SHOW : SW_HIDE);
	}
	for (int i = 0; i < _countof (aLookControls); i++)
	{
		GetDlgItem (aLookControls[i])->ShowWindow (nStep == 3 ? SW_SHOW : SW_HIDE);
	}

	// Memory is only one thing, so it has no second step
	CString	sStep;
	int		nTotal		= (m_nWhat == WHAT_MEMORY) ? 2 : 3;
	int		nShown		= (m_nWhat == WHAT_MEMORY && nStep == 3) ? 2 : nStep;

	sStep.Format (TEXT ("Step %d of %d"), nShown, nTotal);
	SetDlgItemText (IDC_WIZ_STEP, sStep);

	if (nStep == 1)
	{
		SetDlgItemText (IDC_WIZ_TITLE, TEXT ("What do you want to monitor?"));
		SetDlgItemText (IDC_WIZ_HINT, TEXT ("Every meter is one icon in the tray. You can add more meters later."));
		FillWhatList ();
		m_cList.SetFocus ();
	}
	else if (nStep == 2)
	{
		CString	sTitle;
		CString	sHint;

		switch (m_nWhat)
		{
		case WHAT_DOWNLOAD:
		case WHAT_UPLOAD:
			sTitle	= TEXT ("Which network adapter?");
			sHint	= TEXT ("The adapter that carries your internet traffic is recommended. A computer can have several, for example a cable and a Wi-Fi adapter.");
			break;
		case WHAT_DISK:
			sTitle	= TEXT ("Which disk?");
			sHint	= TEXT ("\"All disks\" shows the activity of all of them together.");
			break;
		default:
			sTitle	= TEXT ("Which processor?");
			sHint	= TEXT ("\"All processors\" shows the use of the whole processor. A single processor is one core of it.");
		}
		SetDlgItemText (IDC_WIZ_TITLE, sTitle);
		SetDlgItemText (IDC_WIZ_HINT, sHint);
		SetDlgItemText (IDC_WIZ_DESC, TEXT (""));
		FillInstanceList ();
		m_cList.SetFocus ();
	}
	else
	{
		SetDlgItemText (IDC_WIZ_TITLE, TEXT ("How should it look?"));
		SetDlgItemText (IDC_WIZ_HINT, TEXT ("The name is shown when you point at the icon. The look can be changed later, together with colours and warnings."));

		// A name and an icon are suggested, unless the user chose their own
		CString	sName;

		m_cName.GetWindowText (sName);
		if (sName.IsEmpty () || sName == m_sDefaultName)
		{
			m_sDefaultName	= g_aWhat[m_nWhat].sDefaultName;
			m_cName.SetWindowText (m_sDefaultName);
			m_cIcon.SetCurSel (g_aWhat[m_nWhat].nIconType - 1);
		}
		m_cName.SetFocus ();
		m_cName.SetSel (0, -1);
		UpdatePreview ();
	}

	UpdateButtons ();
}

void CDialogWizard::FillWhatList ()
{
	m_cList.ResetContent ();
	for (int i = 0; i < _countof (g_aWhat); i++)
	{
		m_cList.AddString (g_aWhat[i].sName);
	}
	m_cList.SetCurSel (m_nWhat);
	UpdateDescription ();
}

// The adapters, disks or processors. The one that is probably meant comes first.
void CDialogWizard::FillInstanceList ()
{
	CStringArray	asFound;
	CString			sObject;
	CString			sFirst;
	CString			sFirstText;

	m_cList.ResetContent ();
	m_asInstances.RemoveAll ();

	switch (m_nWhat)
	{
	case WHAT_DOWNLOAD:
	case WHAT_UPLOAD:
		sObject		= TEXT ("Network Interface");
		sFirst		= GetDefaultNetworkInstance ();
		sFirstText	= sFirst + TEXT ("  (recommended)");
		break;
	case WHAT_DISK:
		sObject		= TEXT ("PhysicalDisk");
		sFirst		= TEXT ("_Total");
		sFirstText	= TEXT ("All disks");
		break;
	default:
		sObject		= TEXT ("Processor");
		sFirst		= TEXT ("_Total");
		sFirstText	= TEXT ("All processors");
	}

	GetPerfInstances (sObject, asFound);

	if (!sFirst.IsEmpty ())
	{
		m_cList.AddString (sFirstText);
		m_asInstances.Add (sFirst);
	}

	for (int i = 0; i < asFound.GetSize (); i++)
	{
		CString	sText	= asFound[i];

		if (sText.CompareNoCase (sFirst) == 0)
		{
			continue;
		}
		if (m_nWhat == WHAT_PROCESSOR)
		{
			sText	= TEXT ("Processor ") + sText;
		}
		m_cList.AddString (sText);
		m_asInstances.Add (asFound[i]);
	}

	m_cList.SetCurSel (0);
}

void CDialogWizard::UpdateDescription ()
{
	int	nSel	= m_cList.GetCurSel ();

	SetDlgItemText (IDC_WIZ_DESC, (m_nStep == 1 && nSel >= 0) ? g_aWhat[nSel].sDescription : TEXT (""));
}

// The last step, and "another counter", add the meter instead of going on
void CDialogWizard::UpdateButtons ()
{
	BOOL	bFinish	= (m_nStep == 3) || (m_nStep == 1 && m_cList.GetCurSel () == WHAT_OTHER);

	GetDlgItem (IDC_WIZ_BACK)->EnableWindow (m_nStep > 1);
	SetDlgItemText (IDC_WIZ_NEXT, bFinish ? TEXT ("&Add meter") : TEXT ("&Next >"));
}

void CDialogWizard::OnLbnSelchangeList ()
{
	UpdateDescription ();
	UpdateButtons ();
}

void CDialogWizard::OnBack ()
{
	if (m_nStep == 3)
	{
		ShowStep (m_nWhat == WHAT_MEMORY ? 1 : 2);
	}
	else if (m_nStep == 2)
	{
		ShowStep (1);
	}
}

void CDialogWizard::OnNext ()
{
	int	nSel	= m_cList.GetCurSel ();

	if (m_nStep == 1)
	{
		if (nSel < 0)
		{
			return;
		}
		m_nWhat	= nSel;

		if (m_nWhat == WHAT_OTHER)
		{
			m_bManual	= TRUE;
			EndDialog (IDOK);
			return;
		}
		if (m_nWhat == WHAT_MEMORY)
		{
			m_sInstance.Empty ();
			ShowStep (3);
			return;
		}

		ShowStep (2);
		if (m_asInstances.GetSize () == 0)
		{
			ShowMessage (this, TEXT ("DiskLED could not find anything of this kind on this computer. Please choose another counter."), TEXT ("DiskLED: Add a meter"), MB_OK | MB_ICONEXCLAMATION);
			ShowStep (1);
		}
	}
	else if (m_nStep == 2)
	{
		if (nSel < 0 || nSel >= m_asInstances.GetSize ())
		{
			return;
		}
		m_sInstance	= m_asInstances[nSel];
		ShowStep (3);
	}
	else if (BuildMeter ())
	{
		EndDialog (IDOK);
	}
}


////////////////////////////////////////////////////////////////////////////////
//
//	The counter that was chosen, as an English path
//
////////////////////////////////////////////////////////////////////////////////

CString CDialogWizard::GetPath () const
{
	switch (m_nWhat)
	{
	case WHAT_DOWNLOAD:
		return MakePerfPath (TEXT (""), TEXT ("Network Interface"), m_sInstance, TEXT ("Bytes Received/sec"));
	case WHAT_UPLOAD:
		return MakePerfPath (TEXT (""), TEXT ("Network Interface"), m_sInstance, TEXT ("Bytes Sent/sec"));
	case WHAT_DISK:
		return MakePerfPath (TEXT (""), TEXT ("PhysicalDisk"), m_sInstance, TEXT ("% Disk Time"));
	case WHAT_PROCESSOR:
		return MakePerfPath (TEXT (""), TEXT ("Processor"), m_sInstance, TEXT ("% Processor Time"));
	}

	return MakePerfPath (TEXT (""), TEXT ("Memory"), TEXT (""), TEXT ("% Committed Bytes In Use"));
}

BOOL CDialogWizard::BuildMeter ()
{
	CString	sPath	= GetPath ();
	CString	sName;

	if (!IsValidEnglishCounterPath (sPath))
	{
		ShowMessage (this, TEXT ("DiskLED could not find this data source on this computer. Please go back and choose another one."), TEXT ("DiskLED: Add a meter"), MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	m_cName.GetWindowText (sName);

	// The value format, scale and 100% value that suit the counter
	SetDefaultMeterSettings (m_oMeter, 0);
	m_oMeter.oCounter.sPath	= sPath;
	GetCounterDefaults (sPath, m_oMeter.oCounter);
	m_oMeter.sName			= sName;
	m_oMeter.nIconType		= m_cIcon.GetCurSel () + 1;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////
//
//	The preview: the icon as it will look, animated with a test signal
//
////////////////////////////////////////////////////////////////////////////////

void CDialogWizard::OnChangedLook ()
{
	UpdatePreview ();
}

static double WizardTriangleWave (double dTime, double dPeriod)
{
	double	dPhase	= fmod (dTime, dPeriod) / dPeriod;

	return (dPhase < 0.5) ? dPhase * 2.0 : (1.0 - dPhase) * 2.0;
}

void CDialogWizard::UpdatePreview ()
{
	MeterSettings	oMeter;
	BOOL			bSpeed	= (m_nWhat == WHAT_DOWNLOAD || m_nWhat == WHAT_UPLOAD);
	double			dMax	= bSpeed ? 12500000 : 100;
	double			dValue;

	SetDefaultMeterSettings (oMeter, 1);
	oMeter.nIconType	= m_cIcon.GetCurSel () + 1;
	MakeIconStyle (oMeter, m_oStyle);

	m_dPreviewTime	+= WIZARD_INTERVAL_MS / 1000.0;
	dValue			= dMax * WizardTriangleWave (m_dPreviewTime, 10.0);

	memmove (m_oPreview.adHistory, m_oPreview.adHistory + 1, (HISTORY_SIZE - 1) * sizeof (double));
	m_oPreview.adHistory[HISTORY_SIZE - 1]	= dValue;
	m_oPreview.dValue						= dValue;
	m_oPreview.dScale						= dMax;
	m_oPreview.nValueFormat					= bSpeed ? VALUEFORMAT_BITS : VALUEFORMAT_PERCENT;

	if (m_cPreview.m_hWnd)
	{
		m_cPreview.Invalidate (FALSE);
	}
}

void CDialogWizard::OnTimer (UINT_PTR nIDEvent)
{
	if (nIDEvent == WIZARD_TIMER_ID)
	{
		if (m_nStep == 3)
		{
			UpdatePreview ();
		}
		return;
	}

	CDialog::OnTimer (nIDEvent);
}

void CDialogWizard::OnDrawItem (int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (nIDCtl != IDC_WIZ_PREVIEW)
	{
		CDialog::OnDrawItem (nIDCtl, lpDrawItemStruct);
		return;
	}

	CDC*	pDC		= CDC::FromHandle (lpDrawItemStruct->hDC);
	CRect	oRect	(lpDrawItemStruct->rcItem);
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
		DrawIconImage (&oMemDC, nSize, m_oStyle, &m_oPreview, 1);
		pDC->StretchBlt (oRect.left + (oRect.Width () - nSize * nZoom) / 2, oRect.top + (oRect.Height () - nSize * nZoom) / 2,
			nSize * nZoom, nSize * nZoom, &oMemDC, 0, 0, nSize, nSize, SRCCOPY);

		oMemDC.SelectObject (pOldBitmap);
	}
}
