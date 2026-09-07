#include "stdafx.h"
#include "Virtual DesktopDlg.h"
#include <shlobj.h>

namespace
{
    HWND g_hMainDlg = nullptr;

    void InitLogging(void)
    {
        try
        {
            wil::unique_cotaskmem_string appData;
            if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, appData.put())))
                return;

            std::filesystem::path logDirectory = std::filesystem::path(appData.get()) / L"VirtualDesktop" / L"logs";
            std::filesystem::create_directories(logDirectory);

            auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                ToUtf8((logDirectory / L"virtualdesktop.log").wstring()), 1024 * 1024, 1);
            auto logger = std::make_shared<spdlog::logger>("virtualdesktop", std::move(fileSink));
            logger->set_pattern("[%Y-%m-%d %T.%e] [P%P T%t] [%l] %v");
            logger->flush_on(spdlog::level::info);
            spdlog::set_default_logger(std::move(logger));
        }
        catch (const std::exception&)
        {
            // Logging is a debug aid only; keep running with the default (console) logger.
        }
    }
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    InitLogging();
    spdlog::info("==== virtual desktop starting, pid {} ====", GetCurrentProcessId());

    Sleep(1000);
    wil::unique_handle hMutex(CreateMutexW(nullptr, FALSE, L"Virtual_Desktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}"));
    DWORD mutexError = GetLastError();
    if (!hMutex || ERROR_ALREADY_EXISTS == mutexError || ERROR_ACCESS_DENIED == mutexError)
    {
        spdlog::warn("another instance is running (mutex error {}), exiting", mutexError);
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
