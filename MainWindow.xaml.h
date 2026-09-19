#pragma once
#include "MainWindow.g.h"
#include "MainWindow.xaml.g.h"
#include <winrt/Microsoft.UI.Dispatching.h>
#include "ViewModels/MainViewModel.h"
#include "UI/TrayManager.h"
#include "UI/Components/DropdownMenu.h"
#include "UI/Components/HoverButton.h"
#include "UI/Components/AnimatedToggle.h"
#include "UI/Components/LogCardFactory.h"
#include "UI/Components/WhitelistView.h"
#include "Utils/UiUtils.h"
#include <memory>
#include <unordered_map>
#include <chrono>

namespace winrt::GoodByDpi_App::implementation
{
    struct MainWindow : MainWindowT<MainWindow>
    {
        MainWindow();

        void ActionRing_StateToggled(winrt::Windows::Foundation::IInspectable const&, bool const&);
        void SettingsBackdrop_PointerPressed(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);

        void RestoreWindow();
        void OnWindowHiddenToTray();
        void ToggleStateFromTray();
        void ExitApplication();
        void ShowTrayNotification();
        void StartAutoService();

    private:
        void InitializeWindowPresenter();
        void InitializeComponents();
        void InitializeTrayIcon();
        void RemoveTrayIcon();
        void UpdateTrayTooltip();
        winrt::fire_and_forget ShowCloseConfirmation();
        void UpdateLocalizedStrings();
        void UpdateStatusDisplay();
        void PopulateInitialLogs();
        void UpdateTitleBarRegions();
        void OnWindowResizeTimerTick(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&);
        void OnScrollInactivityTimerTick(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&);
        void OnScrollInteraction();
        void ToggleLogPanel();
        void RefreshWhitelistView();
        void AppendLogCard(::GoodByDpi_App::Services::ConnectionLogItem const& logItem);
        void ClearLogVisuals();
        void CloseSettingsModal();

        std::shared_ptr<::GoodByDpi_App::ViewModels::MainViewModel> m_viewModel;
        void* m_hwnd{ nullptr };
        ::GoodByDpi_App::UI::TrayManager m_trayManager;
        UINT m_restoreMsg{ 0 };

        ::GoodByDpi_App::UI::Components::HoverButton m_settingsBtn;
        ::GoodByDpi_App::UI::Components::HoverButton m_minimizeBtn;
        ::GoodByDpi_App::UI::Components::HoverButton m_closeBtn;
        ::GoodByDpi_App::UI::Components::HoverButton m_logToggleBtn;
        ::GoodByDpi_App::UI::Components::HoverButton m_logClearBtn;
        ::GoodByDpi_App::UI::Components::HoverButton m_settingsCloseBtn;

        ::GoodByDpi_App::UI::Components::DropdownMenu m_presetDropdown;
        ::GoodByDpi_App::UI::Components::DropdownMenu m_pingCountryDropdown;
        ::GoodByDpi_App::UI::Components::DropdownMenu m_langDropdown;

        ::GoodByDpi_App::UI::Components::AnimatedToggle m_autoStartToggle;
        ::GoodByDpi_App::UI::Components::WhitelistView m_whitelistView;

        winrt::Microsoft::UI::Xaml::DispatcherTimer m_windowResizeTimer{ nullptr };
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_scrollInactivityTimer{ nullptr };
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_trimTimer{ nullptr };
        std::chrono::steady_clock::time_point m_lastScrollInteractionTime{};
        bool m_isWindowVisible{ true };
        bool m_isLogOpen{ false };
        double m_currentWindowWidth{ 1200.0 };
        double m_targetWindowWidth{ 1200.0 };
        winrt::Microsoft::UI::Dispatching::DispatcherQueue m_dispatcherQueue{ nullptr };

        std::unordered_map<uint64_t, winrt::Microsoft::UI::Xaml::Controls::TextBlock> m_logTargetTextBlocks;
    };
}

namespace winrt::GoodByDpi_App::factory_implementation
{
    struct MainWindow : MainWindowT<MainWindow, implementation::MainWindow>
    {
    };
}
