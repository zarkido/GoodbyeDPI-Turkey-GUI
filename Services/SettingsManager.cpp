#include "pch.h"
#include "SettingsManager.h"
#include "PingService.h"
#include "DpiEngine.h"
#include "AutoStartManager.h"
#include "Utils/PathUtils.h"
#include <fstream>
#include <filesystem>
#include <vector>

namespace GoodByDpi_App::Services
{
    SettingsManager& SettingsManager::Instance()
    {
        static SettingsManager s_instance;
        return s_instance;
    }

    SettingsManager::SettingsManager()
    {
        Initialize();
    }

    void SettingsManager::Initialize()
    {
        LoadSettings();
        PingService::Instance().SetCountry(m_pingCountry);
        DpiEngine::Instance().SetPreset(m_preset);
        if (m_autoStart)
        {
            SyncAutoStartPath();
        }
    }

    AppLanguage SettingsManager::GetLanguage() const
    {
        std::shared_lock lock(m_mutex);
        return m_language;
    }

    void SettingsManager::SetLanguage(AppLanguage language)
    {
        {
            std::unique_lock lock(m_mutex);
            if (m_language == language) return;
            m_language = language;
        }
        SaveSettings();
        NotifyChanged();
    }

    bool SettingsManager::GetAutoStart() const
    {
        std::shared_lock lock(m_mutex);
        return m_autoStart;
    }

    void SettingsManager::SetAutoStart(bool autoStart)
    {
        {
            std::unique_lock lock(m_mutex);
            if (m_autoStart == autoStart) return;
            m_autoStart = autoStart;
        }
        AutoStartManager::SetAutoStart(autoStart);
        SaveSettings();
        NotifyChanged();
    }

    void SettingsManager::SyncAutoStartPath()
    {
        if (m_autoStart)
        {
            AutoStartManager::SyncPath();
        }
    }

    DpiPreset SettingsManager::GetPreset() const
    {
        std::shared_lock lock(m_mutex);
        return m_preset;
    }

    void SettingsManager::SetPreset(DpiPreset preset)
    {
        {
            std::unique_lock lock(m_mutex);
            if (m_preset == preset) return;
            m_preset = preset;
        }
        SaveSettings();
        DpiEngine::Instance().SetPreset(preset);
        NotifyChanged();
    }

    std::wstring SettingsManager::GetPingCountry() const
    {
        std::shared_lock lock(m_mutex);
        return m_pingCountry;
    }

    void SettingsManager::SetPingCountry(std::wstring const& country)
    {
        {
            std::unique_lock lock(m_mutex);
            if (m_pingCountry == country) return;
            m_pingCountry = country;
        }
        SaveSettings();
        PingService::Instance().SetCountry(country);
        NotifyChanged();
    }

    void SettingsManager::RegisterSettingsChangedCallback(std::function<void()> callback)
    {
        std::unique_lock lock(m_mutex);
        m_callbacks.push_back(std::move(callback));
    }

    void SettingsManager::NotifyChanged()
    {
        std::vector<std::function<void()>> cbs;
        {
            std::shared_lock lock(m_mutex);
            cbs = m_callbacks;
        }
        for (auto const& cb : cbs)
        {
            if (cb) cb();
        }
    }

    void SettingsManager::LoadSettings()
    {
        std::unique_lock lock(m_mutex);
        std::wstring configPath = Utils::GetSettingsFilePath();
        if (!std::filesystem::exists(configPath)) return;

        std::wifstream file(configPath);
        if (!file.is_open()) return;

        auto trim = [](std::wstring& s) {
            while (!s.empty() && (s.back() == L' ' || s.back() == L'\t' || s.back() == L'\r' || s.back() == L'\n'))
                s.pop_back();
            while (!s.empty() && (s.front() == L' ' || s.front() == L'\t' || s.front() == L'\r' || s.front() == L'\n'))
                s.erase(s.begin());
        };

        std::wstring line;
        while (std::getline(file, line))
        {
            trim(line);
            if (line.rfind(L"Language=", 0) == 0)
            {
                std::wstring val = line.substr(9);
                trim(val);
                if (val == L"tr" || val == L"Turkish")
                {
                    m_language = AppLanguage::Turkish;
                }
                else if (val == L"en" || val == L"English")
                {
                    m_language = AppLanguage::English;
                }
            }
            else if (line.rfind(L"AutoStart=", 0) == 0)
            {
                std::wstring val = line.substr(10);
                trim(val);
                m_autoStart = (val == L"1" || val == L"true" || val == L"True");
            }
            else if (line.rfind(L"Preset=", 0) == 0)
            {
                std::wstring val = line.substr(7);
                trim(val);
                int p = _wtoi(val.c_str());
                if (p >= 0 && p <= 6)
                {
                    m_preset = static_cast<DpiPreset>(p);
                }
            }
            else if (line.rfind(L"PingCountry=", 0) == 0)
            {
                std::wstring val = line.substr(12);
                trim(val);
                if (!val.empty())
                {
                    m_pingCountry = val;
                }
            }
        }
    }

    void SettingsManager::SaveSettings()
    {
        std::shared_lock lock(m_mutex);
        std::wstring configPath = Utils::GetSettingsFilePath();

        std::vector<std::wstring> preservedSections;
        if (std::filesystem::exists(configPath))
        {
            std::wifstream in(configPath);
            if (in.is_open())
            {
                std::wstring line;
                bool skippingGeneral = false;
                while (std::getline(in, line))
                {
                    if (line.find(L"[General]") == 0)
                    {
                        skippingGeneral = true;
                        continue;
                    }
                    if (skippingGeneral)
                    {
                        if (!line.empty() && line.front() == L'[' && line.back() == L']')
                        {
                            skippingGeneral = false;
                            preservedSections.push_back(line);
                        }
                        continue;
                    }
                    preservedSections.push_back(line);
                }
            }
        }

        std::wofstream file(configPath, std::ios::trunc);
        if (file.is_open())
        {
            file << L"[General]\n";
            file << L"Language=" << (m_language == AppLanguage::Turkish ? L"tr" : L"en") << L"\n";
            file << L"AutoStart=" << (m_autoStart ? L"1" : L"0") << L"\n";
            file << L"Preset=" << static_cast<int>(m_preset) << L"\n";
            file << L"PingCountry=" << m_pingCountry << L"\n";
            for (auto const& s : preservedSections)
            {
                file << s << L"\n";
            }
        }
    }
}
