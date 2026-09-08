#pragma once

// Windows 10 and up.
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00

#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <wil/resource.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include "logging.h"

// Common Controls v6 activation and DPI awareness live in app.manifest,
// embedded via the .rc; Comctl32 is linked by CMake.
