#pragma once

#include <windows.h>
#include <string>
#include <mutex>
#include <filesystem>

namespace GoodByDpi_App::Services
{
    class CrashLogger
    {
    public:
        static CrashLogger& Instance();

        void Initialize();
        void LogMessage(std::wstring const& category, std::wstring const& message);
        void LogException(PEXCEPTION_POINTERS pEx);
        void LogXamlException(std::wstring const& message);

        std::filesystem::path GetCrashLogPath() const;

    private:
        CrashLogger();
        ~CrashLogger() = default;

        CrashLogger(CrashLogger const&) = delete;
        CrashLogger& operator=(CrashLogger const&) = delete;

        static LONG WINAPI UnhandledFilter(PEXCEPTION_POINTERS pEx);
        static void TerminateHandler();

        mutable std::mutex m_mutex;
        std::filesystem::path m_logPath;
        bool m_initialized{ false };
    };
}
