#include "stdafx.h"

std::wstring GetLastErrorMessage(void)
{
    wil::unique_hlocal buffer;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        GetLastError(),
        0,
        reinterpret_cast<LPWSTR>(buffer.put()),
        0,
        nullptr);

    std::wstring errorMessage;
    if (buffer)
    {
        errorMessage = static_cast<PWSTR>(buffer.get());
        while (!errorMessage.empty() && (errorMessage.back() == L'\n' || errorMessage.back() == L'\r'))
            errorMessage.pop_back();
    }
    return errorMessage;
}

void DebugPrintErrorMessage(const wchar_t* pszErrorString)
{
    DWORD iErrorNo = GetLastError();

    std::wstring output = L"\n";
    if (pszErrorString)
        output += pszErrorString;

    if (iErrorNo)
        output += std::format(L"\nError Number: {}\nSystem Error Description: {}", iErrorNo, GetLastErrorMessage());

    OutputDebugStringW(output.c_str());

    std::wstring logLine = output;
    logLine.erase(0, logLine.find_first_not_of(L'\n'));
    spdlog::error("{}", ToUtf8(logLine));
}

std::string ToUtf8(std::wstring_view text)
{
    if (text.empty())
        return {};

    int iBytes = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string utf8(iBytes, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), iBytes, nullptr, nullptr);
    return utf8;
}
