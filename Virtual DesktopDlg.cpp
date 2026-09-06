// Virtual DesktopDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Virtual Desktop.h"
#include "Virtual DesktopDlg.h"
#include "DesktopManager.h"
#include "RegSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const UINT WM_TRAYICON_NOTIFY_MESSAGE = RegisterWindowMessage(_T("WM_TRAYICON_NOTIFY_MESSAGE-{8DDBE93E-DFE8-4279-934E-05C39902F37D}"));
extern void DebugPrintErrorMessage(TCHAR *pszErrorString = NULL, bool bDisplayMsg = false, TCHAR *pszMsgCaption = NULL);

// CAboutDlg dialog used for App About

#define CONTEXT_MENU_IDS			600
#define MANAGE_DESKTOP_MENU_ID		500
#define VERIFY_SWITCH_MENU_ID		501
#define LAUNCH_APP_MENU_ID			502
#define EXIT_MENU_ID				503
#define SEPARATOR_MENU_ID			504

typedef bool(*InstallHook)(void);

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CVirtualDesktopDlg dialog

CVirtualDesktopDlg::CVirtualDesktopDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CVirtualDesktopDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVirtualDesktopDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DESKTOP_LIST, m_DesktopListControl);
	DDX_Control(pDX, IDC_DESKTOP_NAME, m_DesktopNameControl);
	DDX_Control(pDX, IDC_ADD_NEW_DESKTOP, m_AddNewDesktop);
	DDX_Control(pDX, IDC_SWITCH_TO_DESKTOP, m_SwitchToDesktop);
	DDX_Control(pDX, IDC_VERIFY_CHECK, m_ChkVerifyDesktopSwitch);
}

BEGIN_MESSAGE_MAP(CVirtualDesktopDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_REGISTERED_MESSAGE(WM_TRAYICON_NOTIFY_MESSAGE, OnTrayMessage)
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
	ON_LBN_SELCHANGE(IDC_DESKTOP_LIST, &CVirtualDesktopDlg::OnLbnSelchangeDesktopList)
	ON_BN_CLICKED(IDC_ADD_NEW_DESKTOP, &CVirtualDesktopDlg::OnBnClickedAddNewDesktop)
	ON_BN_CLICKED(IDC_SWITCH_TO_DESKTOP, &CVirtualDesktopDlg::OnBnClickedSwitchToDesktop)
	ON_BN_CLICKED(IDC_LAUNCH_APPLICATION, &CVirtualDesktopDlg::OnBnClickedLaunchApplication)
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
	ON_BN_CLICKED(IDC_VERIFY_CHECK, &CVirtualDesktopDlg::OnBnClickedVerifyCheck)
END_MESSAGE_MAP()


// CVirtualDesktopDlg message handlers

BOOL CVirtualDesktopDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	try
	{
		TCHAR szCurrentDesktopName[ARRAY_SIZE]  = {0};
		//Adding try notify icon for the application.
		NOTIFYICONDATA nData;
		nData.cbSize = sizeof(NOTIFYICONDATA);
		nData.hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));
		nData.hWnd = m_hWnd;

		_tcscpy_s(nData.szTip, 127, TXT_MESSAGEBOX_TITLE);

		if(CDesktopManager::GetCurrentDesktopName(szCurrentDesktopName))
		{
			_tcscat_s(nData.szTip, 127, _T(" ["));
			_tcscat_s(nData.szTip, 127, szCurrentDesktopName);
			_tcscat_s(nData.szTip, 127, _T(" Desktop]"));
		}

		nData.uCallbackMessage = WM_TRAYICON_NOTIFY_MESSAGE;
		nData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE ;
		nData.uID = 1;

		if(!Shell_NotifyIcon(NIM_ADD, &nData))
		{
			MessageBox(_T("Failed to set tray icon."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
			throw false;
		}

		if(!RegisterApplicationHotKeys())
		{
			MessageBox(_T("Failed to register Hot Keys."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
			throw false;
		}

		//Load the Event hooker dll
		HMODULE hModule = LoadLibrary(_T("Event Hooker Dll.dll"));
		InstallHook fpInstallHook = NULL;

		if(NULL != hModule)
		{
			//Get the windows procedure hook installer function address.
			fpInstallHook = (InstallHook) GetProcAddress(hModule, ("InstallWinProcHook"));

			if(NULL != fpInstallHook)
				if(!fpInstallHook()) //Install the windows procedure hook.
				{
					MessageBox(_T("Failed to install hooks."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
					throw false;
				}

			//Get the message hook installer function address.
			fpInstallHook = (InstallHook) GetProcAddress(hModule, ("InstallMessageHook"));

			if(NULL != fpInstallHook)
				if(!fpInstallHook()) //Install the message hook.
				{
					MessageBox(_T("Failed to install hooks."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
					throw false;
				}
		}
		else
		{
			MessageBox(_T("Failed to library."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
			throw false;
		}

		CRegSettings objRegSettingsReader;
		m_ChkVerifyDesktopSwitch.SetCheck(objRegSettingsReader.ReadProfileInt(REG_KEY_COMMON_SETTINGS, REG_SUB_KEY_CONFIRM_SWITCH, 1));

	}
	catch(bool)
	{
		DebugPrintErrorMessage(_T("\nCVirtualDesktopDlg::OnInitDialog:\tCustom Excepton Caught."));
		PostQuitMessage(-1);
	}
	catch(...)
	{
		DebugPrintErrorMessage(_T("\nCVirtualDesktopDlg::OnInitDialog:\tExcepton Caught."));
		PostQuitMessage(-1);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CVirtualDesktopDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model, 
//  this is automatically done for you by the framework.

void CVirtualDesktopDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CVirtualDesktopDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CVirtualDesktopDlg::OnDestroy()
{
	HMODULE hModule = NULL;

	try
	{
		CDialog::OnDestroy();

		OutputDebugString(_T("\nCVirtualDesktopDlg::OnDestroy:\tIn OnDestroy()"));
		// TODO: Add your message handler code here
		NOTIFYICONDATA nData;
		nData.cbSize = sizeof(NOTIFYICONDATA);
		nData.hWnd = m_hWnd;
		nData.uID = 1;

		if(!Shell_NotifyIcon(NIM_DELETE, &nData))
			DebugPrintErrorMessage();

		//Load the event hooker dll.
		hModule = LoadLibrary(_T("Event Hooker Dll.Dll"));

		if(NULL != hModule)
		{
			InstallHook fpUninstallHook = NULL;
			//Get the hook uninstaller function address.
			fpUninstallHook = (InstallHook) GetProcAddress(hModule, ("UnInstallMsgHook"));
			if(NULL == fpUninstallHook || !fpUninstallHook()) //Uninstall the hooks.
				throw _T("\nCVirtualDesktopDlg::OnDestroy:\tFailed to uninstall Msg Hook in OnDestroy()");

			fpUninstallHook = NULL;
			//Get the hook uninstaller function address.
			fpUninstallHook = (InstallHook) GetProcAddress(hModule, ("UnInstallWinProcHook"));
			if(NULL == fpUninstallHook || !fpUninstallHook()) //Uninstall the hooks.
				throw _T("\nCVirtualDesktopDlg::OnDestroy:\tFailed to uninstall WinProc Hook in OnDestroy()");
		}
	}
	catch(TCHAR *pszErrorString)
	{
		OutputDebugString(_T("\nCVirtualDesktopDlg::OnDestroy:\t Custom Exception Caught."));
		DebugPrintErrorMessage(pszErrorString);
	}
	catch(...)
	{
		OutputDebugString(_T("\nCVirtualDesktopDlg::OnDestroy:\t Exception Caught."));
		DebugPrintErrorMessage();
	}

	FreeLibrary(hModule);

	//Release the memory to avoid the memory leaks.
	CDesktopManager::ReleaseMemory();
}

LRESULT CVirtualDesktopDlg::OnTrayMessage(WPARAM wParam, LPARAM lParam)
{
	UINT uMsg = (UINT) lParam;

	if(uMsg == WM_RBUTTONDOWN || uMsg == WM_CONTEXTMENU)
	{
		POINT pt;
		GetCursorPos(&pt);

		//Display a tray menu with all the desktop names as menu items.
		int iDesktopCounts = CDesktopManager::GetDesktopCount();
		int iMenuItemCount = 0;
		int iHotKeyCounter = iDesktopCounts;

		HMENU hContextMenu = CreatePopupMenu();

		//Iterate to add all the desktop names as menu items into the tray menu.
		for(iMenuItemCount = 0;iMenuItemCount < iDesktopCounts ; iMenuItemCount++)
		{
			TCHAR szDesktopName[ARRAY_SIZE] = {0};
			TCHAR szMenuItemName[ARRAY_SIZE] = {0};
			CDesktopManager::GetDesktopName(iMenuItemCount, szDesktopName);

			_tcscpy_s(szMenuItemName, ARRAY_SIZE, szDesktopName);

			wsprintf(szMenuItemName, _T("%s \tCtrl + Shift + %d"), szDesktopName, iHotKeyCounter--);
			
			if(CDesktopManager::IsCurrentDesktop(szDesktopName))
				AppendMenu(hContextMenu, MF_STRING | MF_ENABLED | MF_CHECKED, CONTEXT_MENU_IDS + iMenuItemCount , szMenuItemName);
			else
				AppendMenu(hContextMenu, MF_STRING | MF_ENABLED, CONTEXT_MENU_IDS + iMenuItemCount , szMenuItemName);
		}


		if(iMenuItemCount > 0)
			AppendMenu(hContextMenu, MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, TXT_VIRTUAL_DESKTOP_SEPARATOR_MENU_ITEM);

		if(m_ChkVerifyDesktopSwitch.GetCheck())
			AppendMenu(hContextMenu, MF_ENABLED | MF_STRING | MF_CHECKED, VERIFY_SWITCH_MENU_ID, TXT_CONFIRM_MENU_ITEM);
		else
			AppendMenu(hContextMenu, MF_ENABLED | MF_STRING, VERIFY_SWITCH_MENU_ID, TXT_CONFIRM_MENU_ITEM);

		AppendMenu(hContextMenu, MF_ENABLED | MF_STRING, MANAGE_DESKTOP_MENU_ID, TXT_MANAGE_DESKTOP_MENU_ITEM);
		AppendMenu(hContextMenu, MF_ENABLED | MF_STRING, LAUNCH_APP_MENU_ID, TXT_LAUNCH_APPLICATION_MENU_ITEM);
		AppendMenu(hContextMenu, MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, TXT_VIRTUAL_DESKTOP_SEPARATOR_MENU_ITEM);
		AppendMenu(hContextMenu, MF_ENABLED | MF_STRING, IDM_ABOUTBOX, TXT_ABOUT_MENU_ITEM);
		AppendMenu(hContextMenu, MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, TXT_VIRTUAL_DESKTOP_SEPARATOR_MENU_ITEM);
		AppendMenu(hContextMenu, MF_ENABLED | MF_STRING, EXIT_MENU_ID, TXT_EXIT_MENU_ITEM);
		
		SetForegroundWindow();
		//Display the context menu.
		int iSelectedIndex = TrackPopupMenu(hContextMenu, TPM_TOPALIGN | TPM_VERPOSANIMATION | TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);

		switch(iSelectedIndex)
		{
			case IDM_ABOUTBOX:
				PostMessage(WM_SYSCOMMAND, IDM_ABOUTBOX, NULL);
				break;
			case EXIT_MENU_ID:
				//Exit the application.
				UnRegisterApplicationHotKeys();
				PostQuitMessage(0);
				break;
			case MANAGE_DESKTOP_MENU_ID:
				//Show the Manage Desktop dialog.
				ShowManageDesktopsDialog();
				break;
			case VERIFY_SWITCH_MENU_ID:
				{
					//Toggle the check.
					m_ChkVerifyDesktopSwitch.SetCheck(!m_ChkVerifyDesktopSwitch.GetCheck());

					CRegSettings objRegSettingsReader;
					objRegSettingsReader.SetProfileInt(REG_KEY_COMMON_SETTINGS, REG_SUB_KEY_CONFIRM_SWITCH, m_ChkVerifyDesktopSwitch.GetCheck());

					break;
				}
			case LAUNCH_APP_MENU_ID:
				OnBnClickedLaunchApplication();
				break;
			default:
				if(iSelectedIndex >= CONTEXT_MENU_IDS)
				{
					TCHAR szSwitchToDesktopName[ARRAY_SIZE] = {0};
					//Get the desktop name to be switched to.
					CDesktopManager::GetDesktopName(iSelectedIndex - CONTEXT_MENU_IDS, szSwitchToDesktopName);
					//Switch to the selected desktop.
					if(_tcslen(szSwitchToDesktopName))
						SwitchDesktopTo(szSwitchToDesktopName);
				}
		}
	}

	return LRESULT();
}

//Display the Manage Desktops dialog.
void CVirtualDesktopDlg::ShowManageDesktopsDialog(void)
{
	try
	{
		//CDesktopManager objDeskManager;
		int iDeskCount = CDesktopManager::GetDesktopCount();
		//Clear the list box items.
		m_DesktopListControl.ResetContent();

		//Insert all the desktop names into the list box.
		for(int i = 0;i < iDeskCount;i++)
		{
			TCHAR szTempDeskName[ARRAY_SIZE]  = {0};
			CDesktopManager::GetDesktopName(i, szTempDeskName);
			m_DesktopListControl.AddString(szTempDeskName);
		}

		TCHAR szSelectedDesktopName[ARRAY_SIZE] = {0};

		m_DesktopNameControl.GetWindowText(szSelectedDesktopName, ARRAY_SIZE - 1);

		if(0 != _tcslen(szSelectedDesktopName) && -1 != m_DesktopListControl.SelectString(0, szSelectedDesktopName))
			m_DesktopNameControl.EnableWindow(FALSE);
		else
		{
			m_DesktopNameControl.SetWindowText(NULL);
			m_DesktopNameControl.EnableWindow(TRUE);
		}

		ShowWindow(SW_SHOW);
	}
	catch(...)
	{
		DebugPrintErrorMessage();
		OutputDebugString(_T("\nCVirtualDesktopDlg::ShowManageDesktopsDialog:\tException caught in CVirtualDesktopDlg::ShowManageDesktopsDialog."));
	}
}

void CVirtualDesktopDlg::OnLbnSelchangeDesktopList()
{
	// TODO: Add your control notification handler code here
	TCHAR szSelectedDesktopName[ARRAY_SIZE] = {0};
	m_DesktopListControl.GetText(m_DesktopListControl.GetCurSel(), szSelectedDesktopName);
	m_DesktopNameControl.SetWindowText(szSelectedDesktopName);

	m_AddNewDesktop.SetWindowText(_T("&New"));
	m_SwitchToDesktop.EnableWindow(TRUE);
	m_DesktopNameControl.EnableWindow(FALSE);
}

//Creates the new desktop (Adds new desktop)
void CVirtualDesktopDlg::OnBnClickedAddNewDesktop()
{
	// TODO: Add your control notification handler code here
	TCHAR szCaption[ARRAY_SIZE] = {0};
	m_AddNewDesktop.GetWindowText(szCaption, ARRAY_SIZE - 1);
	if(_tcsicmp(szCaption, _T("&New")))
	{
		m_DesktopNameControl.GetWindowText(szCaption, ARRAY_SIZE -1);

		//Left trimming the string.
		while(' ' == szCaption[0])
			_tcscpy_s(szCaption, ARRAY_SIZE - 1, szCaption + 1);
		
		int iLen = (int) _tcslen(szCaption);
		//Right trimming the string.
		while(' ' == szCaption[--iLen])
			szCaption[iLen] = '\0';

		if(_tcslen(szCaption))
		{
			//Check the desktop name in the list.
			if(-1 != m_DesktopListControl.SelectString(0, szCaption))
			{
				MessageBox(_T("Desktop already created !"), TXT_MESSAGEBOX_TITLE, MB_ICONEXCLAMATION | MB_TOPMOST | MB_TASKMODAL);
			}

			//Create the desktop
			if(CDesktopManager::CreateDesktop(szCaption))
			{
				if(IDYES == MessageBox(_T("New Desktop is been created.\nWould you like to switch to new desktop ?"), TXT_MESSAGEBOX_TITLE, MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL))
					SwitchDesktopTo(szCaption);

				m_DesktopListControl.AddString(szCaption);
				m_DesktopListControl.SelectString(0, szCaption);

				OnLbnSelchangeDesktopList();

				UpdateHotKeys();
			}
		}
		else
		{
			MessageBox(_T("Please enter Desktop Name"), TXT_MESSAGEBOX_TITLE, MB_ICONEXCLAMATION | MB_TOPMOST | MB_TASKMODAL);
			m_DesktopNameControl.SetWindowText(_T("")); 
			m_DesktopNameControl.SetFocus();
		}
	}
	else
	{
		m_AddNewDesktop.SetWindowText(_T("&Add"));
		m_DesktopNameControl.SetWindowText(_T(""));
		m_DesktopNameControl.EnableWindow(TRUE);
		m_SwitchToDesktop.EnableWindow(FALSE);
		m_DesktopNameControl.SetFocus();
	}
}

//Switch desktop
void CVirtualDesktopDlg::OnBnClickedSwitchToDesktop()
{
	// TODO: Add your control notification handler code here

	TCHAR szSwitchToDesktopName[ARRAY_SIZE] = {0};

	m_DesktopListControl.GetText(m_DesktopListControl.GetCurSel(), szSwitchToDesktopName);
	//Switching the desktop to specified one.
	SwitchDesktopTo(szSwitchToDesktopName);
}

void CVirtualDesktopDlg::SwitchDesktopTo(TCHAR * szDesktopName)
{
	try
	{
		SetForegroundWindow();
		//CDesktopManager objDeskManager; 
		if(NULL == szDesktopName)
			return;

		//Checking whether we're in the same desktop.
		if(CDesktopManager::IsCurrentDesktop(szDesktopName))
		{
			//If we're in the same specified desktop, just return.
			MessageBox(_T("You are currently on the same Desktop."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION  | MB_TOPMOST | MB_TASKMODAL);
			return;
		}

		if(m_ChkVerifyDesktopSwitch.GetCheck())
		{
			TCHAR szMessage[ARRAY_SIZE] = {0};
			wsprintf(szMessage, _T("Are you sure to switch to '%s' Desktop ?"), szDesktopName);

			if(IDNO == MessageBox(szMessage, TXT_MESSAGEBOX_TITLE, MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL))
				return;
		}

		//Exit the application from current desktop.
		if(CDesktopManager::SwitchDesktop(szDesktopName))
		{
			TCHAR szAppName[ARRAY_SIZE] = {0};
			//Get the application full path, so that it can be launch into the switching desktop.
			GetModuleFileName(GetModuleHandle(NULL), szAppName, ARRAY_SIZE - 1);

			//Launch the same application into the switching desktop.
			CDesktopManager::LaunchApplication(szAppName, szDesktopName);
			UnRegisterApplicationHotKeys();
			PostQuitMessage(0);
		}
	}
	catch(...)
	{
		DebugPrintErrorMessage();
		OutputDebugString(_T("\nCVirtualDesktopDlg::SwitchDesktopTo:\tException caught in CVirtualDesktopDlg::SwitchDesktopTo."));
	}
}

void CVirtualDesktopDlg::OnBnClickedLaunchApplication()
{
	// TODO: Add your control notification handler code here

	TCHAR szDesktopName[ARRAY_SIZE] = {0};
	m_DesktopListControl.GetText(m_DesktopListControl.GetCurSel(), szDesktopName);

	//Checking the Desktop name.
	if(!_tcslen(szDesktopName))
	{
		MessageBox(_T("Please select the desktop name from the list. And click 'Launch Application' button"), TXT_MESSAGEBOX_TITLE);
		//Making the Manage Desktop Dialog Visible. (If selected "Launch Application" From context menu.
		ShowManageDesktopsDialog();
		return;
	}

	if(!_tcsicmp(szDesktopName, _T("WinLogon")) || !_tcsicmp(szDesktopName, _T("Disconnect")) ) 
	{
		MessageBox(_T("Application cann't be launched in this Desktop."), TXT_MESSAGEBOX_TITLE);
		//Making the Manage Desktop Dialog Visible. (If selected "Launch Application" From context menu.
		ShowManageDesktopsDialog();
		return;
	}

	CFileDialog dlgOpen(TRUE, _T("*.Exe|"), NULL, 4|2, _T("Applications (*.Exe)|*.Exe|"), this);

	if(IDOK == dlgOpen.DoModal())
	{
		CString szFileName = dlgOpen.GetPathName();

		TCHAR szMessage[ARRAY_SIZE] = {0};
		//Launching the selected application.
		if(CDesktopManager::LaunchApplication(szFileName.GetBuffer(), szDesktopName))
		{
			wsprintf(szMessage, _T("Application is launched into the Desktop '%s'."), szDesktopName);
			MessageBox(szMessage, TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION);
		}
		else
		{
			wsprintf(szMessage, _T("Failed to launch application into the Desktop '%s'."), szDesktopName);
			MessageBox(szMessage, TXT_MESSAGEBOX_TITLE, MB_ICONERROR);
		}

		szFileName.ReleaseBuffer();
	}
}

LRESULT CVirtualDesktopDlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	OutputDebugString(_T("\nCVirtualDesktopDlg::OnHotKey:\tHot Key Is Pressed."));

	TCHAR szSwitchToDesktopName[ARRAY_SIZE] = {0};
	//Get the desktop name to be switched to.
	int iDesktopCount = CDesktopManager::GetDesktopCount();
	CDesktopManager::GetDesktopName(iDesktopCount - ((int)wParam - BASE_HOT_KEY_ID) - 1, szSwitchToDesktopName);

	OutputDebugString(_T("\nCVirtualDesktopDlg::OnHotKey:\tSwitch Request To :"));
	OutputDebugString(szSwitchToDesktopName);
	//Switch to the selected desktop.
	if(_tcslen(szSwitchToDesktopName))
		SwitchDesktopTo(szSwitchToDesktopName);

	return 1;
}


bool CVirtualDesktopDlg::RegisterApplicationHotKeys(void)
{
	bool bReturn = false;

	try
	{
		int iDesktopCount = CDesktopManager::GetDesktopCount();

		for(int iCounter = 0; iCounter < iDesktopCount; iCounter++)
		{
			if(!RegisterHotKey(m_hWnd, BASE_HOT_KEY_ID + iCounter, MOD_CONTROL | MOD_SHIFT, (0x31) + iCounter ))
				throw _T("\nHot Key Registration Failed.");
			else
				OutputDebugString(_T("\nCVirtualDesktopDlg::RegisterApplicationHotKeys:\tHot Key Registered Successfully."));
		}

		bReturn = true;
	}
	catch(TCHAR *pszErrorString)
	{
		OutputDebugString(_T("\nCVirtualDesktopDlg::RegisterApplicationHotKeys:\tCustom Exception caught in RegisterApplicationHotKeys."));
		DebugPrintErrorMessage(pszErrorString);
		bReturn = false;
	}
	catch(...)
	{
		OutputDebugString(_T("\nCVirtualDesktopDlg::RegisterApplicationHotKeys:\tException caught in RegisterApplicationHotKeys."));
		bReturn = false;
		DebugPrintErrorMessage();
	}

	return bReturn;
}

bool CVirtualDesktopDlg::UnRegisterApplicationHotKeys(void)
{
	bool bReturn = false;
	
	try
	{
		int iDesktopCount = CDesktopManager::GetDesktopCount() - 1;

		for(int iCounter = 0; iCounter < iDesktopCount; iCounter++)
		{
			if(!UnregisterHotKey(m_hWnd, BASE_HOT_KEY_ID + iCounter))
				throw _T("\nHot Key Registration Failed.");
			else
				OutputDebugString(_T("\nCVirtualDesktopDlg::UnRegisterApplicationHotKeys:\tHot Key UnRegistered Successfully."));
		}

		bReturn = true;
	}
	catch(TCHAR *pszErrorString)
	{
		int i = GetLastError();
		OutputDebugString(_T("\nCVirtualDesktopDlg::UnRegisterApplicationHotKeys:\tCustom Exception caught in UnRegisterApplicationHotKeys."));
		DebugPrintErrorMessage(pszErrorString);
		bReturn = false;
	}
	catch(...)
	{
		DebugPrintErrorMessage();
		OutputDebugString(_T("\nCVirtualDesktopDlg::UnRegisterApplicationHotKeys:\tException caught in UnRegisterApplicationHotKeys."));
		bReturn = false;
	}

	return bReturn;
}

bool CVirtualDesktopDlg::UpdateHotKeys(void)
{
	bool bReturn = false;

	if(!(bReturn = UnRegisterApplicationHotKeys()) )
		MessageBox(_T("Failed to unregister application hot keys."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);

	if(!RegisterApplicationHotKeys())
		MessageBox(_T("Failed to register application hot keys."), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
	else
		bReturn = !bReturn ? false : true;

	return bReturn;
}

void CVirtualDesktopDlg::OnBnClickedVerifyCheck()
{
	// TODO: Add your control notification handler code here
	CRegSettings objRegSettingsReader;
	objRegSettingsReader.SetProfileInt(REG_KEY_COMMON_SETTINGS, REG_SUB_KEY_CONFIRM_SWITCH, m_ChkVerifyDesktopSwitch.GetCheck());
}
