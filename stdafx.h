#pragma once

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

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

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <wil/resource.h>
#include <wil/result.h>
#include <wil/registry.h>
#include <wil/stl.h>
#include <wil/win32_helpers.h>

#include "CommonDef.h"

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Comctl32.lib")
