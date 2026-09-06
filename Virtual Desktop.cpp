#include "stdafx.h"
#include "Virtual DesktopDlg.h"

namespace
{
    HWND g_hMainDlg = nullptr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    Sleep(1000);
    wil::unique_handle hMutex(CreateMutexW(nullptr, FALSE, L"Virtual_Desktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}"));
    DWORD mutexError = GetLastError();
    if (!hMutex || ERROR_ALREADY_EXISTS == mutexError || ERROR_ACCESS_DENIED == mutexError)
    {
        MessageBoxW(nullptr, L"One instance of this aplication is already running.", TXT_MESSAGEBOX_TITLE, MB_OK);
        return 0;
    }

    INITCOMMONCONTROLSEX initCtrls = { sizeof(initCtrls), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&initCtrls);

    g_hMainDlg = CreateVirtualDesktopDialog(hInstance);
    if (!g_hMainDlg)
        return -1;

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
