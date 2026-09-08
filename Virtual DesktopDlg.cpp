#include "stdafx.h"
#include "wilx/win32_helpers.h"
#include "resource.h"
#include "DesktopManager.h"
#include "Virtual DesktopDlg.h"

static const UINT WM_TRAYICON_NOTIFY_MESSAGE = RegisterWindowMessageW(L"WM_TRAYICON_NOTIFY_MESSAGE-{8DDBE93E-DFE8-4279-934E-05C39902F37D}");

namespace
{
    const UINT CONTEXT_MENU_IDS = 600;
    const UINT MANAGE_DESKTOP_MENU_ID = 500;
    const UINT VERIFY_SWITCH_MENU_ID = 501;
    const UINT EXIT_MENU_ID = 503;
    const UINT SEPARATOR_MENU_ID = 504;

    HWND g_hDlg = nullptr;
    HINSTANCE g_hInstance = nullptr;

    HWND g_hDesktopList = nullptr;
    HWND g_hDesktopName = nullptr;
    HWND g_hAddNewDesktop = nullptr;
    HWND g_hSwitchToDesktop = nullptr;

    wilx::unique_notify_icon_data g_trayIcon;

    bool IsVerifyChecked(void)
    {
        return IsDlgButtonChecked(g_hDlg, IDC_VERIFY_CHECK) == BST_CHECKED;
    }

    std::wstring GetSelectedDesktopName(void)
    {
        int iSel = static_cast<int>(SendMessageW(g_hDesktopList, LB_GETCURSEL, 0, 0));
        if (LB_ERR == iSel)
            return std::wstring();

        int iLength = static_cast<int>(SendMessageW(g_hDesktopList, LB_GETTEXTLEN, iSel, 0));
        std::wstring name(iLength + 1, L'\0');
        SendMessageW(g_hDesktopList, LB_GETTEXT, iSel, reinterpret_cast<LPARAM>(name.data()));
        name.resize(iLength);
        return name;
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
        g_trayIcon.cbSize = sizeof(g_trayIcon);
        g_trayIcon.hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDR_MAINFRAME));
        g_trayIcon.hWnd = hDlg;
        g_trayIcon.uID = 1;
        g_trayIcon.uCallbackMessage = WM_TRAYICON_NOTIFY_MESSAGE;
        g_trayIcon.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

        std::wstring tip = TXT_MESSAGEBOX_TITLE;
        std::wstring currentDesktop = wilx::TryGetThreadDesktopName();
        if (!currentDesktop.empty())
            tip += std::format(L" [{} Desktop]", currentDesktop);
        wcsncpy_s(g_trayIcon.szTip, tip.c_str(), _TRUNCATE);

        return Shell_NotifyIconW(NIM_ADD, &g_trayIcon) != FALSE;
    }

    void SwitchDesktopTo(const std::wstring& desktopName)
    {
        if (desktopName.empty())
            return;

        LogInfo(std::format(L"switch to desktop '{}'", desktopName));

        SetForegroundWindow(g_hDlg);

        if (DesktopManager::IsCurrentDesktop(desktopName))
        {
            MessageBoxW(g_hDlg, L"You are currently on the same Desktop.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            return;
        }

        if (IsVerifyChecked())
        {
            std::wstring message = std::format(L"Are you sure to switch to '{}' Desktop ?", desktopName);
            if (IDNO == MessageBoxW(g_hDlg, message.c_str(), TXT_MESSAGEBOX_TITLE, MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL))
                return;
        }

        if (DesktopManager::SwitchDesktop(desktopName))
        {
            std::wstring appName = wil::GetModuleFileNameW<std::wstring>(nullptr);
            DesktopManager::LaunchApplication(appName, desktopName);
            LogInfo(L"self relaunched on target desktop, this instance exits");
            DestroyWindow(g_hDlg);
        }
    }

    void ShowManageDesktopsDialog(void)
    {
        const std::vector<std::wstring> desktopNames = DesktopManager::GetDesktopNames();
        SendMessageW(g_hDesktopList, LB_RESETCONTENT, 0, 0);
        for (const std::wstring& name : desktopNames)
            SendMessageW(g_hDesktopList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));

        std::wstring selectedDesktopName = wilx::TryGetWindowText(g_hDesktopName);

        if (!selectedDesktopName.empty() && LB_ERR != SendMessageW(g_hDesktopList, LB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(selectedDesktopName.c_str())))
            EnableWindow(g_hDesktopName, FALSE);
        else
        {
            SetWindowTextW(g_hDesktopName, L"");
            EnableWindow(g_hDesktopName, TRUE);
        }

        ShowWindow(g_hDlg, SW_SHOW);
        SetForegroundWindow(g_hDlg);
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
        std::wstring caption = wilx::TryGetWindowText(g_hAddNewDesktop);

        if (_wcsicmp(caption.c_str(), L"&New") != 0)
        {
            std::wstring name = wilx::TryGetWindowText(g_hDesktopName);

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

            if (DesktopManager::CreateDesktop(name))
            {
                if (IDYES == MessageBoxW(g_hDlg, L"New Desktop is been created.\nWould you like to switch to new desktop ?", TXT_MESSAGEBOX_TITLE, MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL))
                    SwitchDesktopTo(name);

                SendMessageW(g_hDesktopList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
                SendMessageW(g_hDesktopList, LB_SELECTSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));

                OnDesktopListSelChange();
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

    void OnTrayMessage(WPARAM, LPARAM lParam)
    {
        UINT uMsg = static_cast<UINT>(lParam);
        if (WM_RBUTTONDOWN != uMsg && WM_CONTEXTMENU != uMsg)
            return;

        POINT pt;
        GetCursorPos(&pt);

        const std::vector<std::wstring> desktopNames = DesktopManager::GetDesktopNames();

        wil::unique_hmenu hContextMenu(CreatePopupMenu());

        for (size_t iMenuItem = 0; iMenuItem < desktopNames.size(); iMenuItem++)
        {
            UINT flags = MF_STRING | MF_ENABLED | (DesktopManager::IsCurrentDesktop(desktopNames[iMenuItem]) ? MF_CHECKED : 0);
            AppendMenuW(hContextMenu.get(), flags, CONTEXT_MENU_IDS + static_cast<UINT>(iMenuItem), desktopNames[iMenuItem].c_str());
        }

        if (!desktopNames.empty())
            AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, nullptr);

        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING | (IsVerifyChecked() ? MF_CHECKED : 0), VERIFY_SWITCH_MENU_ID, TXT_CONFIRM_MENU_ITEM);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING, MANAGE_DESKTOP_MENU_ID, TXT_MANAGE_DESKTOP_MENU_ITEM);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, nullptr);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING, IDM_ABOUTBOX, TXT_ABOUT_MENU_ITEM);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_SEPARATOR, SEPARATOR_MENU_ID, nullptr);
        AppendMenuW(hContextMenu.get(), MF_ENABLED | MF_STRING, EXIT_MENU_ID, TXT_EXIT_MENU_ITEM);

        SetForegroundWindow(g_hDlg);
        int iSelectedIndex = TrackPopupMenuEx(hContextMenu.get(), TPM_TOPALIGN | TPM_VERPOSANIMATION | TPM_RETURNCMD, pt.x, pt.y, g_hDlg, nullptr);

        if (IDM_ABOUTBOX == iSelectedIndex)
            ShowAboutBox();
        else if (EXIT_MENU_ID == iSelectedIndex)
            DestroyWindow(g_hDlg);
        else if (MANAGE_DESKTOP_MENU_ID == iSelectedIndex)
            ShowManageDesktopsDialog();
        else if (VERIFY_SWITCH_MENU_ID == iSelectedIndex)
        {
            CheckDlgButton(g_hDlg, IDC_VERIFY_CHECK, IsVerifyChecked() ? BST_UNCHECKED : BST_CHECKED);
        }
        else if (iSelectedIndex >= static_cast<int>(CONTEXT_MENU_IDS))
        {
            const size_t desktopIndex = static_cast<size_t>(iSelectedIndex - static_cast<int>(CONTEXT_MENU_IDS));
            if (desktopIndex < desktopNames.size())
                SwitchDesktopTo(desktopNames[desktopIndex]);
        }
    }

    void OnInitDialog(HWND hDlg)
    {
        g_hDesktopList = GetDlgItem(hDlg, IDC_DESKTOP_LIST);
        g_hDesktopName = GetDlgItem(hDlg, IDC_DESKTOP_NAME);
        g_hAddNewDesktop = GetDlgItem(hDlg, IDC_ADD_NEW_DESKTOP);
        g_hSwitchToDesktop = GetDlgItem(hDlg, IDC_SWITCH_TO_DESKTOP);

        PCWSTR pszAboutMenu = nullptr;
        int iAboutMenuLength = LoadStringW(g_hInstance, IDS_ABOUTBOX, reinterpret_cast<LPWSTR>(&pszAboutMenu), 0);
        if (iAboutMenuLength > 0)
        {
            std::wstring aboutMenu(pszAboutMenu, iAboutMenuLength);
            HMENU hSysMenu = GetSystemMenu(hDlg, FALSE);
            if (hSysMenu)
            {
                AppendMenuW(hSysMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hSysMenu, MF_STRING, IDM_ABOUTBOX, aboutMenu.c_str());
            }
        }

        HICON hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(IDR_MAINFRAME));
        SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
        SendMessageW(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));

        if (!AddTrayIcon(hDlg))
        {
            LogError(L"failed to add tray icon");
            MessageBoxW(hDlg, L"Failed to set tray icon.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            PostQuitMessage(-1);
            return;
        }

        CheckDlgButton(hDlg, IDC_VERIFY_CHECK, BST_CHECKED);

        LogInfo(std::format(L"initialized, {} desktop(s) available", DesktopManager::GetDesktopNames().size()));
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
            if (static_cast<int>(wParam & 0xFFF0) == IDM_ABOUTBOX)
            {
                ShowAboutBox();
                return TRUE;
            }
            break;

        case WM_CLOSE:
            ShowWindow(g_hDlg, SW_HIDE);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
            case IDCANCEL:
                ShowWindow(g_hDlg, SW_HIDE);
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
            }
            break;

        case WM_DESTROY:
            LogInfo(L"instance exiting");
            g_trayIcon.reset();
            g_hDlg = nullptr;
            PostQuitMessage(0);
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
