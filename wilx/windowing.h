//*********************************************************
//
//    wilx - WIL-style extensions for Virtual Desktop.
//    One theme per header, header-only, mirroring wil's
//    organization: details trampolines + thin public APIs.
//    House rules: build on wil's public API only (wil::details
//    is reference material, never a dependency); C++23 and up,
//    no historical back-compat layers.
//
//*********************************************************
//! @file
//! wilx Windowing: window/control text queries, complementing wil's
//! for_each_window family in wil/windowing.h.
//!
//! House regime: TryGet* total fail-soft (see wilx/win32_helpers.h).
//! GetWindowTextW does not distinguish "no text" from "failure" -- both
//! yield zero -- so neither does TryGetWindowText: an empty result means
//! either.
#ifndef __WILX_WINDOWING_INCLUDED
#define __WILX_WINDOWING_INCLUDED

#include <windows.h>
#include <cstring>
#include <string>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
namespace wilx
{
//! Two-call GetWindowTextLength/GetWindowTextW query. Empty result ==
//! failure or empty text.
[[nodiscard]] inline std::wstring TryGetWindowText(_In_ HWND window)
{
    const int length = GetWindowTextLengthW(window);
    if (length <= 0)
    {
        return {};
    }

    std::wstring text;
    text.resize_and_overwrite(static_cast<size_t>(length) + 1, [window](wchar_t* buffer, size_t capacity) {
        GetWindowTextW(window, buffer, static_cast<int>(capacity));
        return std::wcslen(buffer);
    });
    return text;
}
} // namespace wilx
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#endif // __WILX_WINDOWING_INCLUDED
