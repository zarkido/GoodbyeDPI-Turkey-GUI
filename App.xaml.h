#pragma once
#include "App.g.h"
#include "App.xaml.g.h"

namespace winrt::GoodByDpi_App::implementation
{
    struct App : AppT<App>
    {
        App();
        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
    private:
        winrt::Microsoft::UI::Xaml::Window m_window{ nullptr };
        HANDLE m_singleInstanceMutex{ nullptr };
    };
}

namespace winrt::GoodByDpi_App::factory_implementation
{
    struct App : AppT<App, implementation::App>
    {
    };
}
