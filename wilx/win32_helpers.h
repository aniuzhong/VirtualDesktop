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
//! TryGet* = total fail-soft: an empty result means failure (insufficient
//! rights / no input desktop / OOM), never an exception. This is broader than
//! wil's TryGet family, which still surfaces unexpected failures through an
//! HRESULT return or a throw; here every failure is folded into the empty
//! result. An HRESULT nothrow core and Get* (throwing) overloads stay
//! reserved until a caller needs to distinguish or propagate failures.
#ifndef __WILX_WIN32_HELPERS_INCLUDED
#define __WILX_WIN32_HELPERS_INCLUDED

#include <windows.h>
#include <string>

#include <wil/resource.h>

namespace wilx
{
//! UOI_NAME two-call size query; accepts HDESK and HWINSTA alike.
//! Empty result == failure.
template <typename string_type = std::wstring>
string_type TryGetUserObjectName(_In_ HANDLE userObject)
{
    DWORD bytesNeeded = 0;
    if (!GetUserObjectInformationW(userObject, UOI_NAME, nullptr, 0, &bytesNeeded) &&
        ERROR_INSUFFICIENT_BUFFER != GetLastError())
    {
        return {};
    }

    string_type name(bytesNeeded / sizeof(wchar_t) + 1, L'\0');
    if (!GetUserObjectInformationW(userObject, UOI_NAME, name.data(), bytesNeeded, &bytesNeeded))
    {
        return {};
    }

    name.resize(wcslen(name.c_str()));
    return name;
}

//! GetThreadDesktop handle is owned by the thread: never CloseDesktop it.
template <typename string_type = std::wstring>
string_type TryGetThreadDesktopName()
{
    return TryGetUserObjectName<string_type>(GetThreadDesktop(GetCurrentThreadId()));
}

template <typename string_type = std::wstring>
string_type TryGetInputDesktopName()
{
    wil::unique_hdesk inputDesktop(OpenInputDesktop(0, FALSE, DESKTOP_READOBJECTS));
    if (!inputDesktop)
    {
        return {};
    }
    return TryGetUserObjectName<string_type>(inputDesktop.get());
}

template <typename string_type = std::wstring>
string_type TryGetProcessWindowStationName()
{
    return TryGetUserObjectName<string_type>(GetProcessWindowStation());
}
} // namespace wilx
#endif // __WILX_WIN32_HELPERS_INCLUDED
