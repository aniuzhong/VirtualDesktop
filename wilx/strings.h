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
//! wilx Strings: UTF-16 to UTF-8 conversion. wil ships no public
//! conversion API; this fills the gap for sinks that require narrow
//! UTF-8 (e.g. spdlog).
//!
//! House regime: TryGet* total fail-soft (see wilx/win32_helpers.h) --
//! an empty result means failure (invalid UTF-16 / OOM), never an
//! exception.
#ifndef __WILX_STRINGS_INCLUDED
#define __WILX_STRINGS_INCLUDED

#include <windows.h>
#include <string>
#include <string_view>

namespace wilx
{
//! Converts UTF-16 text to UTF-8. Empty result == failure. The buffer is
//! sized by the first WideCharToMultiByte call and shrunk to the number of
//! bytes actually written by the second, so a failing conversion yields an
//! empty string rather than a stale buffer.
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
