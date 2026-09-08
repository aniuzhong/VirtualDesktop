#include "stdafx.h"
#include "wilx/win32_helpers.h"
#include "wilx/strings.h"

void LogErrorMessage(PCWSTR message, DWORD lastError)
{
    if (lastError)
        spdlog::error("{} (error {}: {})", wilx::TryGetUtf8String(message), lastError,
            wilx::TryGetUtf8String(wilx::TryGetWin32ErrorMessage(lastError)));
    else
        spdlog::error("{}", wilx::TryGetUtf8String(message));
}
