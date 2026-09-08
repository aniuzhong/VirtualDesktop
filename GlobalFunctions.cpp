#include "stdafx.h"
#include "wilx/win32_helpers.h"
#include "wilx/strings.h"

void DebugPrintErrorMessage(const wchar_t* message, DWORD lastError)
{
    std::wstring output = L"\n";
    if (message)
        output += message;

    if (lastError)
        output += std::format(L"\nError Number: {}\nSystem Error Description: {}", lastError,
            wilx::TryGetWin32ErrorMessage(lastError));

    OutputDebugStringW(output.c_str());

    std::wstring logLine = std::move(output);
    logLine.erase(0, logLine.find_first_not_of(L'\n'));
    spdlog::error("{}", wilx::TryGetUtf8String(logLine));
}
