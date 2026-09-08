//*********************************************************
//
//    wilx - WIL-style extensions for Virtual Desktop.
//    Header-only, one theme per header, shaped after wil.
//    API contracts: wilx/README.md. Comments here only
//    explain choices the code cannot show.
//
//*********************************************************
//! @file
//! Tray icon RAII on wil::unique_struct.
#ifndef __WILX_SHELL_INCLUDED
#define __WILX_SHELL_INCLUDED

#include <windows.h>
#include <shellapi.h>

#include <wil/resource.h>

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
namespace wilx
{
namespace details
{
    inline void __stdcall DeleteNotifyIcon(_In_ NOTIFYICONDATAW* data) WI_NOEXCEPT
    {
        Shell_NotifyIconW(NIM_DELETE, data);
    }
} // namespace details

//! Always-clear: NIM_DELETE on a never-added icon fails harmlessly
//! (wil's unique_prop_variant semantics).
using unique_notify_icon_data =
    wil::unique_struct<NOTIFYICONDATAW, decltype(&details::DeleteNotifyIcon), details::DeleteNotifyIcon>;
} // namespace wilx
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#endif // __WILX_SHELL_INCLUDED
