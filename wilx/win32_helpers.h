//*********************************************************
//
//    wilx - WIL-style extensions for Virtual Desktop.
//    Header-only, one theme per header, shaped after wil.
//    API contracts: wilx/README.md. Comments here only
//    explain choices the code cannot show.
//
//*********************************************************
//! @file
//! User-object name queries and Win32 error messages. TryGet* here is
//! total fail-soft (see wilx/README.md).
#ifndef __WILX_WIN32_HELPERS_INCLUDED
#define __WILX_WIN32_HELPERS_INCLUDED

#include <windows.h>
#include <cstring>
#include <string>

#include <wil/resource.h>

namespace wilx
{
//! Accepts HDESK and HWINSTA alike.
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

//! Reads the calling thread's last error; call immediately after the failing
//! API, before anything clobbers it.
[[nodiscard]] inline std::wstring TryGetWin32ErrorMessage()
{
    return TryGetWin32ErrorMessage(GetLastError());
}
} // namespace wilx
#endif // __WILX_WIN32_HELPERS_INCLUDED
