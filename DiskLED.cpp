
// DiskLED.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DiskLED.h"
#include "MainFrm.h"
#include "DarkMode.h"
#include "DialogMessage.h"
#include "Helpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDiskLEDApp

BEGIN_MESSAGE_MAP(CDiskLEDApp, CWinAppEx)
	//ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDiskLEDApp construction

// Sent to the running DiskLED when it is started again, so it shows its settings
UINT g_nMsgShowSettings	= ::RegisterWindowMessage (TEXT ("DiskLED.ShowSettings"));

CDiskLEDApp::CDiskLEDApp()
{
	m_strWindowName		= TEXT ("DiskLED");		// Window/Application name
	m_hSingleInstance	= NULL;
}

CDiskLEDApp::~CDiskLEDApp()
{
	if (m_hSingleInstance)
	{
		CloseHandle (m_hSingleInstance);
	}
}


// The one and only CDiskLEDApp object

CDiskLEDApp theApp;


// There is one DiskLED per user, with all the meters. Returns FALSE if it is already running (it is asked to show its settings).
BOOL CDiskLEDApp::IsFirstInstance ()
{
	m_hSingleInstance	= CreateMutex (NULL, FALSE, TEXT ("Local\\DiskLED.SingleInstance"));
	if (m_hSingleInstance != NULL && GetLastError () != ERROR_ALREADY_EXISTS)
	{
		return TRUE;
	}
	if (m_hSingleInstance == NULL)
	{
		// Cannot tell, run anyway
		return TRUE;
	}

	// A copy of an older version in another folder may have left its own autostart entry: remove it, or the program would ask for the settings at every logon
	RemoveLegacyAutoStartEntry ();

	// The user started DiskLED, so Windows lets the running instance come to the foreground
	AllowSetForegroundWindow (ASFW_ANY);
	PostMessage (HWND_BROADCAST, g_nMsgShowSettings, 0, 0);

	return FALSE;
}


// CDiskLEDApp initialization

BOOL CDiskLEDApp::InitInstance()
{
	if (!IsFirstInstance ())
	{
		return FALSE;
	}
	MigrateAutoStart ();

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	// Menus follow the Windows app theme (dark or light)
	InitDarkMode ();

	CWinAppEx::InitInstance();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	SetRegistryKey(_T("DiskLED"));

	m_hIcon = LoadIcon(IDI_ICON1);
	if (m_hIcon == NULL)
	{
		ShowMessage (NULL, TEXT ("Could not load icon. Exiting!"), TEXT ("DiskLED"), MB_OK | MB_ICONERROR);
		return FALSE;
	}

	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

    pMainFrame->ModifyStyle(WS_VISIBLE, 0);

	return TRUE;
}
