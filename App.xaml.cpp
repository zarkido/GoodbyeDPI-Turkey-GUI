#include "pch.h"
#include "App.xaml.h"
#include "MainWindow.xaml.h"
#include "Services/CrashLogger.h"
#include "Services/SettingsManager.h"
#include <microsoft.ui.xaml.window.h>
#include <shellapi.h>
#include <string_view>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::GoodByDpi_App::implementation
{
    App::App()
    {
        ::GoodByDpi_App::Services::CrashLogger::Instance().Initialize();
        ::GoodByDpi_App::Services::SettingsManager::Instance().Initialize();
        this->UnhandledException([](winrt::Windows::Foundation::IInspectable const&, Microsoft::UI::Xaml::UnhandledExceptionEventArgs const& e) {
            ::GoodByDpi_App::Services::CrashLogger::Instance().LogXamlException(e.Message().c_str());
        });
        InitializeComponent();
    }

    void App::OnLaunched(LaunchActivatedEventArgs const&)
    {
        m_singleInstanceMutex.Reset(::CreateMutexW(nullptr, TRUE, L"GoodByDpi_App_SingleInstance_Mutex"));
        if (::GetLastError() == ERROR_ALREADY_EXISTS)
        {
            m_singleInstanceMutex.Reset();
            ::AllowSetForegroundWindow(ASFW_ANY);
            UINT const restoreMsg = ::RegisterWindowMessageW(L"GoodByDpi_RestoreInstance");
            ::PostMessageW(HWND_BROADCAST, restoreMsg, 0, 0);
            ::ExitProcess(0);
            return;
        }

        bool isAutoStart = false;
        int numArgs = 0;
        LPWSTR* argList = ::CommandLineToArgvW(::GetCommandLineW(), &numArgs);
        if (argList)
        {
            for (int i = 1; i < numArgs; ++i)
            {
                if (std::wstring_view(argList[i]) == L"--autostart" || std::wstring_view(argList[i]) == L"-autostart")
                {
                    isAutoStart = true;
                    break;
                }
            }
            ::LocalFree(argList);
        }

        m_window = make<MainWindow>();

        if (isAutoStart)
        {
            auto windowNative = m_window.try_as<::IWindowNative>();
            if (windowNative)
            {
                HWND hwnd{ nullptr };
                windowNative->get_WindowHandle(&hwnd);
                if (hwnd)
                {
                    ::ShowWindow(hwnd, SW_HIDE);
                }
            }
            auto mainWin = winrt::get_self<MainWindow>(m_window.as<winrt::GoodByDpi_App::MainWindow>());
            if (mainWin)
            {
                mainWin->OnWindowHiddenToTray();
                mainWin->StartAutoService();
                mainWin->ShowTrayNotification();
            }
        }
        else
        {
            m_window.Activate();
        }
    }
}
