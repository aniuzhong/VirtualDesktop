#include "stdafx.h"
#include "resource.h"
#include "DesktopManager.h"
#include "RegSettings.h"
#include "Virtual DesktopDlg.h"

static const UINT WM_TRAYICON_NOTIFY_MESSAGE = RegisterWindowMessageW(L"WM_TRAYICON_NOTIFY_MESSAGE-{8DDBE93E-DFE8-4279-934E-05C39902F37D}");

namespace
{
    const UINT CONTEXT_MENU_IDS = 600;
    const UINT MANAGE_DESKTOP_MENU_ID = 500;
    const UINT VERIFY_SWITCH_MENU_ID = 501;
    const UINT LAUNCH_APP_MENU_ID = 502;
    const UINT EXIT_MENU_ID = 503;
    const UINT SEPARATOR_MENU_ID = 504;

    HWND g_hDlg = nullptr;
    HINSTANCE g_hInstance = nullptr;

    HWND g_hDesktopList = nullptr;
    HWND g_hDesktopName = nullptr;
    HWND g_hAddNewDesktop = nullptr;
    HWND g_hSwitchToDesktop = nullptr;

    bool IsVerifyChecked(void)
    {
        return IsDlgButtonChecked(g_hDlg, IDC_VERIFY_CHECK) == BST_CHECKED;
    }

    std::wstring GetSelectedDesktopName(void)
    {
        int iSel = static_cast<int>(SendMessageW(g_hDesktopList, LB_GETCURSEL, 0, 0));
        if (LB_ERR == iSel)
            return std::wstring();

        wchar_t szName[ARRAY_SIZE] = { 0 };
        SendMessageW(g_hDesktopList, LB_GETTEXT, iSel, reinterpret_cast<LPARAM>(szName));
        return szName;
    }

    void ShowAboutBox(void)
    {
        DialogBoxParamW(g_hInstance, MAKEINTRESOURCEW(IDD_ABOUTBOX), g_hDlg, [](HWND hDlg, UINT message, WPARAM wParam, LPARAM) -> INT_PTR {
            switch (message)
            {
            case WM_INITDIALOG:
                return TRUE;
            case WM_COMMAND:
                if (IDOK == LOWORD(wParam) || IDCANCEL == LOWORD(wParam))
                {
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
                }
                break;
            }
            return FALSE;
        }, 0);
    }

    bool AddTrayIcon(HWND hDlg)
    {
        NOTIFYICONDATAW nData = { 0 };
        nData.cbSize = sizeof(nData);
        nData.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDR_MAINFRAME));
        nData.hWnd = hDlg;
        nData.uID = 1;
        nData.uCallbackMessage = WM_TRAYICON_NOTIFY_MESSAGE;
        nData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

        std::wstring tip = TXT_MESSAGEBOX_TITLE;
        std::wstring currentDesktop = CDesktopManager::GetCurrentDesktopName();
        if (!currentDesktop.empty())
            tip += L" [" + currentDesktop + L" Desktop]";
        wcsncpy_s(nData.szTip, tip.c_str(), _TRUNCATE);

        return Shell_NotifyIconW(NIM_ADD, &nData) != FALSE;
    }

    void RemoveTrayIcon(HWND hDlg)
    {
        NOTIFYICONDATAW nData = { 0 };
        nData.cbSize = sizeof(nData);
        nData.hWnd = hDlg;
        nData.uID = 1;
        Shell_NotifyIconW(NIM_DELETE, &nData);
    }

    bool RegisterApplicationHotKeys(void)
    {
        int iDesktopCount = CDesktopManager::GetDesktopCount();
        for (int iCounter = 0; iCounter < iDesktopCount; iCounter++)
        {
            if (!RegisterHotKey(g_hDlg, BASE_HOT_KEY_ID + iCounter, MOD_CONTROL | MOD_SHIFT, L'1' + iCounter))
            {
                DebugPrintErrorMessage(L"Hot Key Registration Failed.");
                return false;
            }
        }
        return true;
    }

    bool UnRegisterApplicationHotKeys(void)
    {
        int iDesktopCount = CDesktopManager::GetDesktopCount() - 1;
        bool bReturn = true;
        for (int iCounter = 0; iCounter < iDesktopCount; iCounter++)
        {
            if (!UnregisterHotKey(g_hDlg, BASE_HOT_KEY_ID + iCounter))
            {
                DebugPrintErrorMessage(L"Hot Key UnRegistration Failed.");
                bReturn = false;
            }
        }
        return bReturn;
    }

    bool UpdateHotKeys(void)
    {
        bool bReturn = UnRegisterApplicationHotKeys();
        if (!bReturn)
            MessageBoxW(g_hDlg, L"Failed to unregister application hot keys.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);

        if (!RegisterApplicationHotKeys())
        {
            MessageBoxW(g_hDlg, L"Failed to register application hot keys.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            return false;
        }
        return bReturn;
    }

    void SwitchDesktopTo(const std::wstring& desktopName)
    {
        if (desktopName.empty())
            return;

        SetForegroundWindow(g_hDlg);

        if (CDesktopManager::IsCurrentDesktop(desktopName))
        {
            MessageBoxW(g_hDlg, L"You are currently on the same Desktop.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            return;
        }

        if (IsVerifyChecked())
        {
            std::wstring message = L"Are you sure to switch to '" + desktopName + L"' Desktop ?";
            if (IDNO == MessageBoxW(g_hDlg, message.c_str(), TXT_MESSAGEBOX_TITLE, MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL))
                return;
        }

        if (CDesktopManager::SwitchDesktop(desktopName))
        {
            wchar_t szAppName[ARRAY_SIZE] = { 0 };
            GetModuleFileNameW(nullptr, szAppName, ARRAY_SIZE - 1);
            CDesktopManager::LaunchApplication(szAppName, desktopName);
            UnRegisterApplicationHotKeys();
            PostQuitMessage(0);
        }
    }

    void ShowManageDesktopsDialog(void)
    {
        int iDeskCount = CDesktopManager::GetDesktopCount();
        SendMessageW(g_hDesktopList, LB_RESETCONTENT, 0, 0);
        for (int i = 0; i < iDeskCount; i++)
        {
            std::wstring name = CDesktopManager::GetDesktopName(i);
            SendMessageW(g_hDesktopList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
        }

        wchar_t szSelectedDesktopName[ARRAY_SIZE] = { 0 };
        GetWindowTextW(g_hDesktopName, szSelectedDesktopName, ARRAY_SIZE - 1);

        if (szSelectedDesktopName[0] && LB_ERR != SendMessageW(g_hDesktopList, LB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(szSelectedDesktopName)))
            EnableWindow(g_hDesktopName, FALSE);
        else
        {
            SetWindowTextW(g_hDesktopName, L"");
            EnableWindow(g_hDesktopName, TRUE);
        }

        ShowWindow(g_hDlg, SW_SHOW);
    }

    void OnDesktopListSelChange(void)
    {
        std::wstring name = GetSelectedDesktopName();
        SetWindowTextW(g_hDesktopName, name.c_str());

        SetWindowTextW(g_hAddNewDesktop, L"&New");
        EnableWindow(g_hSwitchToDesktop, TRUE);
        EnableWindow(g_hDesktopName, FALSE);
    }

    void OnAddNewDesktop(void)
    {
        wchar_t szCaption[ARRAY_SIZE] = { 0 };
        GetWindowTextW(g_hAddNewDesktop, szCaption, ARRAY_SIZE - 1);

        if (_wcsicmp(szCaption, L"&New") != 0)
        {
            std::wstring name = [&] {
                wchar_t szName[ARRAY_SIZE] = { 0 };
                GetWindowTextW(g_hDesktopName, szName, ARRAY_SIZE - 1);
                return std::wstring(szName);
            }();

            size_t begin = name.find_first_not_of(L' ');
            name = (std::wstring::npos == begin) ? std::wstring() : name.substr(begin);
            while (!name.empty() && name.back() == L' ')
                name.pop_back();

            if (name.empty())
            {
                MessageBoxW(g_hDlg, L"Please enter Desktop Name", TXT_MESSAGEBOX_TITLE, MB_ICONEXCLAMATION | MB_TOPMOST | MB_TASKMODAL);
                SetWindowTextW(g_hDesktopName, L"");
                SetFocus(g_hDesktopName);
                return;
            }

            if (LB_ERR != SendMessageW(g_hDesktopList, LB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(name.c_str())))
                MessageBoxW(g_hDlg, L"Desktop already created !", TXT_MESSAGEBOX_TITLE, MB_ICONEXCLAMATION | MB_TOPMOST | MB_TASKMODAL);

            if (CDesktopManager::CreateDesktop(name))
            {
                if (IDYES == MessageBoxW(g_hDlg, L"New Desktop is been created.\nWould you like to switch to new desktop ?", TXT_MESSAGEBOX_TITLE, MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL))
                    SwitchDesktopTo(name);

                SendMessageW(g_hDesktopList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
                SendMessageW(g_hDesktopList, LB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));

                OnDesktopListSelChange();
                UpdateHotKeys();
            }
        }
        else
        {
            SetWindowTextW(g_hAddNewDesktop, L"&Add");
            SetWindowTextW(g_hDesktopName, L"");
            EnableWindow(g_hDesktopName, TRUE);
            EnableWindow(g_hSwitchToDesktop, FALSE);
            SetFocus(g_hDesktopName);
        }
    }

    void OnSwitchToDesktop(void)
    {
        SwitchDesktopTo(GetSelectedDesktopName());
    }

    void OnLaunchApplication(void)
    {
        std::wstring desktopName = GetSelectedDesktopName();
        if (desktopName.empty())
        {
            MessageBoxW(g_hDlg, L"Please select the desktop name from the list. And click 'Launch Application' button", TXT_MESSAGEBOX_TITLE, MB_OK);
            ShowManageDesktopsDialog();
            return;
        }

        if (_wcsicmp(desktopName.c_str(), L"WinLogon") == 0 || _wcsicmp(desktopName.c_str(), L"Disconnect") == 0)
        {
            MessageBoxW(g_hDlg, L"Application cann't be launched in this Desktop.", TXT_MESSAGEBOX_TITLE, MB_OK);
            ShowManageDesktopsDialog();
            return;
        }

        wchar_t szFileName[ARRAY_SIZE] = { 0 };
        OPENFILENAMEW ofn = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = g_hDlg;
        ofn.lpstrFilter = L"Applications (*.Exe)\0*.Exe\0";
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = ARRAY_SIZE;
        ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

        if (GetOpenFileNameW(&ofn))
        {
            if (CDesktopManager::LaunchApplication(szFileName, desktopName))
            {
                std::wstring message = L"Application is launched into the Desktop '" + desktopName + L"'.";
                MessageBoxW(g_hDlg, message.c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION);
            }
            else
            {
                std::wstring message = L"Failed to launch application into the Desktop '" + desktopName + L"'.";
                MessageBoxW(g_hDlg, message.c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONERROR);
            }
        }
    }

    void OnHotKey(WPARAM wParam)
    {
        int iDesktopCount = CDesktopManager::GetDesktopCount();
        int iDesktopIndex = iDesktopCount - (static_cast<int>(wParam) - BASE_HOT_KEY_ID) - 1;
        std::wstring desktopName = CDesktopManager::GetDesktopName(iDesktopIndex);
        if (!desktopName.empty())
            SwitchDesktopTo(desktopName);
    }

    void OnTrayMessage(WPARAM, LPARAM lParam)
    {
        UINT uMsg = static_cast<UINT>(lParam);
        if (WM_RBUTTONDOWN != uMsg && WM_CONTEXTMENU != uMsg)
            return;

        POINT pt;
        GetCursorPos(&pt);

        int iDesktopCount = CDesktopManager::GetDesktopCount();
        int iHotKeyCounter = iDesktopCount;

        wil::unique_hmenu hContextMenu(CreatePopupMenu());

        for (int iMenuItemCount = 0; iMenuItemCount < iDesktopCount; iMenuItemCount++)
        {
            std::wstring desktopName = CDesktopManager::GetDesktopName(iMenuItemCount);
            std::wstring menuItemName = desktopName + L"\tCtrl + Shift + " + std::to_wstring(iHotKeyCounter--);

            UINT flags = MF_STRING | MF_ENABLED | (CDesktopManager::IsCurrentDesktop(desktopName) ? MF_CHECKED : 0);
            AppendMenuW(hContextMenu.get(), flags, CONTEXT_MENU_IDS + iMenuItemCount, menuItemName.c_str());
        }

        if (iDesktopCount > 0)
            AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, nullptr);

        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING | (IsVerifyChecked() ? MF_CHECKED : 0), VERIFY_SWITCH_MENU_ID, TXT_CONFIRM_MENU_ITEM);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING, MANAGE_DESKTOP_MENU_ID, TXT_MANAGE_DESKTOP_MENU_ITEM);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING, LAUNCH_APP_MENU_ID, TXT_LAUNCH_APPLICATION_MENU_ITEM);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, nullptr);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING, IDM_ABOUTBOX, TXT_ABOUT_MENU_ITEM);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, nullptr);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING, EXIT_MENU_ID, TXT_EXIT_MENU_ITEM);

        SetForegroundWindow(g_hDlg);
        int iSelectedIndex = TrackPopupMenuEx(hContextMenu.get(), TPM_TOPALIGN | TPM_VERPOSANIMATION | TPM_RETURNCMD, pt.x, pt.y, g_hDlg, nullptr);

        if (IDM_ABOUTBOX == iSelectedIndex)
            ShowAboutBox();
        else if (EXIT_MENU_ID == iSelectedIndex)
        {
            UnRegisterApplicationHotKeys();
            PostQuitMessage(0);
        }
        else if (MANAGE_DESKTOP_MENU_ID == iSelectedIndex)
            ShowManageDesktopsDialog();
        else if (VERIFY_SWITCH_MENU_ID == iSelectedIndex)
        {
            CheckDlgButton(g_hDlg, IDC_VERIFY_CHECK, IsVerifyChecked() ? BST_UNCHECKED : BST_CHECKED);
            RegSettings::SetProfileInt(REG_KEY_COMMON_SETTINGS, REG_SUB_KEY_CONFIRM_SWITCH, IsVerifyChecked() ? 1 : 0);
        }
        else if (LAUNCH_APP_MENU_ID == iSelectedIndex)
            OnLaunchApplication();
        else if (iSelectedIndex >= static_cast<int>(CONTEXT_MENU_IDS))
        {
            std::wstring desktopName = CDesktopManager::GetDesktopName(iSelectedIndex - CONTEXT_MENU_IDS);
            if (!desktopName.empty())
                SwitchDesktopTo(desktopName);
        }
    }

    void OnInitDialog(HWND hDlg)
    {
        g_hDesktopList = GetDlgItem(hDlg, IDC_DESKTOP_LIST);
        g_hDesktopName = GetDlgItem(hDlg, IDC_DESKTOP_NAME);
        g_hAddNewDesktop = GetDlgItem(hDlg, IDC_ADD_NEW_DESKTOP);
        g_hSwitchToDesktop = GetDlgItem(hDlg, IDC_SWITCH_TO_DESKTOP);

        wchar_t szAboutMenu[ARRAY_SIZE] = { 0 };
        if (LoadStringW(g_hInstance, IDS_ABOUTBOX, szAboutMenu, ARRAY_SIZE) && szAboutMenu[0])
        {
            HMENU hSysMenu = GetSystemMenu(hDlg, FALSE);
            if (hSysMenu)
            {
                AppendMenuW(hSysMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hSysMenu, MF_STRING, IDM_ABOUTBOX, szAboutMenu);
            }
        }

        HICON hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDR_MAINFRAME));
        SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
        SendMessageW(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));

        if (!AddTrayIcon(hDlg))
        {
            MessageBoxW(hDlg, L"Failed to set tray icon.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            PostQuitMessage(-1);
            return;
        }

        if (!RegisterApplicationHotKeys())
        {
            MessageBoxW(hDlg, L"Failed to register Hot Keys.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            PostQuitMessage(-1);
            return;
        }

        CheckDlgButton(hDlg, IDC_VERIFY_CHECK,
            RegSettings::ReadProfileInt(REG_KEY_COMMON_SETTINGS, REG_SUB_KEY_CONFIRM_SWITCH, 1) ? BST_CHECKED : BST_UNCHECKED);
    }

    INT_PTR CALLBACK VirtualDesktopDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_TRAYICON_NOTIFY_MESSAGE)
        {
            OnTrayMessage(wParam, lParam);
            return TRUE;
        }

        switch (message)
        {
        case WM_INITDIALOG:
            OnInitDialog(hDlg);
            return TRUE;

        case WM_SYSCOMMAND:
            if (IDM_ABOUTBOX == (wParam & 0xFFF0))
            {
                ShowAboutBox();
                return TRUE;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDOK:
                DestroyWindow(g_hDlg);
                return TRUE;
            case IDC_DESKTOP_LIST:
                if (LBN_SELCHANGE == HIWORD(wParam))
                {
                    OnDesktopListSelChange();
                    return TRUE;
                }
                break;
            case IDC_ADD_NEW_DESKTOP:
                if (BN_CLICKED == HIWORD(wParam))
                {
                    OnAddNewDesktop();
                    return TRUE;
                }
                break;
            case IDC_SWITCH_TO_DESKTOP:
                if (BN_CLICKED == HIWORD(wParam))
                {
                    OnSwitchToDesktop();
                    return TRUE;
                }
                break;
            case IDC_LAUNCH_APPLICATION:
                if (BN_CLICKED == HIWORD(wParam))
                {
                    OnLaunchApplication();
                    return TRUE;
                }
                break;
            case IDC_VERIFY_CHECK:
                if (BN_CLICKED == HIWORD(wParam))
                {
                    RegSettings::SetProfileInt(REG_KEY_COMMON_SETTINGS, REG_SUB_KEY_CONFIRM_SWITCH, IsVerifyChecked() ? 1 : 0);
                    return TRUE;
                }
                break;
            }
            break;

        case WM_HOTKEY:
            OnHotKey(wParam);
            return TRUE;

        case WM_DESTROY:
            RemoveTrayIcon(hDlg);
            g_hDlg = nullptr;
            break;
        }

        return FALSE;
    }
}

HWND CreateVirtualDesktopDialog(HINSTANCE hInstance)
{
    g_hInstance = hInstance;
    g_hDlg = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(IDD_VIRTUALDESKTOP_DIALOG), nullptr, VirtualDesktopDlgProc, 0);
    return g_hDlg;
}
