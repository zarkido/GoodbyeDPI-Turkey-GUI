#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <functional>
#include <vector>

namespace GoodByDpi_App::Services
{
    enum class AppLanguage
    {
        English = 0,
        Turkish = 1
    };

    class LocalizationService
    {
    public:
        static LocalizationService& Instance();

        void Initialize();
        AppLanguage GetCurrentLanguage() const;
        void SetLanguage(AppLanguage language);

        std::wstring GetString(std::wstring_view key) const;

        void RegisterLanguageChangedCallback(std::function<void(AppLanguage)> callback);

    private:
        LocalizationService();
        void InitializeDictionaries();

        std::unordered_map<std::wstring, std::wstring> m_stringsEn;
        std::unordered_map<std::wstring, std::wstring> m_stringsTr;
        std::vector<std::function<void(AppLanguage)>> m_callbacks;
    };
}
