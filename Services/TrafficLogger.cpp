#include "pch.h"
#include "TrafficLogger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace GoodByDpi_App::Services
{
    TrafficLogger& TrafficLogger::Instance()
    {
        static TrafficLogger s_instance;
        return s_instance;
    }

    void TrafficLogger::SetEnabled(bool enabled)
    {
        m_enabled.store(enabled);
        if (!enabled)
        {
            Clear();
        }
    }

    bool TrafficLogger::IsEnabled() const
    {
        return m_enabled.load();
    }

    void TrafficLogger::RegisterLogAddedCallback(std::function<void(ConnectionLogItem const&)> callback)
    {
        std::lock_guard lock(m_mutex);
        m_callbacks.push_back(std::move(callback));
    }

    void TrafficLogger::RegisterLogUpdatedCallback(std::function<void(uint64_t, std::wstring const&)> callback)
    {
        std::lock_guard lock(m_mutex);
        m_updateCallbacks.push_back(std::move(callback));
    }

    void TrafficLogger::UpdateTargetForIp(std::wstring const& ip, std::wstring const& newTarget)
    {
        if (!m_enabled.load()) return;
        if (ip.empty() || newTarget.empty() || ip == newTarget) return;

        std::vector<std::pair<uint64_t, std::wstring>> updatedItems;
        std::vector<std::function<void(uint64_t, std::wstring const&)>> cbs;
        {
            std::lock_guard lock(m_mutex);
            for (auto& item : m_items)
            {
                if (item.ip == ip && item.target == ip)
                {
                    item.target = newTarget;
                    updatedItems.push_back({ item.id, newTarget });
                }
            }
            cbs = m_updateCallbacks;
        }

        for (auto const& up : updatedItems)
        {
            for (auto const& cb : cbs)
            {
                if (cb) cb(up.first, up.second);
            }
        }
    }

    void TrafficLogger::AddLog(
        std::wstring const& target,
        std::wstring const& ip,
        uint16_t port,
        std::wstring const& protocol,
        bool isDpiProcessed
    )
    {
        if (!m_enabled.load()) return;
        if (target.empty()) return;

        auto nowSteady = std::chrono::steady_clock::now();
        auto now = std::chrono::system_clock::now();
        auto inTime = std::chrono::system_clock::to_time_t(now);
        struct tm tmBuf {};
        localtime_s(&tmBuf, &inTime);

        std::wstringstream wss;
        wss << std::setfill(L'0')
            << std::setw(2) << tmBuf.tm_hour << L":"
            << std::setw(2) << tmBuf.tm_min << L":"
            << std::setw(2) << tmBuf.tm_sec;

        ConnectionLogItem item;
        std::vector<std::function<void(ConnectionLogItem const&)>> cbs;

        std::wstring debounceKey = target + L"|" + std::to_wstring(port);

        {
            std::lock_guard lock(m_mutex);

            auto it = m_recentTargets.find(debounceKey);
            if (it != m_recentTargets.end())
            {
                auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(nowSteady - it->second).count();
                if (diff < 2000)
                {
                    return;
                }
            }
            m_recentTargets[debounceKey] = nowSteady;

            if (m_recentTargets.size() > 300)
            {
                for (auto rIt = m_recentTargets.begin(); rIt != m_recentTargets.end();)
                {
                    auto diff = std::chrono::duration_cast<std::chrono::seconds>(nowSteady - rIt->second).count();
                    if (diff > 10)
                    {
                        rIt = m_recentTargets.erase(rIt);
                    }
                    else
                    {
                        ++rIt;
                    }
                }
            }

            item.id = m_nextId++;
            item.target = target;
            item.ip = ip;
            item.port = port;
            item.protocol = protocol;
            item.isDpiProcessed = isDpiProcessed;
            item.timeString = wss.str();

            if (m_items.size() >= 100)
            {
                m_items.pop_front();
            }
            m_items.push_back(item);
            cbs = m_callbacks;
        }

        for (auto const& cb : cbs)
        {
            if (cb) cb(item);
        }
    }

    std::vector<ConnectionLogItem> TrafficLogger::GetLogs() const
    {
        std::lock_guard lock(m_mutex);
        return std::vector<ConnectionLogItem>(m_items.begin(), m_items.end());
    }

    void TrafficLogger::Clear()
    {
        std::lock_guard lock(m_mutex);
        m_items.clear();
        m_recentTargets.clear();
    }
}
