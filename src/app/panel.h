#pragma once

#include <windows.h>

namespace desktops::panel
{
    // Creates the Default-desktop panel and pumps until it closes. `goHome` is the
    // auto-reset event a second instance signals to recall the user from wherever
    // they are.
    int run(HINSTANCE instance, HANDLE goHome);
}
