#include "pch.h"
#include "MainViewModel.h"
#include "Services/SettingsManager.h"
#include "Services/PingService.h"
#include "Utils/PathUtils.h"
#include "Utils/StringUtils.h"

namespace GoodByDpi_App::ViewModels
{
    MainViewModel::MainViewModel()
    {
        Services::LocalizationService::Instance().RegisterLanguageChangedCallback([this](Services::AppLanguage) {
            NotifyChanged();
        });
        Services::SettingsManager::Instance().RegisterSettingsChangedCallback([this]() {
            NotifyChanged();
        });
        Services::WhitelistManager::Instance().RegisterChangedCallback([this]() {
            NotifyChanged();
        });
        m_isRunning = Services::DpiEngine::Instance().IsRunning();
    }

    bool MainViewModel::IsRunning() const
    {
        return m_isRunning;
    }

    void MainViewModel::SetRunning(bool running)
    {
        if (running)
        {
            auto preset = Services::SettingsManager::Instance().GetPreset();
            Services::DpiEngine::Instance().Start(preset);
            m_isRunning = Services::DpiEngine::Instance().IsRunning();
        }
        else
        {
            Services::DpiEngine::Instance().Stop();
            m_isRunning = false;
        }
        NotifyChanged();
    }

    void MainViewModel::ToggleRunning()
    {
        SetRunning(!m_isRunning);
    }

    Services::AppLanguage MainViewModel::GetCurrentLanguage() const
    {
        return Services::LocalizationService::Instance().GetCurrentLanguage();
    }

    void MainViewModel::SetLanguage(Services::AppLanguage lang)
    {
        Services::LocalizationService::Instance().SetLanguage(lang);
    }

    Services::DpiPreset MainViewModel::GetPreset() const
    {
        return Services::SettingsManager::Instance().GetPreset();
    }

    void MainViewModel::SetPreset(Services::DpiPreset preset)
    {
        Services::SettingsManager::Instance().SetPreset(preset);
        if (m_isRunning)
        {
            Services::DpiEngine::Instance().Start(preset);
        }
        NotifyChanged();
    }

    std::wstring MainViewModel::GetPresetName(Services::DpiPreset preset) const
    {
        std::wstring key = L"Preset_" + std::to_wstring(static_cast<int>(preset));
        return Services::LocalizationService::Instance().GetString(key);
    }

    std::wstring MainViewModel::GetPingCountry() const
    {
        return Services::SettingsManager::Instance().GetPingCountry();
    }

    void MainViewModel::SetPingCountry(std::wstring const& country)
    {
        Services::SettingsManager::Instance().SetPingCountry(country);
        Services::PingService::Instance().SetCountry(country);
        NotifyChanged();
    }

    std::vector<std::wstring> MainViewModel::GetWhitelistItems() const
    {
        return Services::WhitelistManager::Instance().GetItems();
    }

    bool MainViewModel::AddWhitelistItem(std::wstring const& item)
    {
        std::wstring cfg = Utils::GetSettingsFilePath();
        bool res = Services::WhitelistManager::Instance().AddItem(item, cfg);
        if (res) NotifyChanged();
        return res;
    }

    bool MainViewModel::RemoveWhitelistItem(std::wstring const& item)
    {
        std::wstring cfg = Utils::GetSettingsFilePath();
        bool res = Services::WhitelistManager::Instance().RemoveItem(item, cfg);
        if (res) NotifyChanged();
        return res;
    }

    bool MainViewModel::IsWhitelisted(std::wstring const& item) const
    {
        if (item.empty()) return false;
        std::string s = Utils::WideToUtf8(item);
        return Services::WhitelistManager::Instance().IsWhitelisted(s, s);
    }

    std::wstring MainViewModel::AppTitle() const
    {
        return Services::LocalizationService::Instance().GetString(L"AppTitle");
    }

    std::wstring MainViewModel::SettingsButtonText() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsButton");
    }

    std::wstring MainViewModel::TitleBarMinimize() const
    {
        return Services::LocalizationService::Instance().GetString(L"TitleBarMinimize");
    }

    std::wstring MainViewModel::TitleBarClose() const
    {
        return Services::LocalizationService::Instance().GetString(L"TitleBarClose");
    }

    std::wstring MainViewModel::StatusPillText() const
    {
        return Services::LocalizationService::Instance().GetString(m_isRunning ? L"StatusActive" : L"StatusDisabled");
    }

    std::wstring MainViewModel::SettingsTitle() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsTitle");
    }

    std::wstring MainViewModel::SettingsLanguageLabel() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsLanguage");
    }

    std::wstring MainViewModel::SettingsLanguageDesc() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsLanguageDesc");
    }

    std::wstring MainViewModel::SettingsAutoStart() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsAutoStart");
    }

    std::wstring MainViewModel::SettingsAutoStartDesc() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsAutoStartDesc");
    }

    std::wstring MainViewModel::SettingsIspPreset() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsIspPreset");
    }

    std::wstring MainViewModel::SettingsIspPresetDesc() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsIspPresetDesc");
    }

    std::wstring MainViewModel::SettingsPingCountry() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsPingCountry");
    }

    std::wstring MainViewModel::SettingsPingCountryDesc() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsPingCountryDesc");
    }

    std::wstring MainViewModel::SettingsWhitelistTitle() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsWhitelistTitle");
    }

    std::wstring MainViewModel::SettingsWhitelistDesc() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsWhitelistDesc");
    }

    std::wstring MainViewModel::SettingsWhitelistAddBtn() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsWhitelistAddBtn");
    }

    std::wstring MainViewModel::SettingsWhitelistPlaceholder() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsWhitelistPlaceholder");
    }

    std::wstring MainViewModel::SettingsWhitelistEmpty() const
    {
        return Services::LocalizationService::Instance().GetString(L"SettingsWhitelistEmpty");
    }

    bool MainViewModel::GetAutoStart() const
    {
        return Services::SettingsManager::Instance().GetAutoStart();
    }

    void MainViewModel::SetAutoStart(bool autoStart)
    {
        Services::SettingsManager::Instance().SetAutoStart(autoStart);
    }

    std::wstring MainViewModel::CloseDialogTitle() const
    {
        return Services::LocalizationService::Instance().GetString(L"CloseDialogTitle");
    }

    std::wstring MainViewModel::CloseDialogMessage() const
    {
        return Services::LocalizationService::Instance().GetString(L"CloseDialogMessage");
    }

    std::wstring MainViewModel::CloseDialogMinimizeTray() const
    {
        return Services::LocalizationService::Instance().GetString(L"CloseDialogMinimizeTray");
    }

    std::wstring MainViewModel::CloseDialogExit() const
    {
        return Services::LocalizationService::Instance().GetString(L"CloseDialogExit");
    }

    std::wstring MainViewModel::CloseDialogCancel() const
    {
        return Services::LocalizationService::Instance().GetString(L"CloseDialogCancel");
    }

    std::wstring MainViewModel::TrayOpen() const
    {
        return Services::LocalizationService::Instance().GetString(L"TrayOpen");
    }

    std::wstring MainViewModel::TrayStart() const
    {
        return Services::LocalizationService::Instance().GetString(L"TrayStart");
    }

    std::wstring MainViewModel::TrayStop() const
    {
        return Services::LocalizationService::Instance().GetString(L"TrayStop");
    }

    std::wstring MainViewModel::TrayExit() const
    {
        return Services::LocalizationService::Instance().GetString(L"TrayExit");
    }

    std::wstring MainViewModel::TrayTip() const
    {
        return Services::LocalizationService::Instance().GetString(m_isRunning ? L"TrayTipActive" : L"TrayTipInactive");
    }

    std::wstring MainViewModel::TrayNotificationTitle() const
    {
        return Services::LocalizationService::Instance().GetString(L"TrayNotificationTitle");
    }

    std::wstring MainViewModel::TrayNotificationBody() const
    {
        return Services::LocalizationService::Instance().GetString(L"TrayNotificationBody");
    }

    std::wstring MainViewModel::LogButtonText() const
    {
        return Services::LocalizationService::Instance().GetString(L"LogButton");
    }

    std::wstring MainViewModel::LogTitle() const
    {
        return Services::LocalizationService::Instance().GetString(L"LogTitle");
    }

    std::wstring MainViewModel::LogStatusProcessed() const
    {
        return Services::LocalizationService::Instance().GetString(L"LogStatusProcessed");
    }

    std::wstring MainViewModel::LogStatusBypassed() const
    {
        return Services::LocalizationService::Instance().GetString(L"LogStatusBypassed");
    }

    std::wstring MainViewModel::LogAddWhitelist() const
    {
        return Services::LocalizationService::Instance().GetString(L"LogAddWhitelist");
    }

    std::wstring MainViewModel::LogRemoveWhitelist() const
    {
        return Services::LocalizationService::Instance().GetString(L"LogRemoveWhitelist");
    }

    void MainViewModel::RegisterPropertyChangedCallback(std::function<void()> callback)
    {
        m_callbacks.push_back(callback);
    }

    void MainViewModel::NotifyChanged()
    {
        for (auto const& callback : m_callbacks)
        {
            if (callback)
            {
                callback();
            }
        }
    }
}
