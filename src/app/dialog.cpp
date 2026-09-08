#include "stdafx.h"
#include "wilx/desktops.h"
#include "wilx/win32_helpers.h"
#include "resource.h"
#include "dialog.h"

void SwitchBackToDefault(const wchar_t* reason);

static const wchar_t RESIDENT_READY_EVENT[] = L"VirtualDesktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}-resident-ready";

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

    // Set while a deliberate desktop switch is tearing this instance down, so
    // WM_DESTROY does not switch the input desktop back to Default — the
    // desktop being switched TO has its own resident instance.
    bool g_switchingDesktop = false;

    bool IsVerifyChecked(void)
    {
        return IsDlgButtonChecked(g_hDlg, IDC_VERIFY_CHECK) == BST_CHECKED;
    }

    std::vector<std::wstring> GetDesktopNames(void)
    {
        HWINSTA hWindowsStation = GetProcessWindowStation();
        if (NULL == hWindowsStation)
        {
            LogError(L"GetProcessWindowStation failed", GetLastError());
            return {};
        }

        std::vector<std::wstring> desktopNames;
        wilx::for_each_desktop_nothrow(hWindowsStation, [&](PCWSTR lpszDesktopName) {
            desktopNames.emplace_back(lpszDesktopName);
        });
        return desktopNames;
    }

    bool IsCurrentDesktop(const std::wstring& desktopName)
    {
        std::wstring currentName = wilx::TryGetThreadDesktopName();
        if (currentName.empty())
            return false;
        return _wcsicmp(desktopName.c_str(), currentName.c_str()) == 0;
    }

    void LogInputDesktop(const wchar_t* context)
    {
        const std::wstring name = wilx::TryGetInputDesktopName();
        if (name.empty())
            LogError(std::format(L"{}: input desktop name unavailable", context), 0);
        else
            LogInfo(std::format(L"{}: input desktop '{}'", context, name));
    }

    constexpr std::wstring_view ExecutableExtensions[] = { L".exe", L".com", L".pif", L".scr" };

    bool LaunchApplication(const std::wstring& applicationFilePath, const std::wstring& desktopName,
        const std::wstring& arguments = L"", _Out_opt_ DWORD* processId = nullptr)
    {
        if (processId)
            *processId = 0;

        std::wstring extension = std::filesystem::path(applicationFilePath).extension();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
        if (std::find(std::begin(ExecutableExtensions), std::end(ExecutableExtensions), extension) == std::end(ExecutableExtensions))
        {
            LogError(L"[launch] invalid file extension", 0);
            return false;
        }

        std::wstring directoryName = std::filesystem::path(applicationFilePath).parent_path().wstring();

        LogInfo(std::format(L"[launch] '{}' on desktop '{}'", applicationFilePath, desktopName));

        wil::unique_process_information processInfo;
        STARTUPINFOW sInfo = { 0 };
        sInfo.cb = sizeof(sInfo);
        sInfo.lpDesktop = const_cast<LPWSTR>(desktopName.c_str());

        std::wstring commandLine = L"\"" + applicationFilePath + L"\"";
        if (!arguments.empty())
            commandLine += L" " + arguments;

        if (!CreateProcessW(applicationFilePath.c_str(), commandLine.data(), NULL, NULL, TRUE,
                NORMAL_PRIORITY_CLASS, NULL, directoryName.c_str(), &sInfo, &processInfo))
        {
            LogError(L"[launch] CreateProcessW failed", GetLastError());
            return false;
        }

        LogInfo(std::format(L"[launch] pid {}", processInfo.dwProcessId));
        if (processId)
            *processId = processInfo.dwProcessId;
        return true;
    }

    bool CreateDesktop(const std::wstring& desktopName)
    {
        if (desktopName.empty())
            return false;

        // Snapshot before CreateDesktopW: afterwards the snapshot would contain
        // the new desktop itself and read as a duplicate, skipping the Explorer
        // launch — leaving an unpinned desktop that dies with our handle.
        const std::vector<std::wstring> existingNames = GetDesktopNames();
        const bool alreadyExists = std::any_of(existingNames.begin(), existingNames.end(),
            [&desktopName](const std::wstring& existing) { return _wcsicmp(existing.c_str(), desktopName.c_str()) == 0; });
        LogInfo(std::format(L"[create] '{}' duplicate check: {}, {} desktop(s) enumerated",
            desktopName, alreadyExists ? L"pre-existing" : L"new", existingNames.size()));

        SECURITY_ATTRIBUTES sAttribute = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
        wil::unique_hdesk hNewDesktop(::CreateDesktopW(desktopName.c_str(), NULL, NULL, DF_ALLOWOTHERACCOUNTHOOK, GENERIC_ALL, &sAttribute));
        if (!hNewDesktop)
        {
            DWORD createError = GetLastError();
            LogError(std::format(L"[create] CreateDesktopW('{}') failed", desktopName), createError);
            MessageBoxW(NULL, wilx::TryGetWin32ErrorMessage(createError).c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
            return false;
        }
        LogInfo(std::format(L"[create] CreateDesktopW('{}') succeeded", desktopName));

        // A pre-existing desktop already has its own shell; a new one only
        // becomes usable once Explorer runs on it and pins it against
        // destruction — an unseeded desktop renders black.
        bool explorerSeeded = alreadyExists;
        if (!explorerSeeded)
        {
            std::wstring windowsDirectory;
            if (FAILED(wil::GetWindowsDirectoryW(windowsDirectory)))
            {
                LogError(L"[create] GetWindowsDirectoryW failed", GetLastError());
            }
            else
            {
                DWORD explorerPid = 0;
                if (LaunchApplication(windowsDirectory + L"\\Explorer.Exe", desktopName, L"", &explorerPid) && explorerPid != 0)
                {
                    wil::unique_handle explorer(OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, explorerPid));
                    if (!explorer)
                    {
                        LogError(L"[create] OpenProcess(Explorer) failed, seeding state unknown", GetLastError());
                        explorerSeeded = true;
                    }
                    else
                    {
                        // Explorer delegating to an existing shell exits right away,
                        // which would leave this desktop black and unpinned.
                        if (WaitForSingleObject(explorer.get(), 2000) == WAIT_OBJECT_0)
                        {
                            DWORD exitCode = 0;
                            GetExitCodeProcess(explorer.get(), &exitCode);
                            LogError(std::format(L"[create] Explorer exited within 2s (code {}), desktop would render black", exitCode), 0);
                        }
                        else
                        {
                            LogInfo(L"[create] Explorer alive 2s after launch, desktop pinned");
                            explorerSeeded = true;
                        }
                    }
                }
            }
        }

        if (!explorerSeeded)
        {
            // Closing our handle destroys an unpinned desktop: never hand the
            // user a black-screen desktop.
            LogError(std::format(L"[create] desktop '{}' discarded (Explorer not seeded)", desktopName), 0);
            MessageBoxW(NULL,
                std::format(L"Could not start Explorer on desktop '{}'. The desktop was not created.", desktopName).c_str(),
                TXT_MESSAGEBOX_TITLE, MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
            return false;
        }

        LogInfo(std::format(L"[create] desktop '{}' ready", desktopName));
        return true;
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

        LogInfo(std::format(L"[switch] '{}'", desktopName));
        LogInputDesktop(L"[switch] before");

        SetForegroundWindow(g_hDlg);

        if (IsCurrentDesktop(desktopName))
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

        wil::unique_hdesk hDesktopToSwitch(OpenDesktopW(desktopName.c_str(), DF_ALLOWOTHERACCOUNTHOOK, TRUE, GENERIC_ALL));
        if (!hDesktopToSwitch)
        {
            DWORD openError = GetLastError();
            if (ERROR_ACCESS_DENIED == openError)
            {
                std::wstring errorMsg = std::format(L"Failed to switch to {} desktop.\n\t {}",
                    desktopName, wilx::TryGetWin32ErrorMessage(openError));
                MessageBoxW(NULL, errorMsg.c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            }
            LogError(std::format(L"[switch] OpenDesktopW('{}') failed", desktopName), openError);
            return;
        }

        if (!::SwitchDesktop(hDesktopToSwitch.get()))
        {
            LogError(L"[switch] SwitchDesktop failed", GetLastError());
            return;
        }

        LogInputDesktop(L"[switch] after");

        // Launch the resident and verify it settles in: only once the resident
        // reports a usable desktop (shell surface present) does this instance
        // retire. Otherwise undo the switch — on a dead desktop input is gone,
        // and this instance is the last resort able to undo the switch.
        HANDLE readyEvent = CreateEventW(nullptr, TRUE, FALSE, RESIDENT_READY_EVENT);
        if (!readyEvent)
        {
            LogError(L"[switch] CreateEventW(resident-ready) failed", GetLastError());
            SwitchBackToDefault(L"resident readiness event unavailable");
            return;
        }
        ResetEvent(readyEvent);

        DWORD selfPid = 0;
        if (!LaunchApplication(wil::GetModuleFileNameW<std::wstring>(nullptr), desktopName, L"--reside", &selfPid))
        {
            CloseHandle(readyEvent);
            SwitchBackToDefault(L"self relaunch failed");
            return;
        }
        LogInfo(std::format(L"[switch] self relaunched (pid {}), waiting for it to settle", selfPid));

        bool residentHealthy = false;
        const DWORD waitStart = GetTickCount();
        while (GetTickCount() - waitStart < 15000)
        {
            const DWORD waitResult = MsgWaitForMultipleObjects(1, &readyEvent, FALSE, 1000, QS_ALLINPUT);
            if (waitResult == WAIT_OBJECT_0)
            {
                residentHealthy = true;
                break;
            }
            if (waitResult == WAIT_OBJECT_0 + 1)
            {
                MSG msg;
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
            // WAIT_TIMEOUT: keep waiting until the budget expires.
        }
        CloseHandle(readyEvent);

        if (!residentHealthy)
        {
            // Undo the switch no matter what the resident is doing, then reap
            // it if it never came up. This instance stays alive as the Default
            // desktop's resident.
            SwitchBackToDefault(L"resident did not confirm a healthy desktop");
            wil::unique_handle resident(OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, selfPid));
            if (resident)
            {
                if (WaitForSingleObject(resident.get(), 3000) != WAIT_OBJECT_0)
                {
                    TerminateProcess(resident.get(), 1);
                    LogWarn(L"[switch] resident terminated after failed hand-off");
                }
            }
            return;
        }

        LogInfo(std::format(L"[switch] resident healthy (pid {}), this instance exits", selfPid));
        g_switchingDesktop = true;
        DestroyWindow(g_hDlg);
    }

    void ShowManageDesktopsDialog(void)
    {
        const std::vector<std::wstring> desktopNames = GetDesktopNames();
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

            if (CreateDesktop(name))
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

        const std::vector<std::wstring> desktopNames = GetDesktopNames();

        wil::unique_hmenu hContextMenu(CreatePopupMenu());

        for (size_t iMenuItem = 0; iMenuItem < desktopNames.size(); iMenuItem++)
        {
            UINT flags = MF_STRING | MF_ENABLED | (IsCurrentDesktop(desktopNames[iMenuItem]) ? MF_CHECKED : 0);
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
            LogError(L"[startup] failed to add tray icon");
            MessageBoxW(hDlg, L"Failed to set tray icon.", TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            SwitchBackToDefault(L"tray icon failed");
            PostQuitMessage(-1);
            return;
        }
        LogInfo(L"[startup] tray icon added");

        // A created desktop only becomes usable once its shell surface (the
        // wallpaper host) exists; without it the desktop renders black and
        // input is dead. Probe bounded and bail out to Default if absent.
        bool shellFound = false;
        for (int probe = 0; probe < 20 && !shellFound; ++probe)
        {
            if (FindWindowW(L"Progman", nullptr) || FindWindowW(L"WorkerW", nullptr))
                shellFound = true;
            else
                Sleep(500);
        }
        LogInfo(std::format(L"[startup] shell surface probe: {}", shellFound ? L"found" : L"absent after 10s"));
        if (!shellFound)
        {
            SwitchBackToDefault(L"no shell surface on target desktop");
            g_trayIcon.reset();
            PostQuitMessage(-1);
            return;
        }

        if (HANDLE readyEvent = OpenEventW(EVENT_MODIFY_STATE, FALSE, RESIDENT_READY_EVENT))
        {
            SetEvent(readyEvent);
            CloseHandle(readyEvent);
            LogInfo(L"[startup] resident readiness signaled");
        }

        CheckDlgButton(hDlg, IDC_VERIFY_CHECK, BST_CHECKED);

        LogInfo(std::format(L"initialized, {} desktop(s) available", GetDesktopNames().size()));
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
            LogInfo(L"[exit] instance exiting");
            g_trayIcon.reset();
            if (!g_switchingDesktop)
            {
                const std::wstring inputDesktop = wilx::TryGetInputDesktopName();
                if (!inputDesktop.empty() && _wcsicmp(inputDesktop.c_str(), L"Default") != 0)
                    SwitchBackToDefault(L"instance exiting");
            }
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

void SwitchBackToDefault(const wchar_t* reason)
{
    LogInfo(std::format(L"[bailout] ({}): switching input desktop back to Default", reason));
    wil::unique_hdesk hDefault(OpenDesktopW(L"Default", 0, FALSE, DESKTOP_SWITCHDESKTOP));
    if (!hDefault)
    {
        LogError(L"[bailout] OpenDesktopW(Default) failed", GetLastError());
        return;
    }
    if (!::SwitchDesktop(hDefault.get()))
        LogError(L"[bailout] SwitchDesktop(Default) failed", GetLastError());
    else
        LogInfo(L"[bailout] input desktop is now Default");
}
