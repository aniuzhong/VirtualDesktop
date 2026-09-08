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
//! wilx Win32 helpers: user-object name queries (desktops and window
//! stations) and Win32 error-message formatting. House regime of this
//! header is wil's win32_helpers style: PascalCase with Get/TryGet
//! failure markers.
//!
//! TryGet* = total fail-soft: an empty result means failure (insufficient
//! rights / no input desktop / OOM), never an exception. This is broader
//! than wil's TryGet family, which still surfaces unexpected failures
//! through an HRESULT return or a throw; here every failure is folded
//! into the empty result. An HRESULT nothrow core and Get* (throwing)
//! overloads stay reserved until a caller needs to distinguish or
//! propagate failures.
#ifndef __WILX_WIN32_HELPERS_INCLUDED
#define __WILX_WIN32_HELPERS_INCLUDED

#include <windows.h>
#include <cstring>
#include <string>

#include <wil/resource.h>

namespace wilx
{
//! UOI_NAME two-call size query; accepts HDESK and HWINSTA alike.
//! Empty result == failure.
[[nodiscard]] inline std::wstring TryGetUserObjectName(_In_ HANDLE userObject)
{
    DWORD bytesNeeded = 0;
    if (!GetUserObjectInformationW(userObject, UOI_NAME, nullptr, 0, &bytesNeeded) &&
        ERROR_INSUFFICIENT_BUFFER != GetLastError())
    {
        return {};
    }

    std::wstring name;
    name.resize_and_overwrite(bytesNeeded / sizeof(wchar_t) + 1, [&](wchar_t* buffer, size_t capacity) {
        if (!GetUserObjectInformationW(userObject, UOI_NAME, buffer,
            static_cast<DWORD>(capacity * sizeof(wchar_t)), &bytesNeeded))
        {
            return size_t{};
        }
        return std::wcslen(buffer);
    });
    return name;
}

//! GetThreadDesktop handle is owned by the thread: never CloseDesktop it.
[[nodiscard]] inline std::wstring TryGetThreadDesktopName()
{
    return TryGetUserObjectName(GetThreadDesktop(GetCurrentThreadId()));
}

[[nodiscard]] inline std::wstring TryGetInputDesktopName()
{
    wil::unique_hdesk inputDesktop(OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS));
    if (!inputDesktop)
    {
        return {};
    }
    return TryGetUserObjectName(inputDesktop.get());
}

[[nodiscard]] inline std::wstring TryGetProcessWindowStationName()
{
    return TryGetUserObjectName(GetProcessWindowStation());
}

//! Formats a Win32 error code via FormatMessageW. Empty result == failure
//! (no message for the code / OOM). Trailing CR/LF is trimmed.
[[nodiscard]] inline std::wstring TryGetWin32ErrorMessage(DWORD error)
{
    wil::unique_hlocal buffer;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, error, 0, reinterpret_cast<LPWSTR>(buffer.put()), 0, nullptr);
    if (!buffer)
    {
        return {};
    }

    std::wstring message(static_cast<PWSTR>(buffer.get()));
    while (!message.empty() && (message.back() == L'\n' || message.back() == L'\r'))
    {
        message.pop_back();
    }
    return message;
}

//! Convenience overload reading the calling thread's last error code; call
//! it immediately after the failing API, before anything clobbers it.
[[nodiscard]] inline std::wstring TryGetWin32ErrorMessage()
{
    return TryGetWin32ErrorMessage(GetLastError());
}
} // namespace wilx
#endif // __WILX_WIN32_HELPERS_INCLUDED
