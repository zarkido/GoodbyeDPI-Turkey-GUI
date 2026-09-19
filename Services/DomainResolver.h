#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <cstdint>
#include <functional>

namespace GoodByDpi_App::Services
{
    class DomainResolver
    {
    public:
        static DomainResolver& Instance();

        void Start();
        void Stop();

        void AddMapping(std::string const& ip, std::string const& domain);
        std::string GetDomainForIp(std::string const& ip);

        void RegisterResolvedCallback(std::function<void(std::string const& ip, std::string const& domain)> callback);

        static bool ExtractSni(uint8_t const* data, size_t len, std::string& outSni);
        static bool ParseHttpHost(uint8_t const* data, size_t len, std::string& outHost);
        static bool ParseDnsQuestion(uint8_t const* data, size_t len, std::string& outDomain);
        static bool ParseDnsResponse(uint8_t const* data, size_t len, std::string& outDomain, std::vector<std::string>& outIps);

    private:
        DomainResolver();
        ~DomainResolver();
        DomainResolver(DomainResolver const&) = delete;
        DomainResolver& operator=(DomainResolver const&) = delete;

        void WorkerLoop();
        void QueueReverseLookup(std::string const& ip);
        static std::string PerformReverseLookup(std::string const& ip);

        mutable std::shared_mutex m_cacheMutex;
        std::unordered_map<std::string, std::string> m_ipToDomain;

        std::mutex m_queueMutex;
        std::condition_variable m_queueCv;
        std::deque<std::string> m_lookupQueue;
        std::unordered_set<std::string> m_queuedIps;
        std::atomic<bool> m_running{ false };
        std::thread m_workerThread;

        std::mutex m_cbMutex;
        std::vector<std::function<void(std::string const&, std::string const&)>> m_callbacks;
    };
}
