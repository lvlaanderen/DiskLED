#pragma once

#include "Settings.h"
#include "IconRenderer.h"
#include "DarkMode.h"
#include "resource.h"


// Animation of the preview
#define WIZARD_TIMER_ID			1
#define WIZARD_INTERVAL_MS		120


// A wizard that adds a meter in three steps: what to monitor, which one (adapter, disk, ...) and how it looks.
// Memory has only one of it, so it skips the second step. "Another counter" ends the wizard at once: the caller offers the manual settings.
class CDialogWizard : public CDialog
{
	DECLARE_DYNAMIC(CDialogWizard)

public:
	CDialogWizard (CWnd* pParent);

	enum { IDD = IDD_DIALOG_WIZARD };

	// The result when DoModal returns IDOK
	BOOL			m_bManual;			// "Another counter" was chosen: no meter was made, the user picks the counter in the settings
	MeterSettings	m_oMeter;			// The new meter (the number is not set)

protected:
	virtual BOOL	OnInitDialog ();
	virtual void	OnOK ();
	virtual void	OnCancel ();

	afx_msg HBRUSH	OnCtlColor (CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void	OnSettingChange (UINT uFlags, LPCTSTR lpszSection);
	afx_msg void	OnDrawItem (int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void	OnTimer (UINT_PTR nIDEvent);
	afx_msg void	OnDestroy ();
	afx_msg void	OnLbnSelchangeList ();
	afx_msg void	OnBack ();
	afx_msg void	OnNext ();
	afx_msg void	OnChangedLook ();

	DECLARE_MESSAGE_MAP()

private:
	CDarkTheme		m_oTheme;
	CFont			m_oTitleFont;
	CListBox		m_cList;
	CEdit			m_cName;
	CComboBox		m_cIcon;
	CStatic			m_cPreview;

	int				m_nStep;				// 1 to 3
	int				m_nWhat;				// What to monitor, see the list in ShowStep
	CStringArray	m_asInstances;			// The instances in the list of step 2
	CString			m_sInstance;			// The one that was chosen
	CString			m_sDefaultName;			// The name that was suggested, to see whether the user changed it
	BOOL			m_bLoading;

	IconStyle		m_oStyle;
	IconCounter		m_oPreview;
	double			m_dPreviewTime;

	void			ShowStep (int nStep);
	void			FillWhatList ();
	void			FillInstanceList ();
	void			UpdateDescription ();
	void			UpdateButtons ();
	CString			GetPath () const;
	BOOL			BuildMeter ();
	void			UpdatePreview ();
};
