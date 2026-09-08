//*********************************************************
//
//    wilx - WIL-style extensions for Virtual Desktop.
//    Header-only, one theme per header, shaped after wil.
//    API contracts: wilx/README.md. Comments here only
//    explain choices the code cannot show.
//
//*********************************************************
//! @file
//! UTF-16 to UTF-8 conversion.
#ifndef __WILX_STRINGS_INCLUDED
#define __WILX_STRINGS_INCLUDED

#include <windows.h>
#include <string>
#include <string_view>

namespace wilx
{
//! Shrinks to the bytes actually written: a failed conversion yields an
//! empty string, never a stale buffer.
[[nodiscard]] inline std::string TryGetUtf8String(std::wstring_view text)
{
    if (text.empty())
    {
        return {};
    }

    const int byteCount = WideCharToMultiByte(
        CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (byteCount <= 0)
    {
        return {};
    }

    std::string utf8;
    utf8.resize_and_overwrite(static_cast<size_t>(byteCount), [&text](char* buffer, size_t capacity) {
        return static_cast<size_t>(WideCharToMultiByte(CP_UTF8, 0, text.data(),
            static_cast<int>(text.size()), buffer, static_cast<int>(capacity), nullptr, nullptr));
    });
    return utf8;
}
} // namespace wilx
#endif // __WILX_STRINGS_INCLUDED
