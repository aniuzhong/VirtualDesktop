// Virtual Desktop.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Virtual Desktop.h"
#include "Virtual DesktopDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


extern void DebugPrintErrorMessage(TCHAR *pszErrorString = NULL, bool bDisplayMsg = false, TCHAR *pszMsgCaption = NULL);


// CVirtualDesktopApp

BEGIN_MESSAGE_MAP(CVirtualDesktopApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CVirtualDesktopApp construction

CVirtualDesktopApp::CVirtualDesktopApp()
: m_pApplicationWnd(NULL)
{
	// TODO: add construction code here, 
	// Place all significant initialization in InitInstance
}


// The one and only CVirtualDesktopApp object

CVirtualDesktopApp theApp;


// CVirtualDesktopApp initialization

BOOL CVirtualDesktopApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	bool AlreadyRunning = false;

	Sleep(1000);
	HANDLE hMutexOneInstance = ::CreateMutex( NULL, FALSE, _T("Virtual_Desktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}"));

	AlreadyRunning = ( ::GetLastError() == ERROR_ALREADY_EXISTS || 
		::GetLastError() == ERROR_ACCESS_DENIED);

	if(AlreadyRunning)
	{
		MessageBox(NULL, _T("One instance of this aplication is already running."), TXT_MESSAGEBOX_TITLE, 0);
		return FALSE;
	}


	m_pApplicationWnd = new CVirtualDesktopDlg();
	m_pApplicationWnd->Create(IDD_VIRTUALDESKTOP_DIALOG);
	m_pMainWnd = m_pApplicationWnd;
	ShowWindow(m_pMainWnd->GetSafeHwnd(), SW_HIDE);

	return TRUE;
	//CVirtualDesktopDlg dlg;
	//m_pMainWnd = &dlg;
	//INT_PTR nResponse = dlg.DoModal();
	//if (nResponse == IDOK)
	//{
	//	// TODO: Place code here to handle when the dialog is
	//	//  dismissed with OK
	//}
	//else if (nResponse == IDCANCEL)
	//{
	//	// TODO: Place code here to handle when the dialog is
	//	//  dismissed with Cancel
	//}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	//return FALSE;
}

int CVirtualDesktopApp::ExitInstance()
{
	// TODO: Add your specialized code here and/or call the base class
	if(m_pApplicationWnd)
	{
		//Malli::ToDo:Check for need of this call.
		m_pApplicationWnd->DestroyWindow();
		delete m_pApplicationWnd;
	}

	m_pApplicationWnd = NULL;
	return CWinApp::ExitInstance();
}
