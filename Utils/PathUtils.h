#pragma once
#include <string>
#include <filesystem>

namespace GoodByDpi_App::Utils
{
    std::filesystem::path GetExecutablePath();
    std::filesystem::path GetExecutableDirectory();
    std::wstring GetLauncherExecutablePath();
    std::filesystem::path GetAppDataDirectory();
    std::wstring GetSettingsFilePath();
    std::wstring GetCrashLogFilePath();
    std::wstring GetPermanentLauncherPath();
}
