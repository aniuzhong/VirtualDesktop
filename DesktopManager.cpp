#include "stdafx.h"
#include "DesktopManager.h"

namespace
{
    constexpr std::wstring_view ExecutableExtensions[] = { L".exe", L".com", L".pif", L".scr" };

    std::vector<std::wstring> g_desktopNames;

    BOOL CALLBACK EnumDesktopProc(LPWSTR lpszDesktopName, LPARAM lParam)
    {
        reinterpret_cast<std::vector<std::wstring>*>(lParam)->emplace_back(lpszDesktopName);
        return TRUE;
    }

    bool IsExecutableFile(const std::wstring& filePath)
    {
        std::wstring extension = std::filesystem::path(filePath).extension();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
        return std::find(std::begin(ExecutableExtensions), std::end(ExecutableExtensions), extension) != std::end(ExecutableExtensions);
    }

    std::wstring QueryWindowsDirectory(void)
    {
        std::wstring windowsDirectory(MAX_PATH + 1, L'\0');
        UINT iLength = GetWindowsDirectoryW(windowsDirectory.data(), static_cast<UINT>(windowsDirectory.size()));
        if (iLength > windowsDirectory.size())
        {
            windowsDirectory.resize(iLength);
            iLength = GetWindowsDirectoryW(windowsDirectory.data(), static_cast<UINT>(windowsDirectory.size()));
        }
        windowsDirectory.resize(iLength);
        return windowsDirectory;
    }

    void PopulateDesktopList(void)
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

        g_desktopNames = std::move(desktopNames);
    }
}

namespace DesktopManager
{
    int GetDesktopCount(void)
    {
        PopulateDesktopList();
        return static_cast<int>(g_desktopNames.size());
    }

    std::wstring GetDesktopName(int iIndex)
    {
        if (iIndex < 0 || iIndex >= static_cast<int>(g_desktopNames.size()))
            return std::wstring();
        return g_desktopNames[iIndex];
    }

    std::wstring GetCurrentDesktopName(void)
    {
        HDESK hCurrentDesktop = GetThreadDesktop(GetCurrentThreadId());
        if (NULL == hCurrentDesktop)
        {
            DebugPrintErrorMessage(L"GetThreadDesktop failed in GetCurrentDesktopName.");
            return std::wstring();
        }

        DWORD iBytesNeeded = 0;
        if (!GetUserObjectInformationW(hCurrentDesktop, UOI_NAME, nullptr, 0, &iBytesNeeded) &&
            ERROR_INSUFFICIENT_BUFFER != GetLastError())
        {
            DebugPrintErrorMessage(L"GetUserObjectInformation failed in GetCurrentDesktopName.");
            return std::wstring();
        }

        std::wstring desktopName(iBytesNeeded / sizeof(wchar_t) + 1, L'\0');
        if (!GetUserObjectInformationW(hCurrentDesktop, UOI_NAME, desktopName.data(), iBytesNeeded, &iBytesNeeded))
        {
            DebugPrintErrorMessage(L"GetUserObjectInformation failed in GetCurrentDesktopName.");
            return std::wstring();
        }

        desktopName.resize(wcslen(desktopName.c_str()));
        return desktopName;
    }

    bool IsCurrentDesktop(const std::wstring& desktopName)
    {
        std::wstring currentName = GetCurrentDesktopName();
        if (currentName.empty())
            return false;
        return _wcsicmp(desktopName.c_str(), currentName.c_str()) == 0;
    }

    bool SwitchDesktop(const std::wstring& desktopName)
    {
        if (desktopName.empty())
            return false;

        wil::unique_hdesk hDesktopToSwitch(OpenDesktopW(desktopName.c_str(), DF_ALLOWOTHERACCOUNTHOOK, TRUE, GENERIC_ALL));
        if (!hDesktopToSwitch)
        {
            if (ERROR_ACCESS_DENIED == GetLastError())
            {
                std::wstring errorMsg = std::format(L"Failed to switch to {} desktop.\n\t {}", desktopName, GetLastErrorMessage());
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

    bool CreateDesktop(const std::wstring& desktopName)
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

        bool alreadyExists = std::any_of(g_desktopNames.begin(), g_desktopNames.end(),
            [&desktopName](const std::wstring& existing) { return _wcsicmp(existing.c_str(), desktopName.c_str()) == 0; });
        if (!alreadyExists)
            LaunchApplication(QueryWindowsDirectory() + L"\\Explorer.Exe", desktopName);

        spdlog::info("desktop '{}' created", ToUtf8(desktopName));
        PopulateDesktopList();
        return true;
    }

    bool LaunchApplication(const std::wstring& applicationFilePath, const std::wstring& desktopName)
    {
        if (applicationFilePath.empty() || desktopName.empty())
            return false;

        if (!IsExecutableFile(applicationFilePath))
        {
            DebugPrintErrorMessage(L"Invalid File Extension in LaunchApplication.");
            return false;
        }

        std::wstring directoryName = std::filesystem::path(applicationFilePath).parent_path().wstring();

        spdlog::info("launch '{}' on desktop '{}'", ToUtf8(applicationFilePath), ToUtf8(desktopName));

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
}
