#pragma once

#include "stdafx.h"

namespace RegSettings
{
    UINT ReadProfileInt(PCWSTR section, PCWSTR entry, UINT defaultValue);
    bool SetProfileInt(PCWSTR section, PCWSTR entry, UINT value);
}
