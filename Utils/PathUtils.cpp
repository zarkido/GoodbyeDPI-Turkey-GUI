#include "pch.h"
#include "PathUtils.h"
#include <shlobj.h>

namespace GoodByDpi_App::Utils
{
    std::filesystem::path GetAppDataDirectory()
    {
        wchar_t const* appData = _wgetenv(L"APPDATA");
        std::filesystem::path dir;
        if (appData && wcslen(appData) > 0)
        {
            dir = std::filesystem::path(appData) / L"GoodByDpi";
        }
        else
        {
            PWSTR knownPath = nullptr;
            if (SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &knownPath)) && knownPath)
            {
                dir = std::filesystem::path(knownPath) / L"GoodByDpi";
                ::CoTaskMemFree(knownPath);
            }
            else
            {
                wchar_t buffer[MAX_PATH];
                ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
                dir = std::filesystem::path(buffer).parent_path() / L"GoodByDpi";
            }
        }

        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        wchar_t const* localApp = _wgetenv(L"LOCALAPPDATA");
        if (localApp && wcslen(localApp) > 0)
        {
            std::filesystem::path legacyLocal = std::filesystem::path(localApp) / L"GoodByDpi";
            if (std::filesystem::exists(legacyLocal, ec))
            {
                std::filesystem::remove_all(legacyLocal, ec);
            }
        }

        return dir;
    }

    std::wstring GetSettingsFilePath()
    {
        auto configPath = GetAppDataDirectory() / L"settings.ini";
        std::error_code ec;
        if (!std::filesystem::exists(configPath, ec))
        {
            wchar_t exeBuffer[MAX_PATH];
            ::GetModuleFileNameW(nullptr, exeBuffer, MAX_PATH);
            std::filesystem::path oldConfig = std::filesystem::path(exeBuffer).parent_path() / L"settings.ini";
            if (std::filesystem::exists(oldConfig, ec))
            {
                std::filesystem::copy_file(oldConfig, configPath, std::filesystem::copy_options::overwrite_existing, ec);
            }
        }
        return configPath.wstring();
    }

    std::wstring GetCrashLogFilePath()
    {
        return (GetAppDataDirectory() / L"crashlog.txt").wstring();
    }

    std::wstring GetPermanentLauncherPath()
    {
        return (GetAppDataDirectory() / L"app" / L"GoodByDpi.exe").wstring();
    }
}
