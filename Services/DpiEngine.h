#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <memory>
#include <cstring>
#include "WinDivertDefs.h"

namespace GoodByDpi_App::Services
{
    enum class DpiPreset
    {
        TurkTelekomDefault = 0,
        SuperonlineAlt1 = 1,
        SuperonlineAlt2 = 2,
        SuperonlineAlt3 = 3,
        SuperonlineAlt4 = 4,
        VodafoneAlt5 = 5,
        VodafoneAlt6 = 6
    };

    struct DnsConntrackKey
    {
        bool isIpv6{ false };
        uint8_t clientIp[16]{ 0 };
        uint16_t clientPort{ 0 };

        bool operator==(DnsConntrackKey const& o) const
        {
            if (isIpv6 != o.isIpv6 || clientPort != o.clientPort) return false;
            size_t len = isIpv6 ? 16 : 4;
            return memcmp(clientIp, o.clientIp, len) == 0;
        }
    };

    struct DnsConntrackKeyHash
    {
        size_t operator()(DnsConntrackKey const& k) const noexcept
        {
            size_t h = std::hash<uint16_t>{}(k.clientPort);
            h ^= (std::hash<bool>{}(k.isIpv6) << 1);
            size_t len = k.isIpv6 ? 16 : 4;
            for (size_t i = 0; i < len; ++i)
            {
                h ^= (static_cast<size_t>(k.clientIp[i]) << ((i % 4) * 8));
            }
            return h;
        }
    };

    struct DnsConntrackEntry
    {
        uint8_t origDstIp[16]{ 0 };
        uint16_t origDstPort{ 53 };
        std::chrono::steady_clock::time_point timestamp;
    };

    class DpiEngine
    {
    public:
        static DpiEngine& Instance();

        bool Start(DpiPreset preset);
        void Stop();
        bool IsRunning() const;

        DpiPreset GetCurrentPreset() const;
        void SetPreset(DpiPreset preset);

    private:
        DpiEngine();
        ~DpiEngine();
        DpiEngine(DpiEngine const&) = delete;
        DpiEngine& operator=(DpiEngine const&) = delete;

        bool LoadDivertModule();
        void UnloadDivertModule();

        void PacketLoop();
        void CleanupDnsConntrack();
        static void FlushDnsCache();

        HMODULE m_hDivertModule{ nullptr };
        pfnWinDivertOpen m_pfnOpen{ nullptr };
        pfnWinDivertRecv m_pfnRecv{ nullptr };
        pfnWinDivertSend m_pfnSend{ nullptr };
        pfnWinDivertClose m_pfnClose{ nullptr };
        pfnWinDivertSetParam m_pfnSetParam{ nullptr };
        pfnWinDivertHelperParsePacket m_pfnParse{ nullptr };
        pfnWinDivertHelperCalcChecksums m_pfnChecksum{ nullptr };

        std::atomic<bool> m_running{ false };
        std::atomic<HANDLE> m_divertHandle{ INVALID_HANDLE_VALUE };
        std::thread m_workerThread;
        std::mutex m_stateMutex;
        DpiPreset m_preset{ DpiPreset::TurkTelekomDefault };

        std::unordered_map<DnsConntrackKey, DnsConntrackEntry, DnsConntrackKeyHash> m_dnsConntrack;
        std::mutex m_dnsMutex;
    };
}
