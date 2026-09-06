#include "stdafx.h"
#include "resource.h"

#define MOVETO_MENUID 100
#define DESKTOP_NAMES_MENUID 200

#pragma data_seg (".SHARED")
	HHOOK g_hPreviousMsgHook = 0;
	HHOOK g_hPreviousWinProcHook = 0;

	HINSTANCE g_hInstance = 0;

	int g_iDesktopCount = 0;
#pragma data_seg()

#pragma comment(linker,"/SECTION:.SHARED,RWS")


bool LaunchApplication(TCHAR *szApplicationName,TCHAR *szDesktopName,bool bSwitchDesktop = false);
bool InsertMenu(HWND hWnd);

//Windows procedure hook (filter) function.
LRESULT CALLBACK WinProcHookProcedure(int nCode, WPARAM wParam, LPARAM lParam)
{
	if(0 > nCode)
		return CallNextHookEx(g_hPreviousWinProcHook ,nCode,wParam,lParam);

	CWPSTRUCT *pwsStruct = (CWPSTRUCT *) lParam;
	
	switch(pwsStruct->message)
	{
		//Handle the init menu message.
	case WM_INITMENU:
		{
			HMENU hMenu = (HMENU) pwsStruct->wParam;
			OutputDebugString(_T("\nWinProcHookProcedure:\tMenu WM_INITMENUPOPUP Event Fired."));

			TCHAR szWindowText[ARRAY_SIZE] = {0};
			GetWindowText(pwsStruct->hwnd, szWindowText, ARRAY_SIZE);

			//Hook "Move To" Menu Into System Menu.
			if(0 != _tcscmp(szWindowText, _T("Virtual Desktop !")))
				InsertMenu(pwsStruct->hwnd);
		}
	}

	return CallNextHookEx(g_hPreviousWinProcHook ,nCode,wParam,lParam);
}

//Set the Windows procedure filter.
bool InstallWinProcHook(void)
{
	bool bReturn = false;
	try
	{
		//Set the Windows procedure filter.
		OutputDebugString(_T("\nInstallWinProcHook:\tWinProc Event Hooked."));
		g_hPreviousWinProcHook = SetWindowsHookEx(WH_CALLWNDPROC,&WinProcHookProcedure,g_hInstance,0);

		if(NULL == g_hPreviousWinProcHook)
		{
			TCHAR szError[ARRAY_SIZE] = {0};
			wsprintf(szError,_T("Last Error : %d"),GetLastError());
			OutputDebugString(_T("\nInstallWinProcHook:\tFailed to Hook WinProc Event.\n"));
			OutputDebugString(szError);

			bReturn = false;
		}
		else
		{
			OutputDebugString(_T("\nInstallWinProcHook:\tWinProc Event Hooked.\n"));
			bReturn = true;
		}
	}
	catch(...)
	{
		OutputDebugString(_T("\nInstallWinProcHook:\tException caught in InstallWinProcHook."));
		bReturn = false;
	}

	return bReturn;
}

//Rmoves the windows procedure filter.
bool UnInstallWinProcHook()
{
	bool bReturn = false;

	try
	{
		//Rmoves the windows procedure filter.
		OutputDebugString(_T("\nUnInstallWinProcHook:\tWinProc Event UnHooking.\n"));
		if(UnhookWindowsHookEx(g_hPreviousWinProcHook) == FALSE)
		{
			TCHAR szError[ARRAY_SIZE] = {0};
			wsprintf(szError,_T("Last Error : %d"),GetLastError());
			OutputDebugString(_T("\nUnInstallWinProcHook:\tFailed to UnHook WinProc Event.\n"));
			OutputDebugString(szError);
			bReturn = false;
		}
		else
		{
			OutputDebugString(_T("\nUnInstallWinProcHook:\tWinProc Event UnHooked.\n"));
			bReturn = true;
		}
	}
	catch(...)
	{
		OutputDebugString(_T("\nUnInstallWinProcHook:\tException caught in UnInstallWinProcHook."));
		bReturn = false;
	}

	return bReturn;
}

//The message hook (filter) function.
LRESULT CALLBACK MessageHookProcedure(int nCode, WPARAM wParam, LPARAM lParam)
{
	bool bReturn = false;
	try
	{
		if(0 > nCode)
			return CallNextHookEx(g_hPreviousMsgHook,nCode,wParam,lParam);

		MSG *pMsg = (MSG *) lParam;
		switch(pMsg->message)
		{
			//Handle only if menu items from application cotrol panel is selected.
			case WM_SYSCOMMAND:
				//Handle only inserted menu item by our hook.
				if(wParam && (DESKTOP_NAMES_MENUID <= LOWORD(pMsg->wParam) && (DESKTOP_NAMES_MENUID + g_iDesktopCount) >= LOWORD(pMsg->wParam)))
				{
					if(IDYES == MessageBox(pMsg->hwnd,_T("Moving application to other desktop means closing it from this current desktop \nand launching into other desktop. Doing so, you may loose any unsaved data.\n\t\t Are you sure to move ?"),_T("Virtual Desktop"),MB_YESNO | MB_ICONINFORMATION))
					{
						TCHAR szMenuNumber[100] = {0};
						wsprintf(szMenuNumber,_T("\nMenu Number :%d"),LOWORD(pMsg->wParam) - DESKTOP_NAMES_MENUID);
						OutputDebugString(szMenuNumber);

						HMENU hSysMenu = GetSystemMenu(pMsg->hwnd,FALSE);
						TCHAR szDesktopName[ARRAY_SIZE] = {0};
						TCHAR szApplicationFile[ARRAY_SIZE] = {0};

						//Retrive the menu item string, which identifies the selected desktop name.
						GetMenuString(hSysMenu,LOWORD(pMsg->wParam),szDesktopName,ARRAY_SIZE -1 ,MF_BYCOMMAND);
						OutputDebugString(szDesktopName);

						TCHAR szCurrentDesktopName[ARRAY_SIZE] = {0};
						DWORD iOutCount = 0;

						HDESK hCurrentDesktop = GetThreadDesktop(GetCurrentThreadId());
						GetUserObjectInformation(hCurrentDesktop, UOI_NAME, szCurrentDesktopName, ARRAY_SIZE - 1, &iOutCount);

						if(!_tcsicmp(szDesktopName, _T("WinLogon")) || !_tcsicmp(szDesktopName, _T("Disconnect")) ) 
						{
							MessageBox(NULL, _T("Application cann't be launched in this Desktop."), _T("Virtual Desktop"), MB_ICONINFORMATION);
						}else if(_tcsicmp(szDesktopName, szCurrentDesktopName) == 0)
						{
							MessageBox(NULL, _T("Application is currently in the same desktop."), _T("Virtual Desktop"), MB_ICONINFORMATION);
						}
						else
						{
							//Retrive the module file path, to launch it in different desktop.
							HMODULE hModule = (HMODULE) OpenProcess(0,FALSE,GetCurrentThreadId());
							GetModuleFileName(hModule,szApplicationFile,ARRAY_SIZE - 1);

							ShowWindow(pMsg->hwnd, SW_HIDE);
							TCHAR szMessage[ARRAY_SIZE] = {0};
							//Launch the same application into the switching desktop.
							if(LaunchApplication(szApplicationFile, szDesktopName))
							{
								wsprintf(szMessage, _T("Application is launched into the Desktop '%s'."), szDesktopName);
								MessageBox(NULL, szMessage, _T("Virtual Desktop"), MB_ICONINFORMATION);
								PostQuitMessage(0);
							}
							else
							{
								ShowWindow(pMsg->hwnd, SW_SHOW);
								wsprintf(szMessage, _T("Failed to launch application into the Desktop '%s'."), szDesktopName);
								MessageBox(NULL, szMessage, _T("Virtual Desktop"), MB_ICONERROR);
							}
						}
					}
				}
				break;
		}
	}
	catch(...)
	{
		OutputDebugString(_T("\nMessageHookProcedure:\tException caught in CALLBACK MenuHookProcedure.\n"));
	}

	return CallNextHookEx(g_hPreviousMsgHook,nCode,wParam,lParam);
}

//Set the message hook (Filter).
bool InstallMessageHook(void)
{
	bool bReturn = false;
	try
	{
		//Set the message hook (Filter).
		g_hPreviousMsgHook = SetWindowsHookEx(WH_GETMESSAGE,&MessageHookProcedure,g_hInstance,0);

		if(NULL == g_hPreviousMsgHook)
		{
			OutputDebugString(_T("\nInstallMessageHook:\tFailed to Hook Messages.\n"));
			bReturn = false;
		}
		else
		{
			OutputDebugString(_T("\nInstallMessageHook:\tMessages Hooked.\n"));
			bReturn = true;
		}
	}
	catch(...)
	{
		OutputDebugString(_T("\nInstallMessageHook:\tException caught in InstallMessageHook."));
		bReturn = false;
	}

	return bReturn;
}

//Removes the message hook (filter)
bool UnInstallMsgHook()
{
	bool bReturn = false;

	try
	{
		//Removes the message hook (Filter).
		OutputDebugString(_T("\nUnInstallMsgHook:\tMessages UnHooking.\n"));
		if(UnhookWindowsHookEx(g_hPreviousMsgHook) == FALSE)
		{
			TCHAR szError[ARRAY_SIZE] = {0};
			wsprintf(szError,_T("Last Error : %d"),GetLastError());
			OutputDebugString(_T("\nUnInstallMsgHook:\tFailed to UnHook Messages Hook.\n"));
			OutputDebugString(szError);
			bReturn = false;
		}
		else
		{
			OutputDebugString(_T("\nUnInstallMsgHook:\tMessages UnHooked.\n"));
			bReturn = true;
		}
	}
	catch(...)
	{
		OutputDebugString(_T("\nUnInstallMsgHook:\tException caught in UnInstallMsgHook."));
		bReturn = false;
	}

	return bReturn;
}

//Enumerate all the desktop of the calling thread's window station.
BOOL CALLBACK EnumDesktopProc(LPTSTR lpszDesktopName,LPARAM lParam)
{
	try
	{
		HMENU hSubMenu = (HMENU) lParam;
		//Append the desktop name as menu item into the applications control panel menu (system menu). 
		AppendMenu(hSubMenu, MF_BYPOSITION|MF_STRING,DESKTOP_NAMES_MENUID + g_iDesktopCount++,lpszDesktopName);
		//OutputDebugString(_T("\nDesktop Name :"));
		//OutputDebugString(lpszDesktopName);
	}
	catch(...)
	{
		OutputDebugString(_T("\nEnumDesktopProc:\tException caught in EnumDesktopsProc."));
	}

	return TRUE;
}


//Launches the specified application (file) at specified desktop.
bool LaunchApplication(TCHAR *szApplicationName,TCHAR *szDesktopName,bool bSwitchDesktop)
{
	bool bReturn = false;

	if(NULL == szApplicationName || NULL == szDesktopName)
		return false;

	try
	{
		STARTUPINFO sInfo = {0};
		PROCESS_INFORMATION pInfo = {0};

		sInfo.cb = sizeof(sInfo);

		//Set the desktop name of the process to be launched.
		sInfo.lpDesktop = szDesktopName;

		TCHAR szDirectoryName[ARRAY_SIZE] = {0};
		GetCurrentDirectory(ARRAY_SIZE - 1,szDirectoryName);
		
		//Launch the process.
		BOOL bCreateProcessReturn = CreateProcess(szApplicationName,
			GetCommandLine(),
			NULL,
			NULL,
			TRUE,
			NORMAL_PRIORITY_CLASS,
			NULL,
			szDirectoryName,
			&sInfo,
			&pInfo);

		TCHAR *pszError = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,GetLastError(),0,(LPWSTR) &pszError,0,NULL);
		OutputDebugString(_T("\n\t\t"));
		OutputDebugString(pszError);

		if(bSwitchDesktop)
		{
			HDESK hDesktopToSwitch = OpenDesktop(szDesktopName,DF_ALLOWOTHERACCOUNTHOOK,TRUE,GENERIC_ALL);
			if(NULL == hDesktopToSwitch)
			{
				TCHAR *pszError = NULL;
				TCHAR szErrorMsg[ARRAY_SIZE] = {0};
				int iErrorCode = GetLastError();

				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,iErrorCode,0,(LPWSTR) &pszError,0,NULL);
				wsprintf(szErrorMsg,_T("Failed to switch to %s desktop.\n\t %s"),szDesktopName,pszError);
				MessageBox(NULL,szErrorMsg,_T("Virtual Desktop"),MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
				OutputDebugString(pszError);
			}
			else if(FALSE == ::SwitchDesktop(hDesktopToSwitch))
			{
				MessageBox(NULL,_T("Failed to Switch Desktop."), _T("Virtual Desktop"), MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
				OutputDebugString(_T("\nLaunchApplication:\tSwitchDesktop Failed in LaunchApplication."));	
			}
		}

		bReturn = true;
	}
	catch(...)
	{
		OutputDebugString(_T("\nLaunchApplication:\tException caught in LaunchApplication."));
		bReturn = false;
	}

	return bReturn;
}

bool InsertMenu(HWND hWnd)
{
	bool bReturn = false;

	try
	{
		HMENU hSysMenu = GetSystemMenu(hWnd, TRUE);
		hSysMenu = GetSystemMenu(hWnd, FALSE);
		//AppendMenu(hSysMenu, MF_BYPOSITION|MF_STRING,MOVETO_MENUID,_T("Move &To"));
		OutputDebugString(_T("\nInsertMenu:\tAdded menu to window in InsertMenu."));
		HMENU hSubMenu = NULL;
		hSubMenu = CreateMenu();

		TCHAR szDesktopList[ARRAY_SIZE] = {0};

		//Opens the handle to current process's window station, to enumerate all the desktops.
		HWINSTA hWindowsStation = GetProcessWindowStation();
		if(NULL == hWindowsStation)
			throw false;

		g_iDesktopCount = 0;
		//Add all the desktop names to the menu as sub menu items.
		bReturn = (FALSE != EnumDesktops(hWindowsStation,&EnumDesktopProc,(LPARAM) hSubMenu)); //m_szDesktopNames));

		//Add the "Move To" option menu to the system menu.
		if(bReturn && (g_iDesktopCount > 0))
			AppendMenu(hSysMenu, MF_POPUP, (UINT_PTR)hSubMenu,_T("Move &To"));
		else
			AppendMenu(hSysMenu, MF_POPUP | MF_GRAYED,(UINT_PTR)hSubMenu,_T("Move &To"));

		bReturn = true;
	}
	catch(...)
	{
		OutputDebugString(_T("\nInsertMenu:\tException caught in InsertMenu."));
	}

	return bReturn;
}
