#pragma once

#include <windows.h>

#include <string>

namespace desktops::satellite
{
    // Posted to the panel after a satellite moved input back to Default.
    inline constexpr UINT WM_APP_HOME_RETURN = WM_APP + 1;
    // Posted to the panel when a satellite could not attach; lParam is a heap
    // std::wstring* with the desktop name, deleted by the receiver.
    inline constexpr UINT WM_APP_SPAWN_FAILED = WM_APP + 2;
    // Sent (PostThreadMessage) to a satellite: bring the Run dialog to the front.
    inline constexpr UINT WM_APP_ACTIVATE = WM_APP + 3;

    void set_module(HINSTANCE instance);

    // Starts the resident Run dialog thread on the named desktop. The process
    // stays single-instance; a satellite is one of its threads attached to the
    // desktop via SetThreadDesktop.
    void spawn(const std::wstring& desktop, HWND panel);

    // Signals the satellite on that desktop to exit. No desktop switching — the
    // caller owns the input-desktop policy.
    void tear_down(const std::wstring& desktop);

    // Asks the satellite on that desktop to bring itself to the front (used right
    // after the panel switches input onto the desktop).
    void activate(const std::wstring& desktop);

    // True if a window of this process already exists on that desktop.
    bool is_present(const std::wstring& desktop);
}
