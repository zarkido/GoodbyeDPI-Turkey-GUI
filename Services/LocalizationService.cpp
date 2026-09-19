#include "pch.h"
#include "LocalizationService.h"
#include "SettingsManager.h"

namespace GoodByDpi_App::Services
{
    LocalizationService& LocalizationService::Instance()
    {
        static LocalizationService s_instance;
        return s_instance;
    }

    LocalizationService::LocalizationService()
    {
        InitializeDictionaries();
    }

    void LocalizationService::Initialize()
    {
    }

    AppLanguage LocalizationService::GetCurrentLanguage() const
    {
        return SettingsManager::Instance().GetLanguage();
    }

    void LocalizationService::SetLanguage(AppLanguage language)
    {
        if (SettingsManager::Instance().GetLanguage() != language)
        {
            SettingsManager::Instance().SetLanguage(language);
            for (auto const& callback : m_callbacks)
            {
                if (callback)
                {
                    callback(language);
                }
            }
        }
    }

    std::wstring LocalizationService::GetString(std::wstring_view key) const
    {
        std::wstring keyStr(key);
        if (GetCurrentLanguage() == AppLanguage::Turkish)
        {
            auto it = m_stringsTr.find(keyStr);
            if (it != m_stringsTr.end())
            {
                return it->second;
            }
        }

        auto itFallback = m_stringsEn.find(keyStr);
        if (itFallback != m_stringsEn.end())
        {
            return itFallback->second;
        }

        return keyStr;
    }

    void LocalizationService::RegisterLanguageChangedCallback(std::function<void(AppLanguage)> callback)
    {
        m_callbacks.push_back(callback);
    }

    void LocalizationService::InitializeDictionaries()
    {
        m_stringsEn[L"AppTitle"] = L"GoodByDpi";
        m_stringsEn[L"SettingsButton"] = L"Settings";
        m_stringsEn[L"TitleBarMinimize"] = L"Minimize";
        m_stringsEn[L"TitleBarClose"] = L"Close";
        m_stringsEn[L"StatusDisabled"] = L"DISABLED";
        m_stringsEn[L"StatusActive"] = L"ACTIVE";
        m_stringsEn[L"SettingsTitle"] = L"Settings";
        m_stringsEn[L"SettingsLanguage"] = L"Language / Dil";
        m_stringsEn[L"SettingsLanguageDesc"] = L"Select application interface language";
        m_stringsEn[L"SettingsAutoStart"] = L"Start with Windows";
        m_stringsEn[L"SettingsAutoStartDesc"] = L"Run automatically in the background on system startup";
        m_stringsEn[L"CloseDialogTitle"] = L"Exit Application";
        m_stringsEn[L"CloseDialogMessage"] = L"Do you want to close the application completely, or minimize it to the system tray to keep running in the background?";
        m_stringsEn[L"CloseDialogMinimizeTray"] = L"Minimize to Tray";
        m_stringsEn[L"CloseDialogExit"] = L"Exit Completely";
        m_stringsEn[L"CloseDialogCancel"] = L"Cancel";
        m_stringsEn[L"TrayOpen"] = L"Open";
        m_stringsEn[L"TrayStart"] = L"Start";
        m_stringsEn[L"TrayStop"] = L"Stop";
        m_stringsEn[L"TrayExit"] = L"Exit";
        m_stringsEn[L"TrayTipActive"] = L"GoodByDpi - Active";
        m_stringsEn[L"TrayTipInactive"] = L"GoodByDpi - Disabled";
        m_stringsEn[L"LogButton"] = L"LOGS";
        m_stringsEn[L"LogTitle"] = L"CONNECTION LOGS";
        m_stringsEn[L"TrayNotificationTitle"] = L"GoodByDpi";
        m_stringsEn[L"TrayNotificationBody"] = L"The application continues running in the background. Click the tray icon to restore.";
        m_stringsEn[L"Preset_0"] = L"Turkey Default / Turk Telekom (Mode 5 + Yandex DNS)";
        m_stringsEn[L"Preset_1"] = L"Turkcell Superonline (Alt 1 - TTL 3)";
        m_stringsEn[L"Preset_2"] = L"Turkcell Superonline (Alt 2 - Mode 5)";
        m_stringsEn[L"Preset_3"] = L"Turkcell Superonline (Alt 3 - TTL 3 + DNS)";
        m_stringsEn[L"Preset_4"] = L"Turkcell Superonline (Alt 4 - Mode 5 + DNS)";
        m_stringsEn[L"Preset_5"] = L"Vodafone (Alt 5 - Mode 9 + DNS)";
        m_stringsEn[L"Preset_6"] = L"Vodafone (Alt 6 - Mode 9 Aggressive)";
        m_stringsEn[L"SettingsIspPreset"] = L"ISP Profile & Mode";
        m_stringsEn[L"SettingsIspPresetDesc"] = L"Select the bypass configuration for your ISP";
        m_stringsEn[L"SettingsPingCountry"] = L"Secondary Ping Country";
        m_stringsEn[L"SettingsPingCountryDesc"] = L"Select country to measure in the footer";
        m_stringsEn[L"SettingsWhitelistTitle"] = L"Exception List (Whitelist)";
        m_stringsEn[L"SettingsWhitelistDesc"] = L"Domains or IPs exempt from DPI bypass";
        m_stringsEn[L"SettingsWhitelistAddBtn"] = L"Add";
        m_stringsEn[L"SettingsWhitelistPlaceholder"] = L"e.g. youtube.com or 1.1.1.1";
        m_stringsEn[L"SettingsWhitelistEmpty"] = L"No exceptions added yet";
        m_stringsEn[L"LogStatusProcessed"] = L"DPI Applied";
        m_stringsEn[L"LogStatusBypassed"] = L"Whitelist";
        m_stringsEn[L"LogAddWhitelist"] = L"+ Whitelist";
        m_stringsEn[L"LogRemoveWhitelist"] = L"- Remove";

        m_stringsTr[L"AppTitle"] = L"GoodByDpi";
        m_stringsTr[L"SettingsButton"] = L"Ayarlar";
        m_stringsTr[L"TitleBarMinimize"] = L"Alta Al";
        m_stringsTr[L"TitleBarClose"] = L"Kapat";
        m_stringsTr[L"StatusDisabled"] = L"DEVRE DIŞI";
        m_stringsTr[L"StatusActive"] = L"AKTİF";
        m_stringsTr[L"SettingsTitle"] = L"Ayarlar";
        m_stringsTr[L"SettingsLanguage"] = L"Dil / Language";
        m_stringsTr[L"SettingsLanguageDesc"] = L"Uygulama arayüzü için dil seçin";
        m_stringsTr[L"SettingsAutoStart"] = L"Windows ile Birlikte Başlat";
        m_stringsTr[L"SettingsAutoStartDesc"] = L"Bilgisayar açıldığında arka planda otomatik çalışsın";
        m_stringsTr[L"CloseDialogTitle"] = L"Uygulamadan Çıkış";
        m_stringsTr[L"CloseDialogMessage"] = L"Uygulamayı tamamen kapatmak mı istiyorsunuz, yoksa arka planda çalışmayı sürdürmek için sistem tepsisine (Tray) mi küçültmek istersiniz?";
        m_stringsTr[L"CloseDialogMinimizeTray"] = L"Tepsiye Küçült";
        m_stringsTr[L"CloseDialogExit"] = L"Tamamen Kapat";
        m_stringsTr[L"CloseDialogCancel"] = L"İptal";
        m_stringsTr[L"TrayOpen"] = L"Aç";
        m_stringsTr[L"TrayStart"] = L"Başlat";
        m_stringsTr[L"TrayStop"] = L"Durdur";
        m_stringsTr[L"TrayExit"] = L"Çıkış";
        m_stringsTr[L"TrayTipActive"] = L"GoodByDpi - Aktif";
        m_stringsTr[L"TrayTipInactive"] = L"GoodByDpi - Devre Dışı";
        m_stringsTr[L"LogButton"] = L"LOG";
        m_stringsTr[L"LogTitle"] = L"BAĞLANTI GÜNLÜĞÜ";
        m_stringsTr[L"TrayNotificationTitle"] = L"GoodByDpi";
        m_stringsTr[L"TrayNotificationBody"] = L"Uygulama arka planda çalışmaya devam ediyor. Açmak için simgeye tıklayabilirsiniz.";
        m_stringsTr[L"Preset_0"] = L"Türkiye Genel / Türk Telekom (Mod 5 + Yandex DNS)";
        m_stringsTr[L"Preset_1"] = L"Turkcell Superonline (Alternatif 1 - TTL 3)";
        m_stringsTr[L"Preset_2"] = L"Turkcell Superonline (Alternatif 2 - Mod 5)";
        m_stringsTr[L"Preset_3"] = L"Turkcell Superonline (Alternatif 3 - TTL 3 + DNS)";
        m_stringsTr[L"Preset_4"] = L"Turkcell Superonline (Alternatif 4 - Mod 5 + DNS)";
        m_stringsTr[L"Preset_5"] = L"Vodafone (Alternatif 5 - Mod 9 + DNS)";
        m_stringsTr[L"Preset_6"] = L"Vodafone (Alternatif 6 - Mod 9 Agresif)";
        m_stringsTr[L"SettingsIspPreset"] = L"İSS Profili ve Modu";
        m_stringsTr[L"SettingsIspPresetDesc"] = L"Sağlayıcınıza en uygun baypas modunu seçin";
        m_stringsTr[L"SettingsPingCountry"] = L"İkinci Ping Ülkesi";
        m_stringsTr[L"SettingsPingCountryDesc"] = L"Alt çubukta ölçülecek ülkeyi belirleyin";
        m_stringsTr[L"SettingsWhitelistTitle"] = L"İstisna Listesi (Whitelist)";
        m_stringsTr[L"SettingsWhitelistDesc"] = L"DPI baypasından muaf tutulacak alan adı veya IP'ler";
        m_stringsTr[L"SettingsWhitelistAddBtn"] = L"Ekle";
        m_stringsTr[L"SettingsWhitelistPlaceholder"] = L"Örn: youtube.com veya 1.1.1.1";
        m_stringsTr[L"SettingsWhitelistEmpty"] = L"Henüz eklenmiş istisna yok";
        m_stringsTr[L"LogStatusProcessed"] = L"DPI Uygulandı";
        m_stringsTr[L"LogStatusBypassed"] = L"Whitelist";
        m_stringsTr[L"LogAddWhitelist"] = L"+ Whitelist";
        m_stringsTr[L"LogRemoveWhitelist"] = L"- Kaldır";
    }
}
