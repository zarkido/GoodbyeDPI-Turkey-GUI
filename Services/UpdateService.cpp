#include "pch.h"
#include "UpdateService.h"
#include "Utils/PathUtils.h"
#include "Utils/StringUtils.h"
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <sstream>
#include <cstdio>
#include <cwchar>
#include <winrt/Windows.Data.Json.h>
#include <urlmon.h>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "urlmon.lib")

namespace GoodByDpi_App::Services
{
    using namespace winrt::Windows::Data::Json;

    struct ScopedWinHttpHandle
    {
        HINTERNET handle{ nullptr };

        ScopedWinHttpHandle() = default;
        explicit ScopedWinHttpHandle(HINTERNET h) : handle(h) {}
        ~ScopedWinHttpHandle()
        {
            Reset();
        }

        ScopedWinHttpHandle(ScopedWinHttpHandle const&) = delete;
        ScopedWinHttpHandle& operator=(ScopedWinHttpHandle const&) = delete;

        ScopedWinHttpHandle(ScopedWinHttpHandle&& other) noexcept : handle(other.handle)
        {
            other.handle = nullptr;
        }

        ScopedWinHttpHandle& operator=(ScopedWinHttpHandle&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }

        void Reset(HINTERNET h = nullptr)
        {
            if (handle)
            {
                ::WinHttpCloseHandle(handle);
                handle = nullptr;
            }
            handle = h;
        }

        HINTERNET Get() const
        {
            return handle;
        }

        explicit operator bool() const
        {
            return handle != nullptr;
        }
    };

    UpdateService& UpdateService::Instance()
    {
        static UpdateService s_instance;
        return s_instance;
    }

    std::wstring UpdateService::GetCurrentVersion() const
    {
        return L"v1.1.0";
    }

    bool UpdateService::IsNewerVersion(std::wstring const& currentVer, std::wstring const& latestVer)
    {
        if (latestVer.empty()) return false;

        std::wstring cur = currentVer;
        if (!cur.empty() && (cur.front() == L'v' || cur.front() == L'V'))
        {
            cur = cur.substr(1);
        }

        std::wstring lat = latestVer;
        if (!lat.empty() && (lat.front() == L'v' || lat.front() == L'V'))
        {
            lat = lat.substr(1);
        }

        int cMajor = 0, cMinor = 0, cPatch = 0;
        int lMajor = 0, lMinor = 0, lPatch = 0;

        swscanf_s(cur.c_str(), L"%d.%d.%d", &cMajor, &cMinor, &cPatch);
        swscanf_s(lat.c_str(), L"%d.%d.%d", &lMajor, &lMinor, &lPatch);

        if (lMajor != cMajor) return lMajor > cMajor;
        if (lMinor != cMinor) return lMinor > cMinor;
        return lPatch > cPatch;
    }

    bool UpdateService::ParseReleaseJson(std::wstring const& jsonStr, UpdateInfo& outInfo)
    {
        JsonObject root;
        if (!JsonObject::TryParse(winrt::hstring(jsonStr), root))
        {
            return false;
        }

        outInfo.currentVersion = Instance().GetCurrentVersion();
        outInfo.latestVersion = std::wstring(root.GetNamedString(L"tag_name", L""));
        outInfo.releaseTitle = std::wstring(root.GetNamedString(L"name", L""));
        outInfo.releaseUrl = std::wstring(root.GetNamedString(L"html_url", L""));

        auto assets = root.GetNamedArray(L"assets", nullptr);
        if (assets)
        {
            for (uint32_t i = 0; i < assets.Size(); ++i)
            {
                auto item = assets.GetObjectAt(i);
                std::wstring name = std::wstring(item.GetNamedString(L"name", L""));
                if (name == L"GoodByDpi.exe")
                {
                    outInfo.downloadUrl = std::wstring(item.GetNamedString(L"browser_download_url", L""));
                    outInfo.downloadSize = static_cast<uint64_t>(item.GetNamedNumber(L"size", 0.0));
                    break;
                }
            }
        }

        if (outInfo.downloadUrl.empty())
        {
            outInfo.downloadUrl = outInfo.releaseUrl;
        }

        outInfo.isUpdateAvailable = IsNewerVersion(outInfo.currentVersion, outInfo.latestVersion);
        return true;
    }

    void UpdateService::CheckForUpdatesAsync(std::function<void(bool, UpdateInfo const&)> onComplete)
    {
        std::thread([onComplete]() {
            UpdateInfo info;
            info.currentVersion = Instance().GetCurrentVersion();

            ScopedWinHttpHandle hSession(::WinHttpOpen(
                L"GoodbyeDPI-Turkey-GUI-Updater",
                WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                WINHTTP_NO_PROXY_NAME,
                WINHTTP_NO_PROXY_BYPASS,
                0));

            if (!hSession)
            {
                if (onComplete) onComplete(false, info);
                return;
            }

            ::WinHttpSetTimeouts(hSession.Get(), 5000, 5000, 10000, 10000);

            ScopedWinHttpHandle hConnect(::WinHttpConnect(
                hSession.Get(),
                L"api.github.com",
                INTERNET_DEFAULT_HTTPS_PORT,
                0));

            if (!hConnect)
            {
                if (onComplete) onComplete(false, info);
                return;
            }

            ScopedWinHttpHandle hRequest(::WinHttpOpenRequest(
                hConnect.Get(),
                L"GET",
                L"/repos/zarkido/GoodbyeDPI-Turkey-GUI/releases/latest",
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                WINHTTP_FLAG_SECURE));

            if (!hRequest)
            {
                if (onComplete) onComplete(false, info);
                return;
            }

            DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
            ::WinHttpSetOption(hRequest.Get(), WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            LPCWSTR headers = L"User-Agent: GoodbyeDPI-Turkey-GUI\r\nAccept: application/vnd.github.v3+json\r\n";
            ::WinHttpAddRequestHeaders(hRequest.Get(), headers, static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD);

            if (!::WinHttpSendRequest(hRequest.Get(), WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
            {
                if (onComplete) onComplete(false, info);
                return;
            }

            if (!::WinHttpReceiveResponse(hRequest.Get(), nullptr))
            {
                if (onComplete) onComplete(false, info);
                return;
            }

            std::string utf8Body;
            DWORD bytesAvailable = 0;
            while (::WinHttpQueryDataAvailable(hRequest.Get(), &bytesAvailable) && bytesAvailable > 0)
            {
                std::vector<char> buffer(bytesAvailable + 1);
                DWORD bytesRead = 0;
                if (::WinHttpReadData(hRequest.Get(), buffer.data(), bytesAvailable, &bytesRead) && bytesRead > 0)
                {
                    utf8Body.append(buffer.data(), bytesRead);
                }
                else
                {
                    break;
                }
            }

            if (utf8Body.empty())
            {
                if (onComplete) onComplete(false, info);
                return;
            }

            std::wstring wideBody = Utils::Utf8ToWide(utf8Body);
            bool parsed = ParseReleaseJson(wideBody, info);
            if (onComplete)
            {
                onComplete(parsed, info);
            }
        }).detach();
    }

    void UpdateService::DownloadUpdateAsync(
        std::wstring const& downloadUrl,
        std::function<void(size_t, size_t)> onProgress,
        std::function<void(bool, std::wstring const&)> onComplete)
    {
        std::thread([downloadUrl, onProgress, onComplete]() {
            if (downloadUrl.empty())
            {
                if (onComplete) onComplete(false, L"");
                return;
            }

            std::filesystem::path targetFile = Utils::GetAppDataDirectory() / L"GoodByDpi_Update.exe";
            std::error_code ec;
            std::filesystem::remove(targetFile, ec);

            class DownloadCallback : public IBindStatusCallback
            {
            public:
                std::function<void(size_t, size_t)> progressCb;
                DownloadCallback(std::function<void(size_t, size_t)> cb) : progressCb(cb) {}

                STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override
                {
                    if (!ppvObject) return E_POINTER;
                    if (riid == IID_IUnknown || riid == IID_IBindStatusCallback)
                    {
                        *ppvObject = static_cast<IBindStatusCallback*>(this);
                        return S_OK;
                    }
                    *ppvObject = nullptr;
                    return E_NOINTERFACE;
                }

                STDMETHOD_(ULONG, AddRef)() override { return 1; }
                STDMETHOD_(ULONG, Release)() override { return 1; }

                STDMETHOD(OnStartBinding)(DWORD, IBinding*) override { return S_OK; }
                STDMETHOD(GetPriority)(LONG*) override { return S_OK; }
                STDMETHOD(OnLowResource)(DWORD) override { return S_OK; }
                STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG, LPCWSTR) override
                {
                    if (progressCb && ulProgressMax > 0)
                    {
                        progressCb(static_cast<size_t>(ulProgress), static_cast<size_t>(ulProgressMax));
                    }
                    return S_OK;
                }
                STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR) override { return S_OK; }
                STDMETHOD(GetBindInfo)(DWORD*, BINDINFO*) override { return S_OK; }
                STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC*, STGMEDIUM*) override { return S_OK; }
                STDMETHOD(OnObjectAvailable)(REFIID, IUnknown*) override { return S_OK; }
            };

            DownloadCallback callback(onProgress);
            HRESULT hr = ::URLDownloadToFileW(
                nullptr,
                downloadUrl.c_str(),
                targetFile.c_str(),
                0,
                onProgress ? &callback : nullptr
            );

            if (FAILED(hr) || !std::filesystem::exists(targetFile, ec) || std::filesystem::file_size(targetFile, ec) < 1024 * 1024)
            {
                std::filesystem::remove(targetFile, ec);
                if (onComplete) onComplete(false, L"");
                return;
            }

            if (onComplete)
            {
                onComplete(true, targetFile.wstring());
            }
        }).detach();
    }

    bool UpdateService::ApplyUpdateAndRestart(std::wstring const& downloadedFilePath)
    {
        std::error_code ec;
        if (!std::filesystem::exists(downloadedFilePath, ec) || std::filesystem::file_size(downloadedFilePath, ec) < 1024 * 1024)
        {
            return false;
        }

        std::wstring targetExe = Utils::GetLauncherExecutablePath();
        if (targetExe.empty())
        {
            return false;
        }

        std::filesystem::path appDataDir = Utils::GetAppDataDirectory();
        std::filesystem::path batchPath = appDataDir / L"updater.cmd";
        std::filesystem::path versionFile = appDataDir / L"app" / L".version";
        std::wofstream batchFile(batchPath, std::ios::trunc);
        if (!batchFile.is_open())
        {
            return false;
        }

        DWORD pid = ::GetCurrentProcessId();

        batchFile << L"@echo off\r\n";
        batchFile << L"chcp 65001 >nul\r\n";
        batchFile << L":wait_loop\r\n";
        batchFile << L"tasklist /fi \"PID eq " << pid << L"\" 2>nul | findstr /i \"" << pid << L"\" >nul\r\n";
        batchFile << L"if not errorlevel 1 (\r\n";
        batchFile << L"    timeout /t 1 /nobreak >nul\r\n";
        batchFile << L"    goto wait_loop\r\n";
        batchFile << L")\r\n";
        batchFile << L"timeout /t 1 /nobreak >nul\r\n";
        batchFile << L"taskkill /f /im GoodByDpi_App.exe >nul 2>&1\r\n";
        batchFile << L"taskkill /f /im goodbyedpi.exe >nul 2>&1\r\n";
        batchFile << L"net stop WinDivert >nul 2>&1\r\n";
        batchFile << L"if exist \"" << versionFile.wstring() << L"\" del /f /q \"" << versionFile.wstring() << L"\" >nul 2>&1\r\n";
        batchFile << L":copy_loop\r\n";
        batchFile << L"copy /y \"" << downloadedFilePath << L"\" \"" << targetExe << L"\" >nul 2>&1\r\n";
        batchFile << L"if errorlevel 1 (\r\n";
        batchFile << L"    timeout /t 1 /nobreak >nul\r\n";
        batchFile << L"    goto copy_loop\r\n";
        batchFile << L")\r\n";
        batchFile << L"del /f /q \"" << downloadedFilePath << L"\" >nul 2>&1\r\n";
        batchFile << L"start \"\" \"" << targetExe << L"\"\r\n";
        batchFile << L"del /f /q \"%~f0\" >nul 2>&1\r\n";
        batchFile.close();

        HINSTANCE res = ::ShellExecuteW(nullptr, L"open", batchPath.c_str(), nullptr, nullptr, SW_HIDE);
        return reinterpret_cast<INT_PTR>(res) > 32;
    }

    void UpdateService::OpenReleasePage(std::wstring const& releaseUrl)
    {
        std::wstring url = releaseUrl;
        if (url.empty())
        {
            url = L"https://github.com/zarkido/GoodbyeDPI-Turkey-GUI/releases/latest";
        }
        ::ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
