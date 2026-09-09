#pragma once

#include <windows.h>

#include <string_view>

namespace desktops::log
{
    // Call once at startup; on any failure the app keeps running with
    // spdlog's default logger. Enables the debug level.
    void init();

    void debug(std::wstring_view message);
    void info(std::wstring_view message);
    void warn(std::wstring_view message);
    void error(std::wstring_view message);

    // Appends " (error N: message)" for a non-zero GetLastError value; falls back to error(message) for zero.
    void error(std::wstring_view message, DWORD lastError);
}
