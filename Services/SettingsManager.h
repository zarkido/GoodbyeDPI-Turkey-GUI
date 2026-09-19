#pragma once
#include <string>
#include <functional>
#include <vector>
#include <shared_mutex>
#include "DpiEngine.h"
#include "LocalizationService.h"

namespace GoodByDpi_App::Services
{
    class SettingsManager
    {
    public:
        static SettingsManager& Instance();

        void Initialize();

        AppLanguage GetLanguage() const;
        void SetLanguage(AppLanguage language);

        bool GetAutoStart() const;
        void SetAutoStart(bool autoStart);
        void SyncAutoStartPath();

        DpiPreset GetPreset() const;
        void SetPreset(DpiPreset preset);

        std::wstring GetPingCountry() const;
        void SetPingCountry(std::wstring const& country);

        void RegisterSettingsChangedCallback(std::function<void()> callback);

        void SaveSettings();
        void LoadSettings();

    private:
        SettingsManager();
        ~SettingsManager() = default;
        SettingsManager(SettingsManager const&) = delete;
        SettingsManager& operator=(SettingsManager const&) = delete;

        void NotifyChanged();

        mutable std::shared_mutex m_mutex;
        AppLanguage m_language{ AppLanguage::English };
        bool m_autoStart{ false };
        DpiPreset m_preset{ DpiPreset::TurkTelekomDefault };
        std::wstring m_pingCountry{ L"TR" };
        std::vector<std::function<void()>> m_callbacks;
    };
}
