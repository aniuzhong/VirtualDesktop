#include "pch.h"
#include "ui_text.h"
#include "dialog.h"

namespace
{
    HWND g_hMainDlg = nullptr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    InitLogging();
    LogInfo(std::format(L"==== virtual desktop starting, pid {} ====", GetCurrentProcessId()));

    // Every instance holds the single-instance mutex for its entire lifetime —
    // that is what makes the check enforce anything at all. A hand-off
    // resident (--reside) sees "already exists" because the switcher still
    // holds the mutex during the hand-off; it keeps a duplicate handle
    // instead of exiting, so enforcement survives the switcher's retirement.
    const wchar_t singleInstanceMutex[] = L"Virtual_Desktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}";
    wil::unique_mutex_nothrow instanceMutex;
    bool alreadyExists = false;
    const bool residentHandoff = std::wstring_view(GetCommandLineW()).find(L"--reside") != std::wstring_view::npos;
    const bool created = instanceMutex.try_create(singleInstanceMutex, 0, MUTEX_ALL_ACCESS, nullptr, &alreadyExists);
    const DWORD mutexError = GetLastError();
    LogInfo(std::format(L"[startup] single-instance mutex: created={}, alreadyExists={}, hand-off={}",
        created, alreadyExists, residentHandoff));

    if (!created || (alreadyExists && !residentHandoff) || ERROR_ACCESS_DENIED == mutexError)
    {
        LogWarn(std::format(L"another instance is running (mutex error {}), exiting", mutexError));
        MessageBoxW(nullptr, L"One instance of this application is already running.", ui::kTitle, MB_OK);
        return 0;
    }

    INITCOMMONCONTROLSEX initCtrls = { sizeof(initCtrls), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&initCtrls);

    g_hMainDlg = CreateVirtualDesktopDialog(hInstance);
    if (!g_hMainDlg)
    {
        LogError(L"[startup] dialog creation failed");
        SwitchBackToDefault(L"dialog creation failed");
        return -1;
    }

    LogInfo(L"[startup] message loop running");
    MSG msg = { 0 };
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        if (!IsDialogMessageW(g_hMainDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return static_cast<int>(msg.wParam);
}
