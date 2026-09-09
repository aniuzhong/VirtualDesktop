#include "logging.h"

#include <filesystem>
#include <format>

#include <shlobj.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "wilx/win32_helpers.h"

namespace
{

void write(spdlog::level::level_enum level, std::wstring_view message) {
    spdlog::log(level, "{}", wilx::TryGetUtf8String(message));
}

} // namespace

namespace desktops::log
{

void init() {
    try {
        wil::unique_cotaskmem_string appData;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, appData.put())))
            return;
        const std::filesystem::path logDirectory = std::filesystem::path(appData.get()) / L"VirtualDesktop" / L"logs";
        std::filesystem::create_directories(logDirectory);
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(wilx::TryGetUtf8String((logDirectory / L"virtualdesktop.log").wstring()), 1024 * 1024, 1);
        auto logger = std::make_shared<spdlog::logger>("desktops", std::move(fileSink));
        logger->set_pattern("[%Y-%m-%d %T.%e] [P%P T%t] [%l] %v");
        logger->flush_on(spdlog::level::info);
        spdlog::set_level(spdlog::level::debug);
        spdlog::set_default_logger(std::move(logger));
    } catch (const std::exception&) {
        // Logging is a debug aid only; keep running with the default (console) logger.
    }
}

void debug(std::wstring_view message) { write(spdlog::level::debug, message); }
void info(std::wstring_view message) { write(spdlog::level::info, message); }
void warn(std::wstring_view message) { write(spdlog::level::warn, message); }
void error(std::wstring_view message) { write(spdlog::level::err, message); }

    void error(std::wstring_view message, DWORD lastError)
    {
        if (lastError)
            write(spdlog::level::err,
                std::format(L"{} (error {}: {})", message, lastError, wilx::TryGetWin32ErrorMessage(lastError)));
        else
            error(message);
    }
}
