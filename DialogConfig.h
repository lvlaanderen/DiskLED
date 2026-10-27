#pragma once
#include "afxwin.h"
#include "Settings.h"
#include "IconRenderer.h"


// Animation of the preview
#define PREVIEW_TIMER_ID		1
#define PREVIEW_INTERVAL_MS		120


// CDialogConfig dialog

class CDialogConfig : public CDialog
{
	DECLARE_DYNAMIC(CDialogConfig)

public:
	CDialogConfig(CMainFrame* pMainFrame);   // standard constructor
	virtual ~CDialogConfig();

// Dialog Data
	enum { IDD = IDD_DIALOG_CONFIG };

protected:
	virtual	void	DoDataExchange(CDataExchange* pDX);		// DDX/DDV support
	BOOL			PreTranslateMessage(MSG* pMsg);			// Tooltip support

	DECLARE_MESSAGE_MAP()

private:
	// Control object variables
	CComboBox	m_cCounterSel;
	CButton		m_cSecondCounter;
	CComboBox	m_cIconType;
	CComboBox	m_cValueFormat;
	CComboBox	m_cScaleMode;
	CButton		m_cInvert;
	CEdit		m_cAlertPercent;
	CEdit		m_cAlertSeconds;
	CButton		m_cAutoStart;
	CButton		m_cColor[3];
	CEdit		m_cThresholdMid;
	CEdit		m_cThresholdHigh;
	CStatic		m_cPreview;
	CStatic		m_cUpdateFrequencyText1;
	CStatic		m_cUpdateFrequencyText2;
	CEdit		m_cUpdateFrequencyEdit;
	CEdit		m_c100Percent;
	CTabCtrl	m_cTab;
	CStatic		m_cLinkSpeedText;
	CButton		m_cUseLinkSpeed;
	CEdit		m_cCounterPath;
	CButton		m_cUpdateINI;
	CButton		m_cDefaultValues;
	CComboBox	m_cSelectPerfObjects;
	CComboBox	m_cSelectPerfCounters;
	CComboBox	m_cSelectPerfInstances;
	CButton		m_cBuildPathFromSelection;
	CEdit		m_cSelectPerfDescription;

	// Control value variables
	int			m_nCounterSel;			// Which counter the counter settings controls show: 0 or 1
	int			m_nTab;					// The page shown: 0 data source, 1 scale, 2 appearance, 3 alerts
	BOOL		m_bSecondCounterSeeded;	// The second counter already got the first counter's settings as a starting point
	int			m_nSecondCounter;
	int			m_nLEDType;				// Icon type - 1
	int			m_nValueFormat;			// Value format - 1
	int			m_nScaleMode;			// Scale mode - 1
	int			m_nInvert;
	int			m_nAlertPercent;
	int			m_nAlertSeconds;
	int			m_nAutoStart;
	COLORREF	m_anColors[3];
	int			m_nThresholdMid;
	int			m_nThresholdHigh;
	DWORD		m_nUpdateFrequency;
	int			m_nUpdateINI;
	double		m_d100Percent;			// The 100% value, in counter units
	CString		m_s100Percent;			// ... as shown in the edit box, with a unit (e.g. "350 Mbit/s")
	CString		m_s100PercentShown;		// The text last generated from m_d100Percent. If the edit box still has it, m_d100Percent is exact.
	double		m_dLinkSpeed;			// Speed of the network adapter of the selected counter in bytes/s, 0 = not a network counter
	CString		m_sCounterPath;			// In the system language
	CString		m_sPerfObject;
	CString		m_sPerfCounter;
	CString		m_sPerfInstance;
	CString		m_sPerfDescription;

	// Tooltip support
	CToolTipCtrl	m_oToolTip;

	// Member variables
	CMainFrame*		m_pMainFrame;
	CString			m_sSelectedPerfPath;

	// The settings being edited. The counter settings controls show the counter selected in m_nCounterSel,
	// the other counter's settings are kept here until it is selected.
	DiskLEDSettings	m_oSettings;

	// Preview
	IconStyle		m_oPreviewStyle;
	IconCounter		m_aPreview[2];
	int				m_nPreviewCounters;
	double			m_dPreviewTime;

	// Memory buffers for PdhEnumObjectItems
	TCHAR*		m_sBufferCounters;
	DWORD		m_nBufferCounters;
	TCHAR*		m_sBufferInstances;
	DWORD		m_nBufferInstances;

	// Memory buffer for PdhGetCounterInfo
	DWORD				m_nBufferInfo;
	PDH_COUNTER_INFO*	m_oBufferInfo;

	// Functions
	BOOL		OnInitDialog ();
	BOOL		ApplySettings ();
	void		LoadGlobalToControls ();
	void		SaveGlobalFromControls ();
	void		LoadCounterToControls (int nCounter);
	void		SaveControlsToCounter (int nCounter);
	void		UpdateSwatches ();
	void		UpdatePreview ();
	BOOL		Read100Percent ();
	void		UpdateScaleLabel ();
	void		UpdateLinkSpeedHint ();
	void		UpdateLinkSpeedVisibility ();
	void		ShowPage (int nPage);
	void		RefreshScaleControls ();
	void		ApplyCounterDefaults ();
	void		InitPerfBrowser ();
	void		SelectPathInBrowser ();
	DWORD		SplitPerfPath (CString sCounterPath, CString& sObject, CString& sCounter, CString& sInstance);
	DWORD		BuildPerfPath (CString& sCounterPath, CString sObject, CString sCounter, CString sInstance);
	DWORD		GetCounterExplainText (CString sCounterPath, CString& sExplainText);

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedDefaultvalues();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedApply();
	afx_msg void OnColorButton(UINT nID);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeCounterSel();
	afx_msg void OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCbnSelchangeValueFormat();
	afx_msg void OnCbnSelchangeScaleMode();
	afx_msg void OnBnClickedUseLinkSpeed();
	afx_msg void OnCbnSelchangePerfObjects();
	afx_msg void OnBnClickedUseSelectedDataSource();
	afx_msg void OnCbnSelchangePerfCounters();
	afx_msg void OnCbnSelchangePerfInstances();
};
