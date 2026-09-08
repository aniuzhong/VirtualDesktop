#include "stdafx.h"
#include "dialog.h"

namespace
{
    HWND g_hMainDlg = nullptr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    InitLogging();
    LogInfo(std::format(L"==== virtual desktop starting, pid {} ====", GetCurrentProcessId()));

    Sleep(1000);

    // A hand-off from a switching instance (--reside) bypasses the mutex: the
    // switcher serializes hand-offs by design and stays alive until this
    // resident reports a healthy desktop. Plain launches keep the check.
    const bool residentHandoff = std::wstring_view(GetCommandLineW()).find(L"--reside") != std::wstring_view::npos;
    if (residentHandoff)
    {
        LogInfo(L"[startup] resident hand-off, single-instance check skipped");
    }
    else
    {
        const wchar_t singleInstanceMutex[] = L"Virtual_Desktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}";
        wil::unique_mutex_nothrow instanceMutex;
        bool alreadyExists = false;
        if (!(instanceMutex.try_create(singleInstanceMutex, 0, MUTEX_ALL_ACCESS, nullptr, &alreadyExists) && !alreadyExists))
        {
            LogWarn(std::format(L"another instance is running (mutex error {}), exiting", GetLastError()));
            MessageBoxW(nullptr, L"One instance of this application is already running.", TXT_MESSAGEBOX_TITLE, MB_OK);
            return 0;
        }
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
