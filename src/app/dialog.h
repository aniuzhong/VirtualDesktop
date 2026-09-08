#pragma once

#include "stdafx.h"

HWND CreateVirtualDesktopDialog(HINSTANCE hInstance);
void SwitchBackToDefault(const wchar_t* reason);
