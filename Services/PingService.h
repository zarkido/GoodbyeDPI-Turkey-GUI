#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace GoodByDpi_App::Services
{
    class PingService
    {
    public:
        static PingService& Instance();

        void Start();
        void Stop();
        bool IsRunning() const;

        void SetCountry(std::wstring const& countryCode);
        std::wstring GetCountry() const;

        int GetLastEuPing() const;
        int GetLastCountryPing() const;

        void RegisterPingUpdatedCallback(std::function<void(int euMs, int countryMs)> callback);

    private:
        PingService();
        ~PingService();
        PingService(PingService const&) = delete;
        PingService& operator=(PingService const&) = delete;

        void WorkerLoop();
        int MeasurePing(std::string const& ipStr);

        std::atomic<bool> m_running{ false };
        std::atomic<bool> m_enabled{ false };
        std::atomic<bool> m_forcePing{ false };
        std::thread m_thread;
        mutable std::mutex m_mutex;
        std::condition_variable m_cv;

        std::wstring m_countryCode{ L"TR" };
        std::atomic<int> m_lastEuPing{ -1 };
        std::atomic<int> m_lastCountryPing{ -1 };

        std::vector<std::function<void(int, int)>> m_callbacks;
    };
}
