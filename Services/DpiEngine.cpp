#include "pch.h"
#include "DpiEngine.h"
#include "WhitelistManager.h"
#include "TrafficLogger.h"
#include "CrashLogger.h"
#include "DomainResolver.h"
#include "Utils/StringUtils.h"
#include "Utils/ScopedHandle.h"
#include "Utils/PathUtils.h"
#include <ws2tcpip.h>
#include <winsvc.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <chrono>

#pragma comment(lib, "Advapi32.lib")

namespace GoodByDpi_App::Services
{
    using Utils::Utf8ToWide;

    struct PresetConfig
    {
        bool doDnsRedir{ true };
        uint8_t fakeTtl{ 5 };
        bool fakeWrongSeq{ false };
        bool fakeWrongChecksum{ false };
        bool doReverseFrag{ true };
        UINT fragSize{ 2 };
        bool blockQuic{ false };
    };

    static PresetConfig GetPresetConfig(DpiPreset preset)
    {
        PresetConfig cfg;
        switch (preset)
        {
        case DpiPreset::TurkTelekomDefault:
            cfg.doDnsRedir = true;
            cfg.fakeTtl = 5;
            cfg.fakeWrongSeq = false;
            cfg.fakeWrongChecksum = false;
            cfg.doReverseFrag = true;
            cfg.fragSize = 2;
            cfg.blockQuic = false;
            break;
        case DpiPreset::SuperonlineAlt1:
            cfg.doDnsRedir = false;
            cfg.fakeTtl = 3;
            cfg.fakeWrongSeq = false;
            cfg.fakeWrongChecksum = false;
            cfg.doReverseFrag = false;
            cfg.fragSize = 0;
            cfg.blockQuic = false;
            break;
        case DpiPreset::SuperonlineAlt2:
            cfg.doDnsRedir = false;
            cfg.fakeTtl = 5;
            cfg.fakeWrongSeq = false;
            cfg.fakeWrongChecksum = false;
            cfg.doReverseFrag = true;
            cfg.fragSize = 2;
            cfg.blockQuic = false;
            break;
        case DpiPreset::SuperonlineAlt3:
            cfg.doDnsRedir = true;
            cfg.fakeTtl = 3;
            cfg.fakeWrongSeq = false;
            cfg.fakeWrongChecksum = false;
            cfg.doReverseFrag = false;
            cfg.fragSize = 0;
            cfg.blockQuic = false;
            break;
        case DpiPreset::SuperonlineAlt4:
            cfg.doDnsRedir = true;
            cfg.fakeTtl = 5;
            cfg.fakeWrongSeq = false;
            cfg.fakeWrongChecksum = false;
            cfg.doReverseFrag = true;
            cfg.fragSize = 2;
            cfg.blockQuic = false;
            break;
        case DpiPreset::VodafoneAlt5:
            cfg.doDnsRedir = true;
            cfg.fakeTtl = 0;
            cfg.fakeWrongSeq = true;
            cfg.fakeWrongChecksum = true;
            cfg.doReverseFrag = true;
            cfg.fragSize = 2;
            cfg.blockQuic = true;
            break;
        case DpiPreset::VodafoneAlt6:
            cfg.doDnsRedir = false;
            cfg.fakeTtl = 0;
            cfg.fakeWrongSeq = true;
            cfg.fakeWrongChecksum = true;
            cfg.doReverseFrag = true;
            cfg.fragSize = 2;
            cfg.blockQuic = true;
            break;
        }
        return cfg;
    }

    static const unsigned char s_fakeHttp[] =
        "GET / HTTP/1.1\r\n"
        "Host: www.w3.org\r\n"
        "User-Agent: curl/7.65.3\r\n"
        "Accept: \x2a\x2f\x2a\r\n"
        "Accept-Encoding: deflate, gzip, br\r\n\r\n";

    static const unsigned char s_fakeHttps[] = {
        0x16, 0x03, 0x01, 0x02, 0x00, 0x01, 0x00, 0x01, 0xfc, 0x03, 0x03, 0x9a, 0x8f, 0xa7, 0x6a, 0x5d,
        0x57, 0xf3, 0x62, 0x19, 0xbe, 0x46, 0x82, 0x45, 0xe2, 0x59, 0x5c, 0xb4, 0x48, 0x31, 0x12, 0x15,
        0x14, 0x79, 0x2c, 0xaa, 0xcd, 0xea, 0xda, 0xf0, 0xe1, 0xfd, 0xbb, 0x20, 0xf4, 0x83, 0x2a, 0x94,
        0xf1, 0x48, 0x3b, 0x9d, 0xb6, 0x74, 0xba, 0x3c, 0x81, 0x63, 0xbc, 0x18, 0xcc, 0x14, 0x45, 0x57,
        0x6c, 0x80, 0xf9, 0x25, 0xcf, 0x9c, 0x86, 0x60, 0x50, 0x31, 0x2e, 0xe9, 0x00, 0x22, 0x13, 0x01,
        0x13, 0x03, 0x13, 0x02, 0xc0, 0x2b, 0xc0, 0x2f, 0xcc, 0xa9, 0xcc, 0xa8, 0xc0, 0x2c, 0xc0, 0x30,
        0xc0, 0x0a, 0xc0, 0x09, 0xc0, 0x13, 0xc0, 0x14, 0x00, 0x33, 0x00, 0x39, 0x00, 0x2f, 0x00, 0x35,
        0x01, 0x00, 0x01, 0x91, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x0d, 0x00, 0x00, 0x0a, 0x77, 0x77, 0x77,
        0x2e, 0x77, 0x33, 0x2e, 0x6f, 0x72, 0x67, 0x00, 0x17, 0x00, 0x00, 0xff, 0x01, 0x00, 0x01, 0x00,
        0x00, 0x0a, 0x00, 0x0e, 0x00, 0x0c, 0x00, 0x1d, 0x00, 0x17, 0x00, 0x18, 0x00, 0x19, 0x01, 0x00,
        0x01, 0x01, 0x00, 0x0b, 0x00, 0x02, 0x01, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x10, 0x00, 0x0e,
        0x00, 0x0c, 0x02, 0x68, 0x32, 0x08, 0x68, 0x74, 0x74, 0x70, 0x2f, 0x31, 0x2e, 0x31, 0x00, 0x05,
        0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x00, 0x6b, 0x00, 0x69, 0x00, 0x1d, 0x00,
        0x20, 0xb0, 0xe4, 0xda, 0x34, 0xb4, 0x29, 0x8d, 0xd3, 0x5c, 0x70, 0xd3, 0xbe, 0xe8, 0xa7, 0x2a,
        0x6b, 0xe4, 0x11, 0x19, 0x8b, 0x18, 0x9d, 0x83, 0x9a, 0x49, 0x7c, 0x83, 0x7f, 0xa9, 0x03, 0x8c,
        0x3c, 0x00, 0x17, 0x00, 0x41, 0x04, 0x4c, 0x04, 0xa4, 0x71, 0x4c, 0x49, 0x75, 0x55, 0xd1, 0x18,
        0x1e, 0x22, 0x62, 0x19, 0x53, 0x00, 0xde, 0x74, 0x2f, 0xb3, 0xde, 0x13, 0x54, 0xe6, 0x78, 0x07,
        0x94, 0x55, 0x0e, 0xb2, 0x6c, 0xb0, 0x03, 0xee, 0x79, 0xa9, 0x96, 0x1e, 0x0e, 0x98, 0x17, 0x78,
        0x24, 0x44, 0x0c, 0x88, 0x80, 0x06, 0x8b, 0xd4, 0x80, 0xbf, 0x67, 0x7c, 0x37, 0x6a, 0x5b, 0x46,
        0x4c, 0xa7, 0x98, 0x6f, 0xb9, 0x22, 0x00, 0x2b, 0x00, 0x09, 0x08, 0x03, 0x04, 0x03, 0x03, 0x03,
        0x02, 0x03, 0x01, 0x00, 0x0d, 0x00, 0x18, 0x00, 0x16, 0x04, 0x03, 0x05, 0x03, 0x06, 0x03, 0x08,
        0x04, 0x08, 0x05, 0x08, 0x06, 0x04, 0x01, 0x05, 0x01, 0x06, 0x01, 0x02, 0x03, 0x02, 0x01, 0x00,
        0x2d, 0x00, 0x02, 0x01, 0x01, 0x00, 0x1c, 0x00, 0x02, 0x40, 0x01, 0x00, 0x15, 0x00, 0x96, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00
    };

    static void SendFakeData(
        HANDLE divertHandle,
        pfnWinDivertSend pfnSend,
        pfnWinDivertHelperCalcChecksums pfnChecksum,
        WINDIVERT_ADDRESS addr,
        uint8_t const* origPacket,
        UINT origPacketLen,
        void const* packetData,
        UINT packetDataLen,
        PWINDIVERT_IPHDR pIpHdr,
        PWINDIVERT_IPV6HDR pIpV6Hdr,
        PWINDIVERT_TCPHDR pTcpHdr,
        bool isHttps,
        uint8_t setTtl,
        bool setChecksum,
        bool setSeq
    )
    {
        if (!divertHandle || !pfnSend || !pfnChecksum || !origPacket || !packetData || (!pIpHdr && !pIpV6Hdr) || !pTcpHdr) return;

        const unsigned char* fakeData = isHttps ? s_fakeHttps : s_fakeHttp;
        UINT fakeLen = isHttps ? sizeof(s_fakeHttps) : static_cast<UINT>(sizeof(s_fakeHttp) - 1);

        size_t dataOffset = reinterpret_cast<uint8_t const*>(packetData) - origPacket;
        if (dataOffset > origPacketLen) return;

        UINT newPacketLen = static_cast<UINT>(dataOffset) + fakeLen;
        std::vector<uint8_t> fakePkt(newPacketLen);
        memcpy(fakePkt.data(), origPacket, dataOffset);
        memcpy(fakePkt.data() + dataOffset, fakeData, fakeLen);

        if (pIpHdr)
        {
            PWINDIVERT_IPHDR pFakeIp = reinterpret_cast<PWINDIVERT_IPHDR>(fakePkt.data());
            pFakeIp->Length = htons(static_cast<uint16_t>(newPacketLen));
            if (setTtl > 0)
            {
                pFakeIp->TTL = setTtl;
            }
        }
        else if (pIpV6Hdr)
        {
            PWINDIVERT_IPV6HDR pFakeIp6 = reinterpret_cast<PWINDIVERT_IPV6HDR>(fakePkt.data());
            pFakeIp6->Length = htons(static_cast<uint16_t>(newPacketLen - sizeof(WINDIVERT_IPV6HDR)));
            if (setTtl > 0)
            {
                pFakeIp6->HopLimit = setTtl;
            }
        }

        size_t tcpOffset = (reinterpret_cast<uint8_t const*>(pTcpHdr) - origPacket);
        if (tcpOffset + sizeof(WINDIVERT_TCPHDR) <= newPacketLen)
        {
            PWINDIVERT_TCPHDR pFakeTcp = reinterpret_cast<PWINDIVERT_TCPHDR>(fakePkt.data() + tcpOffset);
            if (setSeq)
            {
                pFakeTcp->AckNum = htonl(ntohl(pFakeTcp->AckNum) - 66000);
                pFakeTcp->SeqNum = htonl(ntohl(pFakeTcp->SeqNum) - 10000);
            }
        }

        WINDIVERT_ADDRESS fakeAddr = addr;
        fakeAddr.IPChecksum = 0;
        fakeAddr.TCPChecksum = 0;
        pfnChecksum(fakePkt.data(), newPacketLen, &fakeAddr, 0);

        if (setChecksum)
        {
            if (tcpOffset + sizeof(WINDIVERT_TCPHDR) <= newPacketLen)
            {
                PWINDIVERT_TCPHDR pFakeTcp = reinterpret_cast<PWINDIVERT_TCPHDR>(fakePkt.data() + tcpOffset);
                pFakeTcp->Checksum = htons(ntohs(pFakeTcp->Checksum) - 1);
            }
        }

        pfnSend(divertHandle, fakePkt.data(), newPacketLen, nullptr, &fakeAddr);
    }

    static void SendNativeReverseFragment(
        HANDLE divertHandle,
        pfnWinDivertSend pfnSend,
        pfnWinDivertHelperCalcChecksums pfnChecksum,
        WINDIVERT_ADDRESS addr,
        uint8_t* packet,
        UINT packetLen,
        void* packetData,
        UINT packetDataLen,
        PWINDIVERT_IPHDR pIpHdr,
        PWINDIVERT_IPV6HDR pIpV6Hdr,
        PWINDIVERT_TCPHDR pTcpHdr,
        UINT fragmentSize
    )
    {
        if (!divertHandle || !pfnSend || !pfnChecksum || !packet || (!pIpHdr && !pIpV6Hdr) || !pTcpHdr || !packetData) return;
        if (fragmentSize == 0 || fragmentSize >= packetDataLen) return;

        std::vector<uint8_t> packetBak(packet, packet + packetLen);
        UINT origPacketLen = packetLen;

        if (pIpHdr)
        {
            pIpHdr->Length = htons(ntohs(pIpHdr->Length) - static_cast<uint16_t>(fragmentSize));
        }
        else if (pIpV6Hdr)
        {
            pIpV6Hdr->Length = htons(ntohs(pIpV6Hdr->Length) - static_cast<uint16_t>(fragmentSize));
        }
        memmove(packetData, reinterpret_cast<uint8_t*>(packetData) + fragmentSize, packetDataLen - fragmentSize);
        UINT step1PacketLen = packetLen - fragmentSize;
        pTcpHdr->SeqNum = htonl(ntohl(pTcpHdr->SeqNum) + static_cast<uint32_t>(fragmentSize));

        WINDIVERT_ADDRESS addr1 = addr;
        addr1.IPChecksum = 0;
        addr1.TCPChecksum = 0;
        pfnChecksum(packet, step1PacketLen, &addr1, 0);
        pfnSend(divertHandle, packet, step1PacketLen, nullptr, &addr1);

        memcpy(packet, packetBak.data(), origPacketLen);
        if (pIpHdr)
        {
            pIpHdr->Length = htons(ntohs(pIpHdr->Length) - static_cast<uint16_t>(packetDataLen) + static_cast<uint16_t>(fragmentSize));
        }
        else if (pIpV6Hdr)
        {
            pIpV6Hdr->Length = htons(ntohs(pIpV6Hdr->Length) - static_cast<uint16_t>(packetDataLen) + static_cast<uint16_t>(fragmentSize));
        }
        UINT step0PacketLen = origPacketLen - packetDataLen + fragmentSize;

        WINDIVERT_ADDRESS addr0 = addr;
        addr0.IPChecksum = 0;
        addr0.TCPChecksum = 0;
        pfnChecksum(packet, step0PacketLen, &addr0, 0);
        pfnSend(divertHandle, packet, step0PacketLen, nullptr, &addr0);
    }

    DpiEngine& DpiEngine::Instance()
    {
        static DpiEngine s_instance;
        return s_instance;
    }

    DpiEngine::DpiEngine()
    {
    }

    DpiEngine::~DpiEngine()
    {
        Stop();
        UnloadDivertModule();
    }

    void DpiEngine::FlushDnsCache()
    {
        HMODULE hDnsApi = ::LoadLibraryW(L"dnsapi.dll");
        if (hDnsApi)
        {
            typedef BOOL(WINAPI* pfnDnsFlushResolverCache)();
            auto pfn = reinterpret_cast<pfnDnsFlushResolverCache>(::GetProcAddress(hDnsApi, "DnsFlushResolverCache"));
            if (pfn)
            {
                pfn();
            }
            ::FreeLibrary(hDnsApi);
        }
    }

    void DpiEngine::CleanupDnsConntrack()
    {
        auto now = std::chrono::steady_clock::now();
        std::lock_guard lock(m_dnsMutex);
        for (auto it = m_dnsConntrack.begin(); it != m_dnsConntrack.end(); )
        {
            if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.timestamp).count() > 30)
            {
                it = m_dnsConntrack.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    bool DpiEngine::LoadDivertModule()
    {
        if (m_hDivertModule) return true;

        wchar_t exeBuffer[MAX_PATH];
        ::GetModuleFileNameW(nullptr, exeBuffer, MAX_PATH);
        std::filesystem::path exeDir = std::filesystem::path(exeBuffer).parent_path();
        std::filesystem::path dllPath = exeDir / L"WinDivert.dll";

        m_hDivertModule = ::LoadLibraryW(dllPath.c_str());
        if (!m_hDivertModule)
        {
            m_hDivertModule = ::LoadLibraryW(L"WinDivert.dll");
        }
        if (!m_hDivertModule)
        {
            DWORD err = ::GetLastError();
            CrashLogger::Instance().LogMessage(L"DPI_ENGINE", L"Failed to load WinDivert.dll. Error: " + std::to_wstring(err) + L", Path: " + dllPath.wstring());
            return false;
        }

        m_pfnOpen = reinterpret_cast<pfnWinDivertOpen>(::GetProcAddress(m_hDivertModule, "WinDivertOpen"));
        m_pfnRecv = reinterpret_cast<pfnWinDivertRecv>(::GetProcAddress(m_hDivertModule, "WinDivertRecv"));
        m_pfnSend = reinterpret_cast<pfnWinDivertSend>(::GetProcAddress(m_hDivertModule, "WinDivertSend"));
        m_pfnClose = reinterpret_cast<pfnWinDivertClose>(::GetProcAddress(m_hDivertModule, "WinDivertClose"));
        m_pfnSetParam = reinterpret_cast<pfnWinDivertSetParam>(::GetProcAddress(m_hDivertModule, "WinDivertSetParam"));
        m_pfnParse = reinterpret_cast<pfnWinDivertHelperParsePacket>(::GetProcAddress(m_hDivertModule, "WinDivertHelperParsePacket"));
        m_pfnChecksum = reinterpret_cast<pfnWinDivertHelperCalcChecksums>(::GetProcAddress(m_hDivertModule, "WinDivertHelperCalcChecksums"));

        if (!m_pfnOpen || !m_pfnRecv || !m_pfnSend || !m_pfnClose || !m_pfnParse || !m_pfnChecksum)
        {
            CrashLogger::Instance().LogMessage(L"DPI_ENGINE", L"Failed to resolve one or more WinDivert functions from DLL.");
            UnloadDivertModule();
            return false;
        }

        return true;
    }

    void DpiEngine::UnloadDivertModule()
    {
        if (m_hDivertModule)
        {
            ::FreeLibrary(m_hDivertModule);
            m_hDivertModule = nullptr;
        }
        m_pfnOpen = nullptr;
        m_pfnRecv = nullptr;
        m_pfnSend = nullptr;
        m_pfnClose = nullptr;
        m_pfnSetParam = nullptr;
        m_pfnParse = nullptr;
        m_pfnChecksum = nullptr;
    }

    bool DpiEngine::IsRunning() const
    {
        return m_running.load();
    }

    DpiPreset DpiEngine::GetCurrentPreset() const
    {
        return m_preset;
    }

    void DpiEngine::SetPreset(DpiPreset preset)
    {
        std::lock_guard lock(m_stateMutex);
        m_preset = preset;
        if (m_running)
        {
            Stop();
            Start(preset);
        }
    }

    bool DpiEngine::Start(DpiPreset preset)
    {
        std::lock_guard lock(m_stateMutex);
        if (m_running) return true;

        if (!LoadDivertModule())
        {
            return false;
        }

        m_preset = preset;
        DomainResolver::Instance().Start();
        FlushDnsCache();

        std::string filter = "(!loopback and ((outbound and (tcp.DstPort == 80 or tcp.DstPort == 443 or udp.DstPort == 53 or (udp.DstPort == 443 and udp.PayloadLength >= 1200))) or (inbound and (udp.SrcPort == 53 or udp.SrcPort == 1253))))";

        Utils::ScopedServiceHandle hSCM(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
        if (!hSCM)
        {
            hSCM.Reset(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
        }
        if (hSCM)
        {
            Utils::ScopedServiceHandle hService(::OpenServiceW(hSCM.Get(), L"WinDivert", SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG));
            if (hService)
            {
                DWORD bytesNeeded = 0;
                ::QueryServiceConfigW(hService.Get(), nullptr, 0, &bytesNeeded);
                if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                {
                    std::vector<BYTE> buf(bytesNeeded);
                    auto pConfig = reinterpret_cast<LPQUERY_SERVICE_CONFIGW>(buf.data());
                    if (::QueryServiceConfigW(hService.Get(), pConfig, bytesNeeded, &bytesNeeded))
                    {
                        std::wstring currentSysStr = (Utils::GetExecutableDirectory() / L"WinDivert64.sys").wstring();

                        if (pConfig->dwStartType != SERVICE_DEMAND_START || (pConfig->lpBinaryPathName && wcsstr(pConfig->lpBinaryPathName, currentSysStr.c_str()) == nullptr))
                        {
                            ::ChangeServiceConfigW(hService.Get(), SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, currentSysStr.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                        }
                    }
                }
            }
        }

        HANDLE handle = m_pfnOpen(filter.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);
        if (handle == INVALID_HANDLE_VALUE)
        {
            DWORD err = ::GetLastError();
            if (err == 1058)
            {
                Utils::ScopedServiceHandle hSCMRetry(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
                if (!hSCMRetry)
                {
                    hSCMRetry.Reset(::OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
                }
                if (hSCMRetry)
                {
                    Utils::ScopedServiceHandle hServiceRetry(::OpenServiceW(hSCMRetry.Get(), L"WinDivert", SERVICE_CHANGE_CONFIG));
                    if (hServiceRetry)
                    {
                        std::wstring currentSysStr = (Utils::GetExecutableDirectory() / L"WinDivert64.sys").wstring();
                        ::ChangeServiceConfigW(hServiceRetry.Get(), SERVICE_NO_CHANGE, SERVICE_DEMAND_START, SERVICE_NO_CHANGE, currentSysStr.c_str(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
                    }
                }
                handle = m_pfnOpen(filter.c_str(), WINDIVERT_LAYER_NETWORK, 0, 0);
            }
        }

        if (handle == INVALID_HANDLE_VALUE)
        {
            DWORD err = ::GetLastError();
            std::wstring errDesc = L"WinDivertOpen failed with error code " + std::to_wstring(err);
            if (err == 2) errDesc += L" (Driver WinDivert64.sys not found)";
            else if (err == 5) errDesc += L" (Access Denied: Administrator required)";
            else if (err == 654) errDesc += L" (Incompatible driver version loaded)";
            else if (err == 1058) errDesc += L" (Service Disabled: Recovery attempted)";
            else if (err == 1275) errDesc += L" (Driver blocked by security software)";
            else if (err == 1753) errDesc += L" (Base Filtering Engine disabled)";
            CrashLogger::Instance().LogMessage(L"DPI_ENGINE", errDesc);
            return false;
        }

        if (m_pfnSetParam)
        {
            m_pfnSetParam(handle, WINDIVERT_PARAM_QUEUE_LEN, 8192);
            m_pfnSetParam(handle, WINDIVERT_PARAM_QUEUE_TIME, 2000);
            m_pfnSetParam(handle, WINDIVERT_PARAM_QUEUE_SIZE, 32 * 1024 * 1024);
        }

        m_divertHandle = handle;
        m_running = true;

        m_workerThread = std::thread(&DpiEngine::PacketLoop, this);
        return true;
    }

    void DpiEngine::Stop()
    {
        std::lock_guard lock(m_stateMutex);
        if (!m_running) return;

        m_running = false;
        HANDLE h = m_divertHandle.exchange(INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE && m_pfnClose)
        {
            m_pfnClose(h);
        }

        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }

        {
            std::lock_guard dlock(m_dnsMutex);
            m_dnsConntrack.clear();
        }

        FlushDnsCache();
    }

    void DpiEngine::PacketLoop()
    {
        std::vector<uint8_t> packetBuf(65535);
        WINDIVERT_ADDRESS addr {};

        uint32_t yandexDnsV4 = 0;
        ::inet_pton(AF_INET, "77.88.8.8", &yandexDnsV4);

        in6_addr yandexDnsV6 {};
        ::inet_pton(AF_INET6, "2a02:6b8::feed:0ff", &yandexDnsV6);

        auto pfnRecv = m_pfnRecv;
        auto pfnSend = m_pfnSend;
        auto pfnChecksum = m_pfnChecksum;
        auto pfnParse = m_pfnParse;

        auto lastCleanup = std::chrono::steady_clock::now();

        while (m_running)
        {
            try
            {
                HANDLE h = m_divertHandle.load();
                if (h == INVALID_HANDLE_VALUE) break;

                UINT readLen = 0;
                if (!pfnRecv(h, packetBuf.data(), static_cast<UINT>(packetBuf.size()), &readLen, &addr))
                {
                    if (!m_running) break;
                    continue;
                }

                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanup).count() >= 10)
                {
                    lastCleanup = now;
                    CleanupDnsConntrack();
                }

                PWINDIVERT_IPHDR pIpHdr = nullptr;
                PWINDIVERT_IPV6HDR pIpV6Hdr = nullptr;
                UINT8 protocol = 0;
                PWINDIVERT_ICMPHDR pIcmpHdr = nullptr;
                PWINDIVERT_ICMPV6HDR pIcmpV6Hdr = nullptr;
                PWINDIVERT_TCPHDR pTcpHdr = nullptr;
                PWINDIVERT_UDPHDR pUdpHdr = nullptr;
                PVOID pData = nullptr;
                UINT dataLen = 0;
                PVOID pNext = nullptr;
                UINT nextLen = 0;

                if (!pfnParse(
                    packetBuf.data(),
                    readLen,
                    &pIpHdr,
                    &pIpV6Hdr,
                    &protocol,
                    &pIcmpHdr,
                    &pIcmpV6Hdr,
                    &pTcpHdr,
                    &pUdpHdr,
                    &pData,
                    &dataLen,
                    &pNext,
                    &nextLen))
                {
                    pfnSend(h, packetBuf.data(), readLen, nullptr, &addr);
                    continue;
                }

                PresetConfig cfg = GetPresetConfig(m_preset);

                if (!addr.Outbound && pUdpHdr)
                {
                    uint16_t srcPort = ntohs(pUdpHdr->SrcPort);
                    if (srcPort == 1253 || srcPort == 53)
                    {
                        DnsConntrackKey key;
                        key.isIpv6 = (pIpV6Hdr != nullptr);
                        key.clientPort = ntohs(pUdpHdr->DstPort);
                        if (pIpHdr)
                        {
                            memcpy(key.clientIp, &pIpHdr->DstAddr, 4);
                        }
                        else if (pIpV6Hdr)
                        {
                            memcpy(key.clientIp, pIpV6Hdr->DstAddr, 16);
                        }

                        bool foundConntrack = false;
                        DnsConntrackEntry entry;
                        {
                            std::lock_guard lock(m_dnsMutex);
                            auto it = m_dnsConntrack.find(key);
                            if (it != m_dnsConntrack.end())
                            {
                                entry = it->second;
                                m_dnsConntrack.erase(it);
                                foundConntrack = true;
                            }
                        }

                        if (pData && dataLen > 0)
                        {
                            std::string dnsDomain;
                            std::vector<std::string> dnsIps;
                            if (DomainResolver::ParseDnsResponse(reinterpret_cast<uint8_t const*>(pData), dataLen, dnsDomain, dnsIps))
                            {
                                if (!dnsDomain.empty())
                                {
                                    for (auto const& resolvedIp : dnsIps)
                                    {
                                        DomainResolver::Instance().AddMapping(resolvedIp, dnsDomain);
                                        TrafficLogger::Instance().UpdateTargetForIp(Utf8ToWide(resolvedIp), Utf8ToWide(dnsDomain));
                                    }
                                }
                            }
                        }

                        if (foundConntrack)
                        {
                            if (pIpHdr)
                            {
                                memcpy(&pIpHdr->SrcAddr, entry.origDstIp, 4);
                            }
                            else if (pIpV6Hdr)
                            {
                                memcpy(pIpV6Hdr->SrcAddr, entry.origDstIp, 16);
                            }
                            pUdpHdr->SrcPort = htons(entry.origDstPort);
                            addr.UDPChecksum = 0;
                            addr.IPChecksum = 0;
                            pfnChecksum(packetBuf.data(), readLen, &addr, 0);
                        }

                        pfnSend(h, packetBuf.data(), readLen, nullptr, &addr);
                        continue;
                    }
                }

                if (addr.Outbound && pUdpHdr)
                {
                    uint16_t dstPort = ntohs(pUdpHdr->DstPort);
                    if (dstPort == 53)
                    {
                        if (cfg.doDnsRedir && pData && dataLen >= 12)
                        {
                            uint16_t dnsFlags = (static_cast<uint16_t>(reinterpret_cast<uint8_t const*>(pData)[2]) << 8) |
                                                reinterpret_cast<uint8_t const*>(pData)[3];
                            bool isQuery = ((dnsFlags & 0x8000) == 0);
                            if (isQuery)
                            {
                                DnsConntrackKey key;
                                key.isIpv6 = (pIpV6Hdr != nullptr);
                                key.clientPort = ntohs(pUdpHdr->SrcPort);
                                if (pIpHdr)
                                {
                                    memcpy(key.clientIp, &pIpHdr->SrcAddr, 4);
                                }
                                else if (pIpV6Hdr)
                                {
                                    memcpy(key.clientIp, pIpV6Hdr->SrcAddr, 16);
                                }

                                DnsConntrackEntry entry;
                                entry.origDstPort = dstPort;
                                if (pIpHdr)
                                {
                                    memcpy(entry.origDstIp, &pIpHdr->DstAddr, 4);
                                }
                                else if (pIpV6Hdr)
                                {
                                    memcpy(entry.origDstIp, pIpV6Hdr->DstAddr, 16);
                                }
                                entry.timestamp = std::chrono::steady_clock::now();

                                {
                                    std::lock_guard lock(m_dnsMutex);
                                    m_dnsConntrack[key] = entry;
                                }

                                if (pIpHdr)
                                {
                                    pIpHdr->DstAddr = yandexDnsV4;
                                }
                                else if (pIpV6Hdr)
                                {
                                    memcpy(pIpV6Hdr->DstAddr, &yandexDnsV6, 16);
                                }
                                pUdpHdr->DstPort = htons(1253);
                                addr.UDPChecksum = 0;
                                addr.IPChecksum = 0;
                                pfnChecksum(packetBuf.data(), readLen, &addr, 0);
                            }
                        }

                        std::string detectedDomain;
                        if (pData && dataLen > 0)
                        {
                            DomainResolver::ParseDnsQuestion(reinterpret_cast<uint8_t const*>(pData), dataLen, detectedDomain);
                        }
                        pfnSend(h, packetBuf.data(), readLen, nullptr, &addr);
                        continue;
                    }
                    else if (dstPort == 443)
                    {
                        if (cfg.blockQuic)
                        {
                            continue;
                        }
                    }
                }

                std::string ipStr;
                uint16_t dstPort = 0;
                std::string detectedDomain;
                std::wstring protocolStr = L"TCP";

                if (pIpHdr)
                {
                    char ipBuf[INET_ADDRSTRLEN] {};
                    IN_ADDR inAddr {};
                    inAddr.S_un.S_addr = pIpHdr->DstAddr;
                    ::inet_ntop(AF_INET, &inAddr, ipBuf, sizeof(ipBuf));
                    ipStr = ipBuf;
                }
                else if (pIpV6Hdr)
                {
                    char ipBuf[INET6_ADDRSTRLEN] {};
                    ::inet_ntop(AF_INET6, pIpV6Hdr->DstAddr, ipBuf, sizeof(ipBuf));
                    ipStr = ipBuf;
                }

                if (pTcpHdr)
                {
                    dstPort = ntohs(pTcpHdr->DstPort);
                    if (dstPort == 443) protocolStr = L"HTTPS";
                    else if (dstPort == 80) protocolStr = L"HTTP";
                    else protocolStr = L"TCP";

                    if (pData && dataLen > 0)
                    {
                        uint8_t const* byteData = reinterpret_cast<uint8_t const*>(pData);
                        if (dstPort == 443)
                        {
                            DomainResolver::ExtractSni(byteData, dataLen, detectedDomain);
                        }
                        else if (dstPort == 80)
                        {
                            DomainResolver::ParseHttpHost(byteData, dataLen, detectedDomain);
                        }

                        if (!detectedDomain.empty())
                        {
                            DomainResolver::Instance().AddMapping(ipStr, detectedDomain);
                            TrafficLogger::Instance().UpdateTargetForIp(Utf8ToWide(ipStr), Utf8ToWide(detectedDomain));
                        }
                    }

                    if (detectedDomain.empty())
                    {
                        detectedDomain = DomainResolver::Instance().GetDomainForIp(ipStr);
                    }
                }
                else if (pUdpHdr)
                {
                    dstPort = ntohs(pUdpHdr->DstPort);
                    if (dstPort == 53)
                    {
                        protocolStr = L"DNS";
                    }
                    else if (dstPort == 443)
                    {
                        protocolStr = L"UDP (QUIC)";
                    }
                    else
                    {
                        protocolStr = L"UDP";
                    }
                }

                bool isWhitelisted = WhitelistManager::Instance().IsWhitelisted(detectedDomain, ipStr);

                bool shouldLog = false;
                if (pTcpHdr && pTcpHdr->Syn && !pTcpHdr->Ack)
                {
                    shouldLog = true;
                }
                else if (pTcpHdr && pData && dataLen > 0 && (dstPort == 443 || dstPort == 80) && !detectedDomain.empty())
                {
                    shouldLog = true;
                }
                else if (pUdpHdr && dstPort != 53)
                {
                    shouldLog = true;
                }

                if (shouldLog)
                {
                    std::wstring targetDisplay = detectedDomain.empty() ? Utf8ToWide(ipStr) : Utf8ToWide(detectedDomain);
                    std::wstring ipDisplay = Utf8ToWide(ipStr);
                    TrafficLogger::Instance().AddLog(targetDisplay, ipDisplay, dstPort, protocolStr, !isWhitelisted);
                }

                if (isWhitelisted)
                {
                    pfnSend(h, packetBuf.data(), readLen, nullptr, &addr);
                    continue;
                }

                if (addr.Outbound && pTcpHdr && (pIpHdr || pIpV6Hdr) && pData && dataLen > 0)
                {
                    uint8_t const* byteData = reinterpret_cast<uint8_t const*>(pData);
                    bool isHttpsClientHello = (dataLen >= 3 &&
                        byteData[0] == 0x16 &&
                        byteData[1] == 0x03 &&
                        (byteData[2] == 0x01 || byteData[2] == 0x03));

                    bool isHttpRequest = (dataLen >= 10 &&
                        ntohs(pTcpHdr->DstPort) == 80 &&
                        (memcmp(pData, "GET ", 4) == 0 ||
                         memcmp(pData, "POST ", 5) == 0 ||
                         memcmp(pData, "HEAD ", 5) == 0));

                    if (isHttpsClientHello || isHttpRequest)
                    {
                        if (cfg.fakeWrongSeq || cfg.fakeWrongChecksum)
                        {
                            if (cfg.fakeWrongSeq)
                            {
                                SendFakeData(
                                    h,
                                    pfnSend,
                                    pfnChecksum,
                                    addr,
                                    packetBuf.data(),
                                    readLen,
                                    pData,
                                    dataLen,
                                    pIpHdr,
                                    pIpV6Hdr,
                                    pTcpHdr,
                                    isHttpsClientHello,
                                    0,
                                    false,
                                    true
                                );
                            }
                            if (cfg.fakeWrongChecksum)
                            {
                                SendFakeData(
                                    h,
                                    pfnSend,
                                    pfnChecksum,
                                    addr,
                                    packetBuf.data(),
                                    readLen,
                                    pData,
                                    dataLen,
                                    pIpHdr,
                                    pIpV6Hdr,
                                    pTcpHdr,
                                    isHttpsClientHello,
                                    0,
                                    true,
                                    false
                                );
                            }
                        }
                        else if (cfg.fakeTtl > 0)
                        {
                            SendFakeData(
                                h,
                                pfnSend,
                                pfnChecksum,
                                addr,
                                packetBuf.data(),
                                readLen,
                                pData,
                                dataLen,
                                pIpHdr,
                                pIpV6Hdr,
                                pTcpHdr,
                                isHttpsClientHello,
                                cfg.fakeTtl,
                                false,
                                false
                            );
                        }

                        if (cfg.doReverseFrag && cfg.fragSize > 0 && dataLen > cfg.fragSize)
                        {
                            SendNativeReverseFragment(
                                h,
                                pfnSend,
                                pfnChecksum,
                                addr,
                                packetBuf.data(),
                                readLen,
                                pData,
                                dataLen,
                                pIpHdr,
                                pIpV6Hdr,
                                pTcpHdr,
                                cfg.fragSize
                            );
                            continue;
                        }
                    }
                }

                pfnSend(h, packetBuf.data(), readLen, nullptr, &addr);
            }
            catch (std::exception const& ex)
            {
                std::string msg = ex.what();
                CrashLogger::Instance().LogMessage(L"PACKET_LOOP_EXCEPTION", Utf8ToWide(msg));
            }
            catch (...)
            {
                CrashLogger::Instance().LogMessage(L"PACKET_LOOP_EXCEPTION", L"Unknown exception caught in PacketLoop.");
            }
        }
    }
}
