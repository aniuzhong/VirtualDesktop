#include "stdafx.h"

std::wstring GetLastErrorMessage(void)
{
    LPWSTR pszBuffer = NULL;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        0,
        reinterpret_cast<LPWSTR>(&pszBuffer),
        0,
        NULL);

    std::wstring errorMessage;
    if (pszBuffer)
    {
        wil::unique_hlocal hBuffer(reinterpret_cast<HLOCAL>(pszBuffer));
        errorMessage = pszBuffer;
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
        output += L"\nError Number: " + std::to_wstring(iErrorNo)
            + L"\nSystem Error Description: " + GetLastErrorMessage();

    OutputDebugStringW(output.c_str());
}
