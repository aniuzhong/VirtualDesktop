#pragma once

#include <source_location>
#include <string_view>

void InitLogging(void);

void LogInfo(std::wstring_view message, const std::source_location& loc = std::source_location::current());
void LogWarn(std::wstring_view message, const std::source_location& loc = std::source_location::current());
void LogError(std::wstring_view message, const std::source_location& loc = std::source_location::current());
void LogError(std::wstring_view message, DWORD lastError, const std::source_location& loc = std::source_location::current());
