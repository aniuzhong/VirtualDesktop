#pragma once

#include <windows.h>

#include <string>

namespace desktops::seed
{
    // The arriving content of a new desktop: an inbox console for a shell that
    // does not exist there. Fire-and-forget on purpose — but the returned pid is
    // the caller's arrival proof: until a window of that pid exists on the
    // desktop, the desktop is still pinned by nothing but the caller's handle.
    // Returns the process id, or 0 on failure.
    DWORD powershell(const std::wstring& desktop);
}
