#pragma once

#include "stdafx.h"

class CDesktopManager
{
public:
    static int GetDesktopCount(void);
    static std::wstring GetDesktopName(int iIndex);
    static std::wstring GetCurrentDesktopName(void);
    static bool IsCurrentDesktop(const std::wstring& desktopName);
    static bool SwitchDesktop(const std::wstring& desktopName);
    static bool CreateDesktop(const std::wstring& desktopName);
    static bool LaunchApplication(const std::wstring& applicationFilePath, const std::wstring& desktopName);

private:
    static void PopulateDesktopList(void);
    static std::vector<std::wstring> m_desktopNames;
};
