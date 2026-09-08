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
//! wilx Shell: Shell_NotifyIconW tray icon RAII, built directly on wil's
//! unique_struct (zero-initialized by default, close function called on
//! destruction) -- the same shape wil uses for unique_prop_variant.
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

//! Zero-initialized NOTIFYICONDATAW that issues NIM_DELETE on destruction.
//! Fill the fields (including cbSize), NIM_ADD it, and keep the object
//! alive for as long as the icon should exist. Deleting an icon that was
//! never added is a harmless failed call, matching the always-clear
//! semantics of wil's unique_prop_variant.
using unique_notify_icon_data =
    wil::unique_struct<NOTIFYICONDATAW, decltype(&details::DeleteNotifyIcon), details::DeleteNotifyIcon>;
} // namespace wilx
#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#endif // __WILX_SHELL_INCLUDED
