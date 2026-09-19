#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <shared_mutex>
#include <functional>

namespace GoodByDpi_App::Services
{
    class WhitelistManager
    {
    public:
        static WhitelistManager& Instance();

        void LoadFromSettings(std::wstring const& configPath);
        void SaveToSettings(std::wstring const& configPath) const;

        bool IsWhitelisted(std::string const& domain, std::string const& ipStr) const;
        bool AddItem(std::wstring const& item, std::wstring const& configPath);
        bool RemoveItem(std::wstring const& item, std::wstring const& configPath);
        std::vector<std::wstring> GetItems() const;

        void RegisterChangedCallback(std::function<void()> callback);

    private:
        WhitelistManager() = default;
        ~WhitelistManager() = default;
        WhitelistManager(WhitelistManager const&) = delete;
        WhitelistManager& operator=(WhitelistManager const&) = delete;

        mutable std::shared_mutex m_mutex;
        std::vector<std::wstring> m_items;
        std::unordered_set<std::string> m_domainsLower;
        std::unordered_set<std::string> m_ips;
        std::vector<std::function<void()>> m_callbacks;

        void RebuildLookupSetsUnsafe();
        void NotifyChanged();
    };
}
