#include "stdafx.h"
#include "wilx/desktops.h"
#include "wilx/win32_helpers.h"
#include "wilx/strings.h"
#include "DesktopManager.h"

namespace
{
    constexpr std::wstring_view ExecutableExtensions[] = { L".exe", L".com", L".pif", L".scr" };

    std::vector<std::wstring> g_desktopNames;

    bool IsExecutableFile(const std::wstring& filePath)
    {
        std::wstring extension = std::filesystem::path(filePath).extension();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
        return std::find(std::begin(ExecutableExtensions), std::end(ExecutableExtensions), extension) != std::end(ExecutableExtensions);
    }

    std::wstring QueryWindowsDirectory(void)
    {
        std::wstring windowsDirectory;
        if (FAILED(wil::GetWindowsDirectoryW(windowsDirectory)))
        {
            LogErrorMessage(L"GetWindowsDirectory failed in QueryWindowsDirectory.", GetLastError());
            return {};
        }
        return windowsDirectory;
    }

    void PopulateDesktopList(void)
    {
        HWINSTA hWindowsStation = GetProcessWindowStation();
        if (NULL == hWindowsStation)
        {
            LogErrorMessage(L"GetProcessWindowStation failed in PopulateDesktopList.", GetLastError());
            return;
        }

        std::vector<std::wstring> desktopNames;
        wilx::for_each_desktop_nothrow(hWindowsStation, [&](PCWSTR lpszDesktopName) {
            desktopNames.emplace_back(lpszDesktopName);
        });

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
        return wilx::TryGetThreadDesktopName();
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
            DWORD openError = GetLastError();
            if (ERROR_ACCESS_DENIED == openError)
            {
                std::wstring errorMsg = std::format(L"Failed to switch to {} desktop.\n\t {}",
                    desktopName, wilx::TryGetWin32ErrorMessage(openError));
                MessageBoxW(NULL, errorMsg.c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONINFORMATION | MB_TOPMOST | MB_TASKMODAL);
            }
            LogErrorMessage(L"OpenDesktop failed in SwitchDesktop.", openError);
            return false;
        }

        if (!::SwitchDesktop(hDesktopToSwitch.get()))
        {
            LogErrorMessage(L"SwitchDesktop failed in SwitchDesktop.", GetLastError());
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
            DWORD createError = GetLastError();
            MessageBoxW(NULL, wilx::TryGetWin32ErrorMessage(createError).c_str(), TXT_MESSAGEBOX_TITLE, MB_ICONERROR | MB_TOPMOST | MB_TASKMODAL);
            LogErrorMessage(L"CreateDesktop failed in CreateDesktop.", createError);
            return false;
        }

        bool alreadyExists = std::any_of(g_desktopNames.begin(), g_desktopNames.end(),
            [&desktopName](const std::wstring& existing) { return _wcsicmp(existing.c_str(), desktopName.c_str()) == 0; });
        if (!alreadyExists)
            LaunchApplication(QueryWindowsDirectory() + L"\\Explorer.Exe", desktopName);

        spdlog::info("desktop '{}' created", wilx::TryGetUtf8String(desktopName));
        PopulateDesktopList();
        return true;
    }

    bool LaunchApplication(const std::wstring& applicationFilePath, const std::wstring& desktopName)
    {
        if (applicationFilePath.empty() || desktopName.empty())
            return false;

        if (!IsExecutableFile(applicationFilePath))
        {
            LogErrorMessage(L"Invalid File Extension in LaunchApplication.", 0);
            return false;
        }

        std::wstring directoryName = std::filesystem::path(applicationFilePath).parent_path().wstring();

        spdlog::info("launch '{}' on desktop '{}'",
            wilx::TryGetUtf8String(applicationFilePath), wilx::TryGetUtf8String(desktopName));

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
            LogErrorMessage(L"CreateProcess failed in LaunchApplication.", GetLastError());
            return false;
        }

        return true;
    }
}
