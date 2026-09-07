//*********************************************************
//
//    wilx - WIL-style extensions for Virtual Desktop.
//    One theme per header, header-only, mirroring wil's
//    organization: details trampolines + thin public APIs.
//
//*********************************************************
//! @file
//! wilx Win32 helpers: user-object name queries (desktops and window
//! stations). House regime of this header is wil's win32_helpers style:
//! PascalCase with Get/TryGet failure markers.
//!
//! TryGet* = fail-soft: an empty result means failure (insufficient rights /
//! no input desktop / OOM). Expected failures are returned as data, never
//! thrown. Get* (throwing) and HRESULT nothrow overloads are reserved until a
//! caller needs them.
#ifndef __WILX_WIN32_HELPERS_INCLUDED
#define __WILX_WIN32_HELPERS_INCLUDED

#include <windows.h>
#include <string>

#include <wil/resource.h>

namespace wilx
{
//! UOI_NAME two-call size query; accepts HDESK and HWINSTA alike.
//! Empty result == failure.
inline std::wstring TryGetUserObjectName(_In_ HANDLE userObject)
{
    DWORD bytesNeeded = 0;
    if (!GetUserObjectInformationW(userObject, UOI_NAME, nullptr, 0, &bytesNeeded) &&
        ERROR_INSUFFICIENT_BUFFER != GetLastError())
    {
        return {};
    }

    std::wstring name(bytesNeeded / sizeof(wchar_t) + 1, L'\0');
    if (!GetUserObjectInformationW(userObject, UOI_NAME, name.data(), bytesNeeded, &bytesNeeded))
    {
        return {};
    }

    name.resize(wcslen(name.c_str()));
    return name;
}

//! GetThreadDesktop handle is owned by the thread: never CloseDesktop it.
inline std::wstring TryGetThreadDesktopName()
{
    return TryGetUserObjectName(GetThreadDesktop(GetCurrentThreadId()));
}

inline std::wstring TryGetInputDesktopName()
{
    wil::unique_hdesk inputDesktop(OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS));
    if (!inputDesktop)
    {
        return {};
    }
    return TryGetUserObjectName(inputDesktop.get());
}

inline std::wstring TryGetProcessWindowStationName()
{
    return TryGetUserObjectName(GetProcessWindowStation());
}
} // namespace wilx
#endif // __WILX_WIN32_HELPERS_INCLUDED
