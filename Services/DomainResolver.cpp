#include "pch.h"
#include "DomainResolver.h"
#include "TrafficLogger.h"
#include "Utils/StringUtils.h"
#include <ws2tcpip.h>
#include <algorithm>
#include <cstring>
#include <string_view>

#pragma comment(lib, "Ws2_32.lib")

namespace GoodByDpi_App::Services
{
    using Utils::Utf8ToWide;

    DomainResolver& DomainResolver::Instance()
    {
        static DomainResolver s_instance;
        return s_instance;
    }

    DomainResolver::DomainResolver()
    {
        Start();
    }

    DomainResolver::~DomainResolver()
    {
        Stop();
    }

    void DomainResolver::Start()
    {
        if (m_running) return;
        m_running = true;
        m_workerThread = std::thread(&DomainResolver::WorkerLoop, this);
    }

    void DomainResolver::Stop()
    {
        if (!m_running) return;
        m_running = false;
        m_queueCv.notify_all();
        if (m_workerThread.joinable())
        {
            m_workerThread.join();
        }
    }

    void DomainResolver::AddMapping(std::string const& ip, std::string const& domain)
    {
        if (ip.empty() || domain.empty()) return;
        {
            std::unique_lock lock(m_cacheMutex);
            if (m_ipToDomain.size() > 10000)
            {
                m_ipToDomain.clear();
            }
            m_ipToDomain[ip] = domain;
        }
    }

    std::string DomainResolver::GetDomainForIp(std::string const& ip)
    {
        if (ip.empty()) return {};
        {
            std::shared_lock lock(m_cacheMutex);
            auto it = m_ipToDomain.find(ip);
            if (it != m_ipToDomain.end())
            {
                return it->second;
            }
        }
        QueueReverseLookup(ip);
        return {};
    }

    void DomainResolver::RegisterResolvedCallback(std::function<void(std::string const&, std::string const&)> callback)
    {
        std::lock_guard lock(m_cbMutex);
        m_callbacks.push_back(std::move(callback));
    }

    void DomainResolver::QueueReverseLookup(std::string const& ip)
    {
        std::lock_guard lock(m_queueMutex);
        if (m_queuedIps.find(ip) != m_queuedIps.end()) return;
        if (m_lookupQueue.size() > 200) return;

        m_queuedIps.insert(ip);
        m_lookupQueue.push_back(ip);
        m_queueCv.notify_one();
    }

    void DomainResolver::WorkerLoop()
    {
        while (m_running)
        {
            std::string ip;
            {
                std::unique_lock lock(m_queueMutex);
                m_queueCv.wait(lock, [this]() {
                    return !m_running || !m_lookupQueue.empty();
                });
                if (!m_running) break;
                ip = m_lookupQueue.front();
                m_lookupQueue.pop_front();
            }

            std::string host = PerformReverseLookup(ip);
            if (!host.empty())
            {
                AddMapping(ip, host);
                TrafficLogger::Instance().UpdateTargetForIp(Utf8ToWide(ip), Utf8ToWide(host));

                std::vector<std::function<void(std::string const&, std::string const&)>> cbs;
                {
                    std::lock_guard lock(m_cbMutex);
                    cbs = m_callbacks;
                }
                for (auto const& cb : cbs)
                {
                    if (cb) cb(ip, host);
                }
            }
        }
    }

    std::string DomainResolver::PerformReverseLookup(std::string const& ip)
    {
        sockaddr_in sa4{};
        sockaddr_in6 sa6{};
        sockaddr* sa = nullptr;
        socklen_t saLen = 0;

        if (::inet_pton(AF_INET, ip.c_str(), &sa4.sin_addr) == 1)
        {
            sa4.sin_family = AF_INET;
            sa = reinterpret_cast<sockaddr*>(&sa4);
            saLen = sizeof(sa4);
        }
        else if (::inet_pton(AF_INET6, ip.c_str(), &sa6.sin6_addr) == 1)
        {
            sa6.sin6_family = AF_INET6;
            sa = reinterpret_cast<sockaddr*>(&sa6);
            saLen = sizeof(sa6);
        }
        else
        {
            return {};
        }

        char host[NI_MAXHOST]{};
        if (::getnameinfo(sa, saLen, host, sizeof(host), nullptr, 0, NI_NAMEREQD) == 0)
        {
            std::string h(host);
            std::transform(h.begin(), h.end(), h.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return h;
        }
        return {};
    }

    bool DomainResolver::ExtractSni(uint8_t const* data, size_t len, std::string& outSni)
    {
        if (!data || len < 12) return false;

        if (data[0] == 0x16 && data[1] == 0x03 && len >= 44 && data[5] == 0x01)
        {
            size_t pos = 43;
            if (pos < len)
            {
                uint8_t sessIdLen = data[pos++];
                pos += sessIdLen;
                if (pos + 2 <= len)
                {
                    uint16_t cipherLen = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
                    pos += 2 + cipherLen;
                    if (pos < len)
                    {
                        uint8_t compLen = data[pos++];
                        pos += compLen;
                        if (pos + 2 <= len)
                        {
                            uint16_t extTotalLen = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
                            pos += 2;
                            size_t maxExtEnd = std::min(len, pos + extTotalLen);
                            while (pos + 4 <= maxExtEnd)
                            {
                                uint16_t extType = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
                                uint16_t extLen = (static_cast<uint16_t>(data[pos + 2]) << 8) | data[pos + 3];
                                pos += 4;
                                if (extType == 0)
                                {
                                    if (pos + 5 <= maxExtEnd && pos + extLen <= len)
                                    {
                                        size_t sniPos = pos + 2;
                                        uint8_t nameType = data[sniPos++];
                                        if (nameType == 0)
                                        {
                                            uint16_t nameLen = (static_cast<uint16_t>(data[sniPos]) << 8) | data[sniPos + 1];
                                            sniPos += 2;
                                            if (nameLen >= 3 && nameLen <= 253 && sniPos + nameLen <= len)
                                            {
                                                std::string candidate(reinterpret_cast<char const*>(data + sniPos), nameLen);
                                                bool valid = true;
                                                for (char& c : candidate)
                                                {
                                                    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                                                    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_'))
                                                    {
                                                        valid = false;
                                                        break;
                                                    }
                                                }
                                                if (valid)
                                                {
                                                    outSni = std::move(candidate);
                                                    return true;
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                                pos += extLen;
                            }
                        }
                    }
                }
            }
        }

        for (size_t ptr = 0; ptr + 9 < len; ++ptr)
        {
            if (data[ptr] == 0x00 && data[ptr + 1] == 0x00 && data[ptr + 2] == 0x00 &&
                data[ptr + 4] == 0x00 && data[ptr + 6] == 0x00 && data[ptr + 7] == 0x00)
            {
                int extLen = static_cast<int>(data[ptr + 3]);
                int listLen = static_cast<int>(data[ptr + 5]);
                int nameLen = static_cast<int>(data[ptr + 8]);
                if (extLen - listLen == 2 && listLen - nameLen == 3)
                {
                    if (ptr + 9 + nameLen <= len && nameLen >= 3 && nameLen <= 253)
                    {
                        std::string candidate(reinterpret_cast<char const*>(data + ptr + 9), nameLen);
                        bool valid = true;
                        for (char& c : candidate)
                        {
                            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_'))
                            {
                                valid = false;
                                break;
                            }
                        }
                        if (valid)
                        {
                            outSni = std::move(candidate);
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    bool DomainResolver::ParseHttpHost(uint8_t const* data, size_t len, std::string& outHost)
    {
        if (!data || len < 10) return false;
        std::string_view sv(reinterpret_cast<char const*>(data), len);

        size_t hostPos = sv.find("Host: ");
        if (hostPos == std::string_view::npos)
        {
            hostPos = sv.find("host: ");
        }
        if (hostPos == std::string_view::npos)
        {
            hostPos = sv.find("HOST: ");
        }
        if (hostPos == std::string_view::npos) return false;

        size_t valStart = hostPos + 6;
        size_t valEnd = sv.find("\r", valStart);
        if (valEnd == std::string_view::npos)
        {
            valEnd = sv.find("\n", valStart);
        }
        if (valEnd == std::string_view::npos || valEnd <= valStart) return false;

        std::string candidate(sv.substr(valStart, valEnd - valStart));
        size_t colon = candidate.find(':');
        if (colon != std::string::npos)
        {
            candidate = candidate.substr(0, colon);
        }
        while (!candidate.empty() && (candidate.back() == ' ' || candidate.back() == '\t'))
        {
            candidate.pop_back();
        }
        if (candidate.size() < 3 || candidate.size() > 253) return false;

        for (char& c : candidate)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_'))
            {
                return false;
            }
        }
        outHost = std::move(candidate);
        return true;
    }

    bool DomainResolver::ParseDnsQuestion(uint8_t const* data, size_t len, std::string& outDomain)
    {
        if (!data || len < 13) return false;
        uint16_t qdCount = (static_cast<uint16_t>(data[4]) << 8) | data[5];
        if (qdCount == 0) return false;

        size_t pos = 12;
        std::string domain;
        while (pos < len)
        {
            uint8_t labelLen = data[pos++];
            if (labelLen == 0)
            {
                if (domain.size() >= 3 && domain.size() <= 253)
                {
                    outDomain = std::move(domain);
                    return true;
                }
                return false;
            }
            if ((labelLen & 0xC0) != 0) return false;
            if (pos + labelLen > len) return false;

            if (!domain.empty()) domain += '.';
            for (size_t i = 0; i < labelLen; ++i)
            {
                char c = static_cast<char>(std::tolower(static_cast<unsigned char>(data[pos + i])));
                if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_'))
                {
                    return false;
                }
                domain += c;
            }
            pos += labelLen;
        }
        return false;
    }

    bool DomainResolver::ParseDnsResponse(uint8_t const* data, size_t len, std::string& outDomain, std::vector<std::string>& outIps)
    {
        if (!data || len < 12) return false;
        uint16_t flags = (static_cast<uint16_t>(data[2]) << 8) | data[3];
        bool isResponse = ((flags & 0x8000) != 0);
        uint8_t rcode = (flags & 0x000F);
        if (!isResponse || rcode != 0) return false;

        uint16_t qdCount = (static_cast<uint16_t>(data[4]) << 8) | data[5];
        uint16_t anCount = (static_cast<uint16_t>(data[6]) << 8) | data[7];
        if (anCount == 0) return false;

        size_t pos = 12;
        ParseDnsQuestion(data, len, outDomain);

        for (uint16_t q = 0; q < qdCount && pos < len; ++q)
        {
            while (pos < len)
            {
                uint8_t l = data[pos++];
                if (l == 0) break;
                if ((l & 0xC0) == 0xC0)
                {
                    if (pos < len) pos++;
                    break;
                }
                if (pos + l > len) return false;
                pos += l;
            }
            if (pos + 4 > len) return false;
            pos += 4;
        }

        for (uint16_t a = 0; a < anCount && pos < len; ++a)
        {
            while (pos < len)
            {
                uint8_t l = data[pos++];
                if (l == 0) break;
                if ((l & 0xC0) == 0xC0)
                {
                    if (pos < len) pos++;
                    break;
                }
                if (pos + l > len) return false;
                pos += l;
            }

            if (pos + 10 > len) break;
            uint16_t rtype = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
            pos += 8;
            uint16_t rdlen = (static_cast<uint16_t>(data[pos]) << 8) | data[pos + 1];
            pos += 2;

            if (pos + rdlen > len) break;

            if (rtype == 1 && rdlen == 4)
            {
                char ipBuf[INET_ADDRSTRLEN]{};
                IN_ADDR addr{};
                memcpy(&addr, data + pos, 4);
                ::inet_ntop(AF_INET, &addr, ipBuf, sizeof(ipBuf));
                outIps.push_back(ipBuf);
            }
            else if (rtype == 28 && rdlen == 16)
            {
                char ipBuf[INET6_ADDRSTRLEN]{};
                IN6_ADDR addr{};
                memcpy(&addr, data + pos, 16);
                ::inet_ntop(AF_INET6, &addr, ipBuf, sizeof(ipBuf));
                outIps.push_back(ipBuf);
            }

            pos += rdlen;
        }

        return !outIps.empty();
    }
}
