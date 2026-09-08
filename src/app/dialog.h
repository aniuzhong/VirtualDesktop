#pragma once

#include "pch.h"

HWND CreateVirtualDesktopDialog(HINSTANCE hInstance);
void SwitchBackToDefault(const wchar_t* reason);
