#include "pch.h"
#include "WhitelistManager.h"
#include "Utils/StringUtils.h"
#include "Utils/PathUtils.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace GoodByDpi_App::Services
{
    using Utils::ToLowerUtf8;
    using Utils::ToLowerAscii;

    WhitelistManager& WhitelistManager::Instance()
    {
        static WhitelistManager s_instance;
        return s_instance;
    }

    void WhitelistManager::RegisterChangedCallback(std::function<void()> callback)
    {
        std::unique_lock lock(m_mutex);
        m_callbacks.push_back(std::move(callback));
    }

    void WhitelistManager::NotifyChanged()
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

    void WhitelistManager::RebuildLookupSetsUnsafe()
    {
        m_domainsLower.clear();
        m_ips.clear();

        for (auto const& item : m_items)
        {
            std::string s = ToLowerUtf8(item);
            if (s.empty()) continue;

            bool isIp = true;
            for (char c : s)
            {
                if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.' && c != ':')
                {
                    isIp = false;
                    break;
                }
            }

            if (isIp)
            {
                m_ips.insert(s);
            }
            else
            {
                m_domainsLower.insert(s);
            }
        }
    }

    void WhitelistManager::LoadFromSettings(std::wstring const& configPath)
    {
        std::unique_lock lock(m_mutex);
        m_items.clear();

        if (std::filesystem::exists(configPath))
        {
            std::wifstream file(configPath);
            if (file.is_open())
            {
                std::wstring line;
                bool inSection = false;
                while (std::getline(file, line))
                {
                    while (!line.empty() && (line.back() == L'\r' || line.back() == L' ' || line.back() == L'\t'))
                    {
                        line.pop_back();
                    }
                    size_t start = 0;
                    while (start < line.size() && (line[start] == L' ' || line[start] == L'\t'))
                    {
                        start++;
                    }
                    if (start > 0) line = line.substr(start);

                    if (line == L"[Whitelist]")
                    {
                        inSection = true;
                        continue;
                    }
                    else if (!line.empty() && line.front() == L'[' && line.back() == L']')
                    {
                        inSection = false;
                        continue;
                    }

                    if (inSection && !line.empty())
                    {
                        size_t eq = line.find(L'=');
                        std::wstring val = (eq != std::wstring::npos) ? line.substr(eq + 1) : line;
                        if (!val.empty() && std::find(m_items.begin(), m_items.end(), val) == m_items.end())
                        {
                            m_items.push_back(val);
                        }
                    }
                }
            }
        }

        RebuildLookupSetsUnsafe();
    }

    void WhitelistManager::SaveToSettings(std::wstring const& configPath) const
    {
        std::shared_lock lock(m_mutex);

        std::vector<std::wstring> existingLines;
        if (std::filesystem::exists(configPath))
        {
            std::wifstream in(configPath);
            if (in.is_open())
            {
                std::wstring line;
                bool skippingOld = false;
                while (std::getline(in, line))
                {
                    if (line.find(L"[Whitelist]") == 0)
                    {
                        skippingOld = true;
                        continue;
                    }
                    if (skippingOld)
                    {
                        if (!line.empty() && line.front() == L'[' && line.back() == L']')
                        {
                            skippingOld = false;
                            existingLines.push_back(line);
                        }
                        continue;
                    }
                    existingLines.push_back(line);
                }
            }
        }

        std::wofstream out(configPath, std::ios::trunc);
        if (out.is_open())
        {
            for (auto const& line : existingLines)
            {
                out << line << L"\n";
            }
            out << L"\n[Whitelist]\n";
            for (size_t i = 0; i < m_items.size(); ++i)
            {
                out << L"Item" << i << L"=" << m_items[i] << L"\n";
            }
        }
    }

    bool WhitelistManager::IsWhitelisted(std::string const& domain, std::string const& ipStr) const
    {
        std::shared_lock lock(m_mutex);

        if (!ipStr.empty())
        {
            std::string ipLower = ToLowerAscii(ipStr);
            if (m_ips.find(ipLower) != m_ips.end())
            {
                return true;
            }
        }

        if (!domain.empty())
        {
            std::string domLower = ToLowerAscii(domain);
            if (m_domainsLower.find(domLower) != m_domainsLower.end())
            {
                return true;
            }

            size_t dotPos = domLower.find('.');
            while (dotPos != std::string::npos && dotPos + 1 < domLower.size())
            {
                std::string parent = domLower.substr(dotPos + 1);
                if (m_domainsLower.find(parent) != m_domainsLower.end())
                {
                    return true;
                }
                dotPos = domLower.find('.', dotPos + 1);
            }
        }

        return false;
    }

    bool WhitelistManager::AddItem(std::wstring const& item, std::wstring const& configPath)
    {
        if (item.empty()) return false;
        {
            std::unique_lock lock(m_mutex);
            if (std::find(m_items.begin(), m_items.end(), item) != m_items.end())
            {
                return false;
            }
            m_items.push_back(item);
            RebuildLookupSetsUnsafe();
        }
        SaveToSettings(configPath);
        NotifyChanged();
        return true;
    }

    bool WhitelistManager::RemoveItem(std::wstring const& item, std::wstring const& configPath)
    {
        {
            std::unique_lock lock(m_mutex);
            auto it = std::find(m_items.begin(), m_items.end(), item);
            if (it == m_items.end())
            {
                return false;
            }
            m_items.erase(it);
            RebuildLookupSetsUnsafe();
        }
        SaveToSettings(configPath);
        NotifyChanged();
        return true;
    }

    std::vector<std::wstring> WhitelistManager::GetItems() const
    {
        std::shared_lock lock(m_mutex);
        return m_items;
    }
}
