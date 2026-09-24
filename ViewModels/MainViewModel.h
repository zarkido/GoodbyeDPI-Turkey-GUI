#pragma once
#include <string>
#include <functional>
#include <vector>
#include "Services/LocalizationService.h"
#include "Services/SettingsManager.h"
#include "Services/DpiEngine.h"
#include "Services/PingService.h"
#include "Services/WhitelistManager.h"
#include "Services/TrafficLogger.h"
#include "Services/UpdateService.h"

namespace GoodByDpi_App::ViewModels
{
    class MainViewModel
    {
    public:
        MainViewModel();

        bool IsRunning() const;
        void SetRunning(bool running);
        void ToggleRunning();

        Services::AppLanguage GetCurrentLanguage() const;
        void SetLanguage(Services::AppLanguage lang);

        Services::DpiPreset GetPreset() const;
        void SetPreset(Services::DpiPreset preset);
        std::wstring GetPresetName(Services::DpiPreset preset) const;

        std::wstring GetPingCountry() const;
        void SetPingCountry(std::wstring const& country);

        std::vector<std::wstring> GetWhitelistItems() const;
        bool AddWhitelistItem(std::wstring const& item);
        bool RemoveWhitelistItem(std::wstring const& item);
        bool IsWhitelisted(std::wstring const& item) const;

        std::wstring AppTitle() const;
        std::wstring SettingsButtonText() const;
        std::wstring TitleBarMinimize() const;
        std::wstring TitleBarClose() const;
        std::wstring StatusPillText() const;
        std::wstring SettingsTitle() const;
        std::wstring SettingsLanguageLabel() const;
        std::wstring SettingsLanguageDesc() const;
        std::wstring SettingsAutoStart() const;
        std::wstring SettingsAutoStartDesc() const;
        std::wstring SettingsIspPreset() const;
        std::wstring SettingsIspPresetDesc() const;
        std::wstring SettingsPingCountry() const;
        std::wstring SettingsPingCountryDesc() const;
        std::wstring SettingsWhitelistTitle() const;
        std::wstring SettingsWhitelistDesc() const;
        std::wstring SettingsWhitelistAddBtn() const;
        std::wstring SettingsWhitelistPlaceholder() const;
        std::wstring SettingsWhitelistEmpty() const;
        bool GetAutoStart() const;
        void SetAutoStart(bool autoStart);
        std::wstring CloseDialogTitle() const;
        std::wstring CloseDialogMessage() const;
        std::wstring CloseDialogMinimizeTray() const;
        std::wstring CloseDialogExit() const;
        std::wstring CloseDialogCancel() const;
        std::wstring TrayOpen() const;
        std::wstring TrayStart() const;
        std::wstring TrayStop() const;
        std::wstring TrayExit() const;
        std::wstring TrayTip() const;
        std::wstring TrayNotificationTitle() const;
        std::wstring TrayNotificationBody() const;
        std::wstring LogButtonText() const;
        std::wstring LogTitle() const;
        std::wstring LogStatusProcessed() const;
        std::wstring LogStatusBypassed() const;
        std::wstring LogAddWhitelist() const;
        std::wstring LogRemoveWhitelist() const;

        std::wstring UpdateAvailableText() const;
        std::wstring UpdateCheckingText() const;
        std::wstring UpdateUpToDateText() const;
        std::wstring UpdateNowText() const;
        std::wstring UpdateViewReleaseText() const;
        std::wstring UpdateLaterText() const;
        std::wstring UpdateDownloadingText() const;
        std::wstring UpdateFailedText() const;
        std::wstring SettingsCheckUpdatesText() const;
        std::wstring SettingsCheckUpdatesDescText() const;

        bool HasUpdateAvailable() const;
        Services::UpdateInfo GetUpdateInfo() const;
        void SetUpdateInfo(Services::UpdateInfo const& info);

        void RegisterPropertyChangedCallback(std::function<void()> callback);

    private:
        void NotifyChanged();

        bool m_isRunning{ false };
        Services::UpdateInfo m_updateInfo;
        std::vector<std::function<void()>> m_callbacks;
    };
}
