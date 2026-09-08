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
    // Take the single-instance mutex. A fresh desktop's instance can start
    // while the previous instance is still unwinding, so an attempt that finds
    // the mutex held is followed by a bounded wait for it to be released —
    // treating that hand-off window as "already running" would strand the new
    // desktop without a resident instance.
    const wchar_t singleInstanceMutex[] = L"Virtual_Desktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}";
    wil::unique_mutex_nothrow instanceMutex;
    bool acquired = false;
    bool alreadyExists = false;
    DWORD mutexError = ERROR_SUCCESS;
    for (int attempt = 0; attempt < 12 && !acquired; ++attempt)
    {
        if (instanceMutex.try_create(singleInstanceMutex, 0, MUTEX_ALL_ACCESS, nullptr, &alreadyExists) && !alreadyExists)
        {
            acquired = true;
            break;
        }

        mutexError = GetLastError();
        LogWarn(std::format(L"[startup] single-instance mutex held (error {}), waiting for release", mutexError));
        wil::unique_handle holder(OpenMutexW(SYNCHRONIZE, FALSE, singleInstanceMutex));
        if (!holder || WaitForSingleObject(holder.get(), 500) != WAIT_OBJECT_0)
            break;  // the holder is alive (or unopenable): genuine second instance
    }

    if (!acquired)
    {
        LogWarn(std::format(L"another instance is running (mutex error {}), exiting", mutexError));
        MessageBoxW(nullptr, L"One instance of this application is already running.", TXT_MESSAGEBOX_TITLE, MB_OK);
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
