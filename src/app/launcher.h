#pragma once

#include <string>

namespace desktops::launcher
{
    // Resolves user input along the standard Run ladder (folder, URL, executable
    // path, PATH, App Paths, file association) and launches it on the named
    // desktop. Failures are reported on that desktop, never on Default.
    void launch(const std::wstring& desktop, const std::wstring& input);
}
