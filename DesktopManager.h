#pragma once

#include "stdafx.h"

namespace DesktopManager
{
    int GetDesktopCount(void);
    std::wstring GetDesktopName(int iIndex);
    std::wstring GetCurrentDesktopName(void);
    bool IsCurrentDesktop(const std::wstring& desktopName);
    bool SwitchDesktop(const std::wstring& desktopName);
    bool CreateDesktop(const std::wstring& desktopName);
    bool LaunchApplication(const std::wstring& applicationFilePath, const std::wstring& desktopName);
}
