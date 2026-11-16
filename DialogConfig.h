#pragma once
#include "afxwin.h"
#include "Settings.h"
#include "IconRenderer.h"
#include "DarkMode.h"


// Animation of the preview
#define PREVIEW_TIMER_ID		1
#define PREVIEW_INTERVAL_MS		120

// A change in the dialog is shown on the tray icons this long after the last change (so typing a number is not applied digit by digit)
#define CHANGE_TIMER_ID			2
#define CHANGE_DELAY_MS			250

// Posted to the dialog itself: open the wizard at the import
#define WM_IMPORT_WIZARD		(WM_APP + 20)


// CDialogConfig dialog

class CDialogConfig : public CDialog
{
	DECLARE_DYNAMIC(CDialogConfig)

public:
	CDialogConfig(CMainFrame* pMainFrame);   // standard constructor
	virtual ~CDialogConfig();

	// The first start of DiskLED found older copies: open the wizard at the import when the window opens. The default meter makes room for what is imported.
	void	StartWithImport ()		{ m_bFirstRunImport = TRUE; }

// Dialog Data
	enum { IDD = IDD_DIALOG_CONFIG };

protected:
	virtual	void	DoDataExchange(CDataExchange* pDX);		// DDX/DDV support
	BOOL			PreTranslateMessage(MSG* pMsg);			// Tooltip support
	virtual BOOL	OnCommand (WPARAM wParam, LPARAM lParam);
	virtual void	OnOK ();
	virtual void	OnCancel ();

	DECLARE_MESSAGE_MAP()

private:
	// Control object variables
	CListBox	m_cMeterList;			// The meters: the settings controls show the one selected
	CButton		m_cAddMeter;
	CButton		m_cRemoveMeter;
	CEdit		m_cMeterName;
	CButton		m_cSave;
	CButton		m_cRevert;
	CStatic		m_cStatus;
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
	CDarkTabCtrl	m_cTab;
	CComboBox	m_cQuickPick;
	CDarkTheme	m_oTheme;				// Follows the Windows app theme (dark or light)
	CStatic		m_cLinkSpeedText;
	CButton		m_cUseLinkSpeed;
	CEdit		m_cCounterPath;
	CButton		m_cDefaultValues;
	CComboBox	m_cSelectPerfObjects;
	CComboBox	m_cSelectPerfCounters;
	CComboBox	m_cSelectPerfInstances;
	CButton		m_cBuildPathFromSelection;
	CEdit		m_cSelectPerfDescription;

	// Control value variables
	int			m_nCounterSel;			// Which meter the settings controls show (index into m_oSettings.aMeters)
	CString		m_sMeterName;
	int			m_nTab;					// The page shown: 0 data source, 1 scale, 2 appearance, 3 alerts
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

	// The settings being edited. The controls show the meter selected in m_nCounterSel,
	// the settings of the other meters are kept here until they are selected.
	DiskLEDSettings	m_oSettings;

	// Changes are shown on the tray icons while editing (live preview) and only kept by Save
	DiskLEDSettings	m_oSaved;				// What was saved last (or what was running when the dialog opened)
	DiskLEDSettings	m_oApplied;				// What the tray icons show now
	int				m_nAutoStartSaved;
	BOOL			m_bLoading;				// The controls are being filled: their notifications are not changes
	BOOL			m_bDirty;
	BOOL			m_bFirstRunImport;		// See StartWithImport
	CStringArray	m_asPendingRunRemovals;	// Start with Windows entries of imported copies, removed when the settings are saved

	// Preview
	IconStyle		m_oPreviewStyle;
	IconCounter		m_oPreview;
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
	BOOL		ReadControls (BOOL bReport);
	void		ShowMeterInControls ();
	void		NotifyChanged ();
	BOOL		CommitChanges ();
	void		UpdateDirtyState ();
	BOOL		SaveNow ();
	void		LoadGlobalToControls ();
	void		SaveGlobalFromControls ();
	void		LoadMeterToControls (int nMeter);
	void		SaveControlsToMeter (int nMeter);
	void		FillMeterList ();
	CString		GetMeterListText (int nMeter);
	void		UpdateMeterButtons ();
	void		ShowWizard (BOOL bStartWithImport);
	void		UpdateSwatches ();
	void		UpdatePreview ();
	BOOL		Read100Percent (BOOL bReport);
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
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedRevert();
	afx_msg void OnBnClickedDefaultvalues();
	afx_msg void OnColorButton(UINT nID);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnLbnSelchangeMeterList();
	afx_msg LRESULT OnImportWizard(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedAddMeter();
	afx_msg void OnBnClickedRemoveMeter();
	afx_msg void OnTcnSelchangeTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCbnSelchangeValueFormat();
	afx_msg void OnCbnSelchangeScaleMode();
	afx_msg void OnBnClickedUseLinkSpeed();
	afx_msg void OnCbnSelchangeQuickPick();
	afx_msg void OnCbnSelchangePerfObjects();
	afx_msg void OnBnClickedUseSelectedDataSource();
	afx_msg void OnCbnSelchangePerfCounters();
	afx_msg void OnCbnSelchangePerfInstances();
};
