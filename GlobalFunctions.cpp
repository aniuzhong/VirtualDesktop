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
}
