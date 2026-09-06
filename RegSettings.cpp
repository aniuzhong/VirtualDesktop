#include "stdafx.h"
#include "RegSettings.h"

namespace
{
    constexpr PCWSTR ROOT_SUBKEY = L"SOFTWARE\\DigitalBlackCat\\Virtual Desktop";
    const HKEY ROOT_KEY = HKEY_LOCAL_MACHINE;
    std::wstring MakeValuePath(PCWSTR section)
    {
        return std::wstring(ROOT_SUBKEY) + L"\\" + section;
    }
}

UINT RegSettings::ReadProfileInt(PCWSTR section, PCWSTR entry, UINT defaultValue)
{
    std::wstring valuePath = MakeValuePath(section);
    std::optional<uint32_t> value = wil::reg::try_get_value_dword(ROOT_KEY, valuePath.c_str(), entry);
    return value.value_or(defaultValue);
}

bool RegSettings::SetProfileInt(PCWSTR section, PCWSTR entry, UINT value)
{
    std::wstring valuePath = MakeValuePath(section);
    return SUCCEEDED(wil::reg::set_value_dword_nothrow(ROOT_KEY, valuePath.c_str(), entry, value));
}
