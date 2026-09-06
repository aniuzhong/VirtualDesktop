#include "stdafx.h"
#include "DesktopManager.h"

extern "C" SHSTDAPI_(BOOL) PathIsExe(_In_ PCWSTR pszPath);

std::vector<std::wstring> CDesktopManager::m_desktopNames;

namespace
{
    BOOL CALLBACK EnumDesktopProc(LPWSTR lpszDesktopName, LPARAM lParam)
    {
        reinterpret_cast<std::vector<std::wstring>*>(lParam)->emplace_back(lpszDesktopName);
        return TRUE;
    }
}

void CDesktopManager::PopulateDesktopList(void)
{
    HWINSTA hWindowsStation = GetProcessWindowStation();
    if (NULL == hWindowsStation)
    {
        DebugPrintErrorMessage(L"GetProcessWindowStation failed in PopulateDesktopList.");
        return;
    }

    std::vector<std::wstring> desktopNames;
    if (!EnumDesktopsW(hWindowsStation, EnumDesktopProc, reinterpret_cast<LPARAM>(&desktopNames)))
    {
        DebugPrintErrorMessage(L"EnumDesktops failed in PopulateDesktopList.");
        return;
    }

    m_desktopNames = std::move(desktopNames);
}

int CDesktopManager::GetDesktopCount(void)
{
    PopulateDesktopList();
    return static_cast<int>(m_desktopNames.size());
}

std::wstring CDesktopManager::GetDesktopName(int iIndex)
{
    if (iIndex < 0 || iIndex >= static_cast<int>(m_desktopNames.size()))
        return std::wstring();
    return m_desktopNames[iIndex];
}

std::wstring CDesktopManager::GetCurrentDesktopName(void)
{
    HDESK hCurrentDesktop = GetThreadDesktop(GetCurrentThreadId());
    if (NULL == hCurrentDesktop)
    {
        DebugPrintErrorMessage(L"GetThreadDesktop failed in GetCurrentDesktopName.");
        return std::wstring();
    }

    wchar_t szDesktopName[ARRAY_SIZE] = { 0 };
    DWORD iOutCount = 0;
    if (!GetUserObjectInformationW(hCurrentDesktop, UOI_NAME, szDesktopName, sizeof(szDesktopName) - sizeof(wchar_t), &iOutCount))
    {
        DebugPrintErrorMessage(L"GetUserObjectInformation failed in GetCurrentDesktopName.");
        return std::wstring();
    }

    return szDesktopName;
}

bool CDesktopManager::IsCurrentDesktop(const std::wstring& desktopName)
{
    std::wstring currentName = GetCurrentDesktopName();
    if (currentName.empty())
        return false;
    return _wcsicmp(desktopName.c_str(), currentName.c_str()) == 0;
}

bool CDesktopManager::SwitchDesktop(const std::wstring& desktopName)
{
    if (desktopName.empty())
        return false;

    wil::unique_hdesk hDesktopToSwitch(OpenDesktopW(desktopName.c_str(), DF_ALLOWOTHERACCOUNTHOOK, TRUE, GENERIC_ALL));
    if (!hDesktopToSwitch)
    {
        if (ERROR_ACCESS_DENIED == GetLastError())
        {
            std::wstring errorMsg = L"Failed to switch to " + desktopName + L" desktop.\n\t " + GetLastErrorMessage();
            MessageBoxW(NULL, errorMsg.c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
        }
        DebugPrintErrorMessage(L"OpenDesktop failed in SwitchDesktop.");
        return false;
    }

    if (!::SwitchDesktop(hDesktopToSwitch.get()))
    {
        DebugPrintErrorMessage(L"SwitchDesktop failed in SwitchDesktop.");
        return false;
    }

    return true;
}

bool CDesktopManager::CreateDesktop(const std::wstring& desktopName)
{
    if (desktopName.empty())
        return false;

    SECURITY_ATTRIBUTES sAttribute = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    wil::unique_hdesk hNewDesktop(::CreateDesktopW(desktopName.c_str(), NULL, NULL, DF_ALLOWOTHERACCOUNTHOOK, GENERIC_ALL, &sAttribute));
    if (!hNewDesktop)
    {
        std::wstring errorMsg = GetLastErrorMessage();
        MessageBoxW(NULL, errorMsg.c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
        DebugPrintErrorMessage(L"CreateDesktop failed in CreateDesktop.");
        return false;
    }

    bool alreadyExists = std::any_of(m_desktopNames.begin(), m_desktopNames.end(),
        [&desktopName](const std::wstring& existing) { return _wcsicmp(existing.c_str(), desktopName.c_str()) == 0; });
    if (!alreadyExists)
    {
        wchar_t szWindowsDirectory[ARRAY_SIZE] = { 0 };
        GetWindowsDirectoryW(szWindowsDirectory, ARRAY_SIZE - 1);
        LaunchApplication(std::wstring(szWindowsDirectory) + L"\\Explorer.Exe", desktopName);
    }

    PopulateDesktopList();
    return true;
}

bool CDesktopManager::LaunchApplication(const std::wstring& applicationFilePath, const std::wstring& desktopName)
{
    if (applicationFilePath.empty() || desktopName.empty())
        return false;

    if (!PathIsExe(applicationFilePath.c_str()))
    {
        DebugPrintErrorMessage(L"Invalid File Extension in LaunchApplication.");
        return false;
    }

    std::wstring directoryName = applicationFilePath;
    PathRemoveFileSpecW(directoryName.data());

    wil::unique_process_information processInfo;
    STARTUPINFOW sInfo = { 0 };
    sInfo.cb = sizeof(sInfo);
    sInfo.lpDesktop = const_cast<LPWSTR>(desktopName.c_str());

    if (!CreateProcessW(applicationFilePath.c_str(),
            NULL,
            NULL,
            NULL,
            TRUE,
            NORMAL_PRIORITY_CLASS,
            NULL,
            directoryName.c_str(),
            &sInfo,
            &processInfo))
    {
        DebugPrintErrorMessage(GetLastErrorMessage().c_str());
        return false;
    }

    return true;
}
