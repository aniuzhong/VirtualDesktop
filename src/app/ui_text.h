#pragma once

// User-visible strings the C++ side shows or dispatches on. The .rc owns
// dialog layout and its static text; these are wchar_t arrays so they decay
// straight to the PCWSTR the Win32 APIs take.
#include <string_view>

namespace ui
{
    inline constexpr wchar_t kTitle[] = L"Virtual Desktop";
    inline constexpr wchar_t kConfirmSwitchMenuItem[] = L"&Confirm \"Desktop Switch\"";
    inline constexpr wchar_t kManageDesktopsMenuItem[] = L"&Manage Desktops";
    inline constexpr wchar_t kAboutMenuItem[] = L"&About Virtual Desktop...";
    inline constexpr wchar_t kExitMenuItem[] = L"&Exit";
}
