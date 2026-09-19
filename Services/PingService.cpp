#include "pch.h"
#include "PingService.h"
#include <winsock2.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")

namespace GoodByDpi_App::Services
{
    PingService& PingService::Instance()
    {
        static PingService s_instance;
        return s_instance;
    }

    PingService::PingService()
    {
        m_running = true;
        m_enabled = false;
        m_thread = std::thread(&PingService::WorkerLoop, this);
    }

    PingService::~PingService()
    {
        m_running = false;
        m_enabled = false;
        m_cv.notify_all();
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void PingService::Start()
    {
        m_enabled = true;
        m_cv.notify_all();
    }

    void PingService::Stop()
    {
        m_enabled = false;
        m_cv.notify_all();
    }

    bool PingService::IsRunning() const
    {
        return m_enabled;
    }

    void PingService::SetCountry(std::wstring const& countryCode)
    {
        {
            std::lock_guard lock(m_mutex);
            m_countryCode = countryCode;
            m_forcePing = true;
        }
        m_cv.notify_all();
    }

    std::wstring PingService::GetCountry() const
    {
        std::lock_guard lock(m_mutex);
        return m_countryCode;
    }

    int PingService::GetLastEuPing() const
    {
        return m_lastEuPing;
    }

    int PingService::GetLastCountryPing() const
    {
        return m_lastCountryPing;
    }

    void PingService::RegisterPingUpdatedCallback(std::function<void(int euMs, int countryMs)> callback)
    {
        std::lock_guard lock(m_mutex);
        m_callbacks.push_back(std::move(callback));
    }

    int PingService::MeasurePing(std::string const& ipStr)
    {
        HANDLE hIcmpFile = ::IcmpCreateFile();
        if (hIcmpFile == INVALID_HANDLE_VALUE)
        {
            return -1;
        }

        IN_ADDR addr {};
        if (::inet_pton(AF_INET, ipStr.c_str(), &addr) != 1)
        {
            ::IcmpCloseHandle(hIcmpFile);
            return -1;
        }

        char sendData[32] = "GBDPI_PROBE_PING";
        DWORD replySize = 256;
        std::vector<BYTE> replyBuffer(replySize, 0);

        DWORD replies = ::IcmpSendEcho(
            hIcmpFile,
            addr.S_un.S_addr,
            sendData,
            static_cast<WORD>(sizeof(sendData)),
            nullptr,
            replyBuffer.data(),
            replySize,
            1200
        );

        int result = -1;
        if (replies > 0)
        {
            auto pEchoReply = reinterpret_cast<PICMP_ECHO_REPLY>(replyBuffer.data());
            if (pEchoReply->Status == IP_SUCCESS)
            {
                result = static_cast<int>(pEchoReply->RoundTripTime);
            }
        }

        ::IcmpCloseHandle(hIcmpFile);
        return result;
    }

    void PingService::WorkerLoop()
    {
        while (m_running)
        {
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [this] {
                    return !m_running || m_enabled;
                });
            }

            if (!m_running) break;

            std::wstring country;
            {
                std::lock_guard lock(m_mutex);
                country = m_countryCode;
                m_forcePing = false;
            }

            std::string euTarget = "213.133.98.98";
            std::string countryTarget = "195.175.39.39";
            if (country == L"UK")
            {
                countryTarget = "8.8.4.4";
            }
            else if (country == L"US")
            {
                countryTarget = "1.1.1.1";
            }

            int euMs = MeasurePing(euTarget);
            int countryMs = MeasurePing(countryTarget);

            m_lastEuPing = euMs;
            m_lastCountryPing = countryMs;

            std::vector<std::function<void(int, int)>> cbs;
            {
                std::lock_guard lk(m_mutex);
                cbs = m_callbacks;
            }

            for (auto const& cb : cbs)
            {
                if (cb) cb(euMs, countryMs);
            }

            {
                std::unique_lock waitLock(m_mutex);
                m_cv.wait_for(waitLock, std::chrono::seconds(5), [this] {
                    return !m_running || !m_enabled || m_forcePing.load();
                });
            }
        }
    }
}
