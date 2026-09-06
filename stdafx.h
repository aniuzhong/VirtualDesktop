#pragma once

#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#define NOMINMAX
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <tchar.h>

#include <algorithm>
#include <string>
#include <vector>

#include <wil/resource.h>
#include <wil/result.h>
#include <wil/registry.h>

#include "CommonDef.h"

#define ARRAY_SIZE 1024

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "Shlwapi.lib")
#pragma comment(lib, "Comctl32.lib")
