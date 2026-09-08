#include "stdafx.h"
#include <shlobj.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include "wilx/win32_helpers.h"
#include "wilx/strings.h"

namespace
{
    void Write(spdlog::level::level_enum level, std::wstring_view message, const std::source_location& loc)
    {
        spdlog::log(spdlog::source_loc{loc.file_name(), static_cast<int>(loc.line()), loc.function_name()},
            level, "{}", wilx::TryGetUtf8String(message));
    }
}

void InitLogging(void)
{
    try
    {
        wil::unique_cotaskmem_string appData;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, appData.put())))
            return;

        std::filesystem::path logDirectory = std::filesystem::path(appData.get()) / L"VirtualDesktop" / L"logs";
        std::filesystem::create_directories(logDirectory);

        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            wilx::TryGetUtf8String((logDirectory / L"virtualdesktop.log").wstring()), 1024 * 1024, 1);
        auto logger = std::make_shared<spdlog::logger>("virtualdesktop", std::move(fileSink));
        logger->set_pattern("[%Y-%m-%d %T.%e] [P%P T%t] [%l] [%s:%#] %v");
        logger->flush_on(spdlog::level::info);
        spdlog::set_default_logger(std::move(logger));
    }
    catch (const std::exception&)
    {
        // Logging is a debug aid only; keep running with the default (console) logger.
    }
}

void LogInfo(std::wstring_view message, const std::source_location& loc)
{
    Write(spdlog::level::info, message, loc);
}

void LogWarn(std::wstring_view message, const std::source_location& loc)
{
    Write(spdlog::level::warn, message, loc);
}

void LogError(std::wstring_view message, const std::source_location& loc)
{
    Write(spdlog::level::err, message, loc);
}

void LogError(std::wstring_view message, DWORD lastError, const std::source_location& loc)
{
    if (lastError)
        Write(spdlog::level::err,
            std::format(L"{} (error {}: {})", message, lastError, wilx::TryGetWin32ErrorMessage(lastError)), loc);
    else
        LogError(message, loc);
}
