//*********************************************************
//
//    wilx - WIL-style extensions for Virtual Desktop.
//    Header-only, one theme per header, shaped after wil.
//    API contracts: wilx/README.md. Comments here only
//    explain choices the code cannot show.
//
//*********************************************************
//! @file
//! Window/control text queries.
#ifndef __WILX_WINDOWING_INCLUDED
#define __WILX_WINDOWING_INCLUDED

#include <windows.h>
#include <cstring>
#include <string>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
namespace wilx
{
//! GetWindowTextW does not distinguish "no text" from failure; neither does this.
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
