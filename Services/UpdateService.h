#pragma once
#include <string>
#include <functional>
#include <memory>
#include <cstdint>

namespace GoodByDpi_App::Services
{
    struct UpdateInfo
    {
        bool isUpdateAvailable{ false };
        std::wstring currentVersion{ L"v1.1.0" };
        std::wstring latestVersion;
        std::wstring releaseTitle;
        std::wstring releaseUrl;
        std::wstring downloadUrl;
        uint64_t downloadSize{ 0 };
    };

    class UpdateService
    {
    public:
        static UpdateService& Instance();

        std::wstring GetCurrentVersion() const;

        void CheckForUpdatesAsync(std::function<void(bool success, UpdateInfo const& info)> onComplete);

        void DownloadUpdateAsync(
            std::wstring const& downloadUrl,
            std::function<void(size_t downloaded, size_t total)> onProgress,
            std::function<void(bool success, std::wstring const& downloadedFilePath)> onComplete);

        bool ApplyUpdateAndRestart(std::wstring const& downloadedFilePath);

        void OpenReleasePage(std::wstring const& releaseUrl);

    private:
        UpdateService() = default;
        ~UpdateService() = default;
        UpdateService(UpdateService const&) = delete;
        UpdateService& operator=(UpdateService const&) = delete;

        static bool ParseReleaseJson(std::wstring const& jsonStr, UpdateInfo& outInfo);
        static bool IsNewerVersion(std::wstring const& currentVer, std::wstring const& latestVer);
    };
}
