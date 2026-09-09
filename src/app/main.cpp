#include <format>

#include <wil/resource.h>

#include "logging.h"
#include "panel.h"
#include "satellite.h"
#include "wilx/win32_helpers.h"

namespace
{
    constexpr wchar_t kMutexName[] =
        L"Local\\VirtualDesktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}";
    constexpr wchar_t kGoHomeEvent[] =
        L"Local\\VirtualDesktop_{44D28BCA-7F46-4af2-A1FF-36EE0DAC7CD2}-go-home";
}

int APIENTRY wWinMain(_In_ HINSTANCE instance, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
    desktops::log::init();
    desktops::log::info(std::format(L"==== starting, pid {} ====", GetCurrentProcessId()));

    // Session-scoped single instance. A second launch — from any desktop — is the
    // go-home recall: signal the resident panel and exit.
    wil::unique_mutex_nothrow instanceMutex;
    bool alreadyExists = false;
    if (!instanceMutex.try_create(kMutexName, 0, MUTEX_ALL_ACCESS, nullptr, &alreadyExists))
    {
        desktops::log::warn(std::format(L"mutex unavailable (error {}), exiting", GetLastError()));
        return 0;
    }
    if (alreadyExists)
    {
        desktops::log::info(L"instance already running; signaling go-home");
        wil::unique_handle goHome(OpenEventW(EVENT_MODIFY_STATE, FALSE, kGoHomeEvent));
        if (goHome)
            SetEvent(goHome.get());
        return 0;
    }

    // The panel lives on Default only. Launching from elsewhere is a refusal,
    // never a relocation — the user sees the box on the desktop they are on.
    const std::wstring threadDesktop = wilx::TryGetThreadDesktopName();
    if (_wcsicmp(threadDesktop.c_str(), L"Default") != 0)
    {
        desktops::log::warn(std::format(L"launched on '{}', refusing", threadDesktop));
        MessageBoxW(nullptr,
            L"Virtual Desktop runs on the Default desktop.\nSwitch back to Default and start it there.",
            L"Virtual Desktop", MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
        return 0;
    }

    wil::unique_handle goHome(CreateEventW(nullptr, FALSE, FALSE, kGoHomeEvent));
    if (!goHome)
    {
        desktops::log::error(L"[startup] go-home event creation failed", GetLastError());
        return -1;
    }

    desktops::satellite::set_module(instance);
    const int exitCode = desktops::panel::run(instance, goHome.get());
    desktops::log::info(std::format(L"==== exiting with {} ====", exitCode));
    return exitCode;
}
