#pragma once

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <atomic>

namespace GoodByDpi_App::Services
{
    struct ConnectionLogItem
    {
        uint64_t id;
        std::wstring target;
        std::wstring ip;
        uint16_t port;
        std::wstring protocol;
        bool isDpiProcessed;
        std::wstring timeString;
    };

    class TrafficLogger
    {
    public:
        static TrafficLogger& Instance();

        void SetEnabled(bool enabled);
        bool IsEnabled() const;

        void AddLog(
            std::wstring const& target,
            std::wstring const& ip,
            uint16_t port,
            std::wstring const& protocol,
            bool isDpiProcessed
        );

        void UpdateTargetForIp(std::wstring const& ip, std::wstring const& newTarget);

        std::vector<ConnectionLogItem> GetLogs() const;
        void Clear();

        void RegisterLogAddedCallback(std::function<void(ConnectionLogItem const&)> callback);
        void RegisterLogUpdatedCallback(std::function<void(uint64_t id, std::wstring const& newTarget)> callback);

    private:
        TrafficLogger() = default;
        ~TrafficLogger() = default;
        TrafficLogger(TrafficLogger const&) = delete;
        TrafficLogger& operator=(TrafficLogger const&) = delete;

        std::atomic<bool> m_enabled{ true };
        mutable std::mutex m_mutex;
        std::deque<ConnectionLogItem> m_items;
        std::unordered_map<std::wstring, std::chrono::steady_clock::time_point> m_recentTargets;
        uint64_t m_nextId{ 1 };
        std::vector<std::function<void(ConnectionLogItem const&)>> m_callbacks;
        std::vector<std::function<void(uint64_t, std::wstring const&)>> m_updateCallbacks;
    };
}
