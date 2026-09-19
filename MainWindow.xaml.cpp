#include "pch.h"
#include "MainWindow.xaml.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Text.h>
#include <microsoft.ui.xaml.window.h>
#include <winuser.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <filesystem>
#include <cmath>
#include <algorithm>
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace ::GoodByDpi_App::Services;
using namespace ::GoodByDpi_App::UI::Components;
using namespace ::GoodByDpi_App::Utils;

namespace winrt::GoodByDpi_App::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

        auto windowNative = this->try_as<::IWindowNative>();
        if (windowNative)
        {
            HWND hwnd{ nullptr };
            windowNative->get_WindowHandle(&hwnd);
            m_hwnd = hwnd;
        }

        InitializeWindowPresenter();

        m_restoreMsg = ::RegisterWindowMessageW(L"GoodByDpi_RestoreInstance");
        ::ChangeWindowMessageFilterEx(static_cast<HWND>(m_hwnd), m_restoreMsg, MSGFLT_ALLOW, nullptr);

        m_windowResizeTimer = DispatcherTimer();
        m_windowResizeTimer.Interval(std::chrono::milliseconds(16));
        m_windowResizeTimer.Tick({ this, &MainWindow::OnWindowResizeTimerTick });

        m_trimTimer = DispatcherTimer();
        m_trimTimer.Interval(std::chrono::milliseconds(400));
        m_trimTimer.Tick([this](auto const&, auto const&) {
            if (m_trimTimer && m_trimTimer.IsEnabled())
            {
                m_trimTimer.Stop();
            }
            if (!m_isWindowVisible)
            {
                ::SetProcessWorkingSetSize(::GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
            }
        });

        m_viewModel = std::make_shared<::GoodByDpi_App::ViewModels::MainViewModel>();

        InitializeComponents();
        InitializeTrayIcon();

        AppTitleBar().Loaded([this](IInspectable const&, RoutedEventArgs const&) {
            UpdateTitleBarRegions();
        });
        AppTitleBar().SizeChanged([this](IInspectable const&, SizeChangedEventArgs const&) {
            UpdateTitleBarRegions();
        });
        this->AppWindow().Changed([this](Microsoft::UI::Windowing::AppWindow const&, Microsoft::UI::Windowing::AppWindowChangedEventArgs const&) {
            UpdateTitleBarRegions();
        });
        UpdateTitleBarRegions();

        m_dispatcherQueue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();

        PingService::Instance().RegisterPingUpdatedCallback([this](int euMs, int countryMs) {
            if (m_dispatcherQueue)
            {
                m_dispatcherQueue.TryEnqueue([this, euMs, countryMs]() {
                    if (EuPingBlock())
                    {
                        EuPingBlock().Text(L"DE: " + (euMs >= 0 ? std::to_wstring(euMs) + L" ms" : L"-- ms"));
                    }
                    if (CountryPingBlock() && m_viewModel)
                    {
                        std::wstring code = m_viewModel->GetPingCountry();
                        CountryPingBlock().Text(code + L": " + (countryMs >= 0 ? std::to_wstring(countryMs) + L" ms" : L"-- ms"));
                    }
                });
            }
        });

        std::wstring savedCountry = m_viewModel->GetPingCountry();
        PingService::Instance().SetCountry(savedCountry);
        PingService::Instance().Start();
        if (CountryPingBlock())
        {
            CountryPingBlock().Text(savedCountry + L": -- ms");
        }

        TrafficLogger::Instance().RegisterLogAddedCallback([this](ConnectionLogItem const& logItem) {
            if (!m_isWindowVisible || !m_isLogOpen) return;
            if (m_dispatcherQueue)
            {
                m_dispatcherQueue.TryEnqueue([this, logItem]() {
                    if (!m_isWindowVisible || !m_isLogOpen) return;
                    AppendLogCard(logItem);

                    bool canAutoScroll = true;
                    if (m_lastScrollInteractionTime != std::chrono::steady_clock::time_point{})
                    {
                        auto now = std::chrono::steady_clock::now();
                        auto diffMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastScrollInteractionTime).count();
                        if (diffMs < 5000)
                        {
                            canAutoScroll = false;
                        }
                    }
                    if (canAutoScroll && LogScrollViewer())
                    {
                        LogScrollViewer().ChangeView(nullptr, LogScrollViewer().ScrollableHeight(), nullptr);
                    }
                });
            }
        });

        TrafficLogger::Instance().RegisterLogUpdatedCallback([this](uint64_t id, std::wstring const& newTarget) {
            if (!m_isWindowVisible || !m_isLogOpen) return;
            if (m_dispatcherQueue)
            {
                m_dispatcherQueue.TryEnqueue([this, id, newTarget]() {
                    if (!m_isWindowVisible || !m_isLogOpen) return;
                    auto it = m_logTargetTextBlocks.find(id);
                    if (it != m_logTargetTextBlocks.end() && it->second)
                    {
                        it->second.Text(newTarget);
                    }
                });
            }
        });

        m_scrollInactivityTimer = DispatcherTimer();
        m_scrollInactivityTimer.Interval(std::chrono::seconds(5));
        m_scrollInactivityTimer.Tick({ this, &MainWindow::OnScrollInactivityTimerTick });

        if (LogScrollViewer())
        {
            LogScrollViewer().ViewChanging([this](auto const&, auto const&) { OnScrollInteraction(); });
            LogScrollViewer().ViewChanged([this](auto const&, auto const&) { OnScrollInteraction(); });
            LogScrollViewer().PointerWheelChanged([this](auto const&, auto const&) { OnScrollInteraction(); });
            LogScrollViewer().PointerPressed([this](auto const&, auto const&) { OnScrollInteraction(); });
        }

        m_viewModel->RegisterPropertyChangedCallback([this]() {
            UpdateLocalizedStrings();
            UpdateStatusDisplay();
            UpdateTrayTooltip();
        });

        UpdateLocalizedStrings();
        UpdateStatusDisplay();
    }

    void MainWindow::InitializeWindowPresenter()
    {
        auto presenter = this->AppWindow().Presenter().as<Microsoft::UI::Windowing::OverlappedPresenter>();
        if (presenter)
        {
            presenter.IsResizable(false);
            presenter.IsMaximizable(false);
            presenter.SetBorderAndTitleBar(true, false);
        }
        this->ExtendsContentIntoTitleBar(true);

        int borderX = GetWindowBorderX(static_cast<HWND>(m_hwnd));
        this->AppWindow().Resize({ 1200 + borderX, 700 });

        wchar_t buffer[MAX_PATH];
        ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        std::filesystem::path exePath(buffer);
        std::filesystem::path icoPath = exePath.parent_path() / L"Assets" / L"app.ico";
        if (!std::filesystem::exists(icoPath))
        {
            icoPath = exePath.parent_path() / L"app.ico";
        }
        if (std::filesystem::exists(icoPath))
        {
            this->AppWindow().SetIcon(icoPath.c_str());
        }
    }

    void MainWindow::InitializeComponents()
    {
        m_settingsBtn.Initialize(this->SettingsButton(), this->SettingsIcon());
        m_settingsBtn.SetColors(
            Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0),
            Microsoft::UI::ColorHelper::FromArgb(200, 30, 41, 59),
            Microsoft::UI::ColorHelper::FromArgb(255, 148, 163, 184),
            Microsoft::UI::ColorHelper::FromArgb(255, 248, 250, 252));
        m_settingsBtn.SetOnClick([this]() {
            this->SettingsOverlay().Visibility(Visibility::Visible);
            m_autoStartToggle.SetIsOn(m_viewModel->GetAutoStart(), false);
        });

        m_minimizeBtn.Initialize(this->MinimizeButton(), this->MinimizeIcon());
        m_minimizeBtn.SetColors(
            Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0),
            Microsoft::UI::ColorHelper::FromArgb(200, 30, 41, 59),
            Microsoft::UI::ColorHelper::FromArgb(255, 148, 163, 184),
            Microsoft::UI::ColorHelper::FromArgb(255, 248, 250, 252));
        m_minimizeBtn.SetOnClick([this]() {
            auto presenter = this->AppWindow().Presenter().try_as<Microsoft::UI::Windowing::OverlappedPresenter>();
            if (presenter)
            {
                presenter.Minimize();
                this->OnWindowHiddenToTray();
            }
        });

        m_closeBtn.Initialize(this->CloseButton(), this->CloseIcon());
        m_closeBtn.SetColors(
            Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0),
            Microsoft::UI::ColorHelper::FromArgb(230, 220, 38, 38),
            Microsoft::UI::ColorHelper::FromArgb(255, 148, 163, 184),
            Microsoft::UI::ColorHelper::FromArgb(255, 255, 255, 255));
        m_closeBtn.SetOnClick([this]() {
            this->ShowCloseConfirmation();
        });

        m_logToggleBtn.Initialize(this->LogToggleButton(), this->LogToggleIcon(), this->LogToggleText());
        m_logToggleBtn.SetColors(
            Microsoft::UI::ColorHelper::FromArgb(255, 16, 23, 38),
            Microsoft::UI::ColorHelper::FromArgb(255, 30, 41, 59),
            Microsoft::UI::ColorHelper::FromArgb(255, 148, 163, 184),
            Microsoft::UI::ColorHelper::FromArgb(255, 248, 250, 252));
        m_logToggleBtn.SetBorders(
            Microsoft::UI::ColorHelper::FromArgb(255, 31, 41, 61),
            Microsoft::UI::ColorHelper::FromArgb(255, 51, 65, 85));
        m_logToggleBtn.SetActiveColors(
            Microsoft::UI::ColorHelper::FromArgb(255, 3, 105, 161),
            Microsoft::UI::ColorHelper::FromArgb(255, 56, 189, 248),
            Microsoft::UI::ColorHelper::FromArgb(255, 255, 255, 255));
        m_logToggleBtn.SetOnClick([this]() {
            this->ToggleLogPanel();
        });

        m_logClearBtn.Initialize(this->LogClearBtn(), this->LogClearIcon(), this->LogClearText());
        m_logClearBtn.SetColors(
            Microsoft::UI::ColorHelper::FromArgb(255, 16, 23, 38),
            Microsoft::UI::ColorHelper::FromArgb(255, 30, 41, 59),
            Microsoft::UI::ColorHelper::FromArgb(255, 148, 163, 184),
            Microsoft::UI::ColorHelper::FromArgb(255, 56, 189, 248));
        m_logClearBtn.SetBorders(
            Microsoft::UI::ColorHelper::FromArgb(255, 31, 41, 61),
            Microsoft::UI::ColorHelper::FromArgb(255, 56, 189, 248));
        m_logClearBtn.SetOnClick([this]() {
            TrafficLogger::Instance().Clear();
            ClearLogVisuals();
        });

        m_settingsCloseBtn.Initialize(this->SettingsModalCloseIconBtn(), nullptr);
        m_settingsCloseBtn.SetColors(
            Microsoft::UI::ColorHelper::FromArgb(0, 0, 0, 0),
            Microsoft::UI::ColorHelper::FromArgb(255, 31, 41, 61),
            Microsoft::UI::ColorHelper::FromArgb(255, 148, 163, 184),
            Microsoft::UI::ColorHelper::FromArgb(255, 248, 250, 252));
        m_settingsCloseBtn.SetOnClick([this]() {
            CloseSettingsModal();
        });

        m_autoStartToggle.Initialize(this->AutoStartRow(), this->AutoStartToggle(), this->AutoStartKnob());
        m_autoStartToggle.SetIsOn(m_viewModel->GetAutoStart(), false);
        m_autoStartToggle.SetOnToggled([this](bool isOn) {
            m_viewModel->SetAutoStart(isOn);
        });

        m_presetDropdown.Initialize(this->PresetSelectTrigger(), this->PresetSelectedText(), this->PresetSelectChevron(), 462.0);
        m_presetDropdown.SetOnSelectionChanged([this](std::wstring const& tag) {
            int p = _wtoi(tag.c_str());
            m_viewModel->SetPreset(static_cast<DpiPreset>(p));
        });

        m_pingCountryDropdown.Initialize(this->PingCountrySelectTrigger(), this->PingCountrySelectedText(), this->PingCountrySelectChevron(), 462.0);
        m_pingCountryDropdown.SetOnSelectionChanged([this](std::wstring const& tag) {
            m_viewModel->SetPingCountry(tag);
            if (this->CountryPingBlock())
            {
                this->CountryPingBlock().Text(tag + L": -- ms");
            }
        });

        m_langDropdown.Initialize(this->LanguageSelectTrigger(), this->LanguageSelectedText(), this->LanguageSelectChevron(), 462.0);
        m_langDropdown.SetOnSelectionChanged([this](std::wstring const& tag) {
            auto lang = (tag == L"TR") ? AppLanguage::Turkish : AppLanguage::English;
            m_viewModel->SetLanguage(lang);
        });

        m_whitelistView.Initialize(this->WhitelistItemsList(), this->WhitelistInputBox(), this->WhitelistAddBtn(), this->WhitelistAddBtnText());
        m_whitelistView.SetOnAdd([this](std::wstring const& item) {
            m_viewModel->AddWhitelistItem(item);
            RefreshWhitelistView();
        });
        m_whitelistView.SetOnDelete([this](std::wstring const& item) {
            m_viewModel->RemoveWhitelistItem(item);
            RefreshWhitelistView();
        });
    }

    void MainWindow::ActionRing_StateToggled(IInspectable const&, bool const& isActive)
    {
        m_viewModel->SetRunning(isActive);
        UpdateStatusDisplay();
    }

    void MainWindow::SettingsBackdrop_PointerPressed(IInspectable const&, Input::PointerRoutedEventArgs const& e)
    {
        e.Handled(true);
        CloseSettingsModal();
    }

    void MainWindow::UpdateLocalizedStrings()
    {
        this->Title(m_viewModel->AppTitle());
        AppTitleBlock().Text(m_viewModel->AppTitle());
        PillText().Text(m_viewModel->StatusPillText());
        LogToggleText().Text(m_viewModel->LogButtonText());
        LogPanelTitleBlock().Text(m_viewModel->LogTitle());
        LogClearText().Text(m_viewModel->GetCurrentLanguage() == AppLanguage::Turkish ? L"Temizle" : L"Clear");

        SettingsModalTitle().Text(m_viewModel->SettingsTitle());
        SettingsIspTitle().Text(m_viewModel->SettingsIspPreset());
        SettingsIspDesc().Text(m_viewModel->SettingsIspPresetDesc());

        SettingsPingCountryTitle().Text(m_viewModel->SettingsPingCountry());
        SettingsPingCountryDesc().Text(m_viewModel->SettingsPingCountryDesc());

        SettingsLangTitle().Text(m_viewModel->SettingsLanguageLabel());
        SettingsLangDesc().Text(m_viewModel->SettingsLanguageDesc());
        SettingsAutoStartTitle().Text(m_viewModel->SettingsAutoStart());
        SettingsAutoStartDesc().Text(m_viewModel->SettingsAutoStartDesc());
        m_autoStartToggle.SetIsOn(m_viewModel->GetAutoStart(), false);

        SettingsWhitelistTitleBlock().Text(m_viewModel->SettingsWhitelistTitle());
        SettingsWhitelistDescBlock().Text(m_viewModel->SettingsWhitelistDesc());
        m_whitelistView.SetAddButtonText(m_viewModel->SettingsWhitelistAddBtn());
        m_whitelistView.SetInputPlaceholder(m_viewModel->SettingsWhitelistPlaceholder());

        m_presetDropdown.SetItems({
            { L"0", L"DEF", m_viewModel->GetPresetName(DpiPreset::TurkTelekomDefault) },
            { L"1", L"ALT 1", m_viewModel->GetPresetName(DpiPreset::SuperonlineAlt1) },
            { L"2", L"ALT 2", m_viewModel->GetPresetName(DpiPreset::SuperonlineAlt2) },
            { L"3", L"ALT 3", m_viewModel->GetPresetName(DpiPreset::SuperonlineAlt3) },
            { L"4", L"ALT 4", m_viewModel->GetPresetName(DpiPreset::SuperonlineAlt4) },
            { L"5", L"ALT 5", m_viewModel->GetPresetName(DpiPreset::VodafoneAlt5) },
            { L"6", L"ALT 6", m_viewModel->GetPresetName(DpiPreset::VodafoneAlt6) }
        });
        m_presetDropdown.SetSelectedItem(std::to_wstring(static_cast<int>(m_viewModel->GetPreset())));

        bool isTr = (m_viewModel->GetCurrentLanguage() == AppLanguage::Turkish);
        m_pingCountryDropdown.SetItems({
            { L"TR", L"TR", isTr ? L"Türkiye (İstanbul - 195.175.39.39)" : L"Turkey (Istanbul - 195.175.39.39)" },
            { L"UK", L"UK", isTr ? L"Birleşik Krallık (Londra - 8.8.4.4)" : L"United Kingdom (London - 8.8.4.4)" },
            { L"US", L"US", isTr ? L"Amerika Birleşik Devletleri (1.1.1.1)" : L"United States (1.1.1.1)" }
        });
        m_pingCountryDropdown.SetSelectedItem(m_viewModel->GetPingCountry());

        m_langDropdown.SetItems({
            { L"EN", L"EN", L"English (US)" },
            { L"TR", L"TR", L"Türkçe (TR)" }
        });
        m_langDropdown.SetSelectedItem(isTr ? L"TR" : L"EN");

        RefreshWhitelistView();

        Controls::ToolTipService::SetToolTip(SettingsButton(), box_value(m_viewModel->SettingsButtonText()));
        Controls::ToolTipService::SetToolTip(MinimizeButton(), box_value(m_viewModel->TitleBarMinimize()));
        Controls::ToolTipService::SetToolTip(CloseButton(), box_value(m_viewModel->TitleBarClose()));
    }

    void MainWindow::UpdateStatusDisplay()
    {
        PillText().Text(m_viewModel->StatusPillText());
        if (m_viewModel->IsRunning())
        {
            StatusPill().Background(SolidBrush(255, 6, 78, 59));
            StatusPill().BorderBrush(SolidBrush(255, 16, 185, 129));
            PillDot().Fill(SolidBrush(255, 52, 211, 153));
            PillText().Foreground(SolidBrush(255, 167, 243, 208));
        }
        else
        {
            StatusPill().Background(SolidBrush(255, 69, 10, 10));
            StatusPill().BorderBrush(SolidBrush(255, 153, 27, 27));
            PillDot().Fill(SolidBrush(255, 239, 68, 68));
            PillText().Foreground(SolidBrush(255, 252, 165, 165));
        }
        if (this->ActionRing().IsActive() != m_viewModel->IsRunning())
        {
            this->ActionRing().IsActive(m_viewModel->IsRunning());
        }
    }

    void MainWindow::ToggleLogPanel()
    {
        m_isLogOpen = !m_isLogOpen;
        m_logToggleBtn.SetActive(m_isLogOpen);
        m_targetWindowWidth = m_isLogOpen ? 1600.0 : 1200.0;
        if (m_isLogOpen)
        {
            PopulateInitialLogs();
        }
        else
        {
            ClearLogVisuals();
            ::SetProcessWorkingSetSize(::GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
        }
        if (!m_windowResizeTimer.IsEnabled())
        {
            m_windowResizeTimer.Start();
        }
    }

    void MainWindow::PopulateInitialLogs()
    {
        ClearLogVisuals();
        auto logs = TrafficLogger::Instance().GetLogs();
        for (auto const& item : logs)
        {
            AppendLogCard(item);
        }
    }

    void MainWindow::OnWindowResizeTimerTick(IInspectable const&, IInspectable const&)
    {
        int borderX = GetWindowBorderX(static_cast<HWND>(m_hwnd));

        double diff = m_targetWindowWidth - m_currentWindowWidth;
        if (std::abs(diff) < 1.0)
        {
            m_currentWindowWidth = m_targetWindowWidth;
            this->AppWindow().Resize({ static_cast<int32_t>(m_targetWindowWidth + borderX), 700 });
            m_windowResizeTimer.Stop();
        }
        else
        {
            m_currentWindowWidth += diff * 0.25;
            this->AppWindow().Resize({ static_cast<int32_t>(m_currentWindowWidth + borderX), 700 });
        }
        UpdateTitleBarRegions();
    }

    void MainWindow::OnScrollInteraction()
    {
        if (!LogScrollViewer()) return;

        double offset = LogScrollViewer().VerticalOffset();
        double scrollable = LogScrollViewer().ScrollableHeight();

        if (scrollable > 0 && offset >= scrollable - 15.0)
        {
            m_lastScrollInteractionTime = std::chrono::steady_clock::time_point{};
            if (m_scrollInactivityTimer && m_scrollInactivityTimer.IsEnabled())
            {
                m_scrollInactivityTimer.Stop();
            }
            return;
        }

        m_lastScrollInteractionTime = std::chrono::steady_clock::now();
        if (m_scrollInactivityTimer)
        {
            m_scrollInactivityTimer.Stop();
            m_scrollInactivityTimer.Start();
        }
    }

    void MainWindow::OnScrollInactivityTimerTick(IInspectable const&, IInspectable const&)
    {
        if (m_scrollInactivityTimer)
        {
            m_scrollInactivityTimer.Stop();
        }
        m_lastScrollInteractionTime = std::chrono::steady_clock::time_point{};
        if (LogScrollViewer())
        {
            LogScrollViewer().ChangeView(nullptr, LogScrollViewer().ScrollableHeight(), nullptr);
        }
    }

    void MainWindow::UpdateTitleBarRegions()
    {
        if (!m_hwnd) return;
        try
        {
            auto nonClientInputSrc = winrt::Microsoft::UI::Input::InputNonClientPointerSource::GetForWindowId(this->AppWindow().Id());
            if (!nonClientInputSrc) return;

            UINT dpi = ::GetDpiForWindow(static_cast<HWND>(m_hwnd));
            if (dpi == 0) dpi = 96;

            int titleBarHeightPx = ::MulDiv(50, dpi, 96);
            auto size = this->AppWindow().Size();

            int mainAreaWidthPx = ::MulDiv(1200, dpi, 96);
            int buttonAreaWidthPx = ::MulDiv(170, dpi, 96);
            int caption1Width = mainAreaWidthPx - buttonAreaWidthPx;
            if (caption1Width < 0) caption1Width = 0;

            std::vector<winrt::Windows::Graphics::RectInt32> captionRects;
            captionRects.push_back({ 0, 0, caption1Width, titleBarHeightPx });

            if (size.Width > mainAreaWidthPx)
            {
                int logCaptionWidth = size.Width - mainAreaWidthPx;
                captionRects.push_back({ mainAreaWidthPx, 0, logCaptionWidth, titleBarHeightPx });
            }

            winrt::Windows::Graphics::RectInt32 passthroughRect{ caption1Width, 0, buttonAreaWidthPx, titleBarHeightPx };

            nonClientInputSrc.SetRegionRects(winrt::Microsoft::UI::Input::NonClientRegionKind::Caption, captionRects);
            nonClientInputSrc.SetRegionRects(winrt::Microsoft::UI::Input::NonClientRegionKind::Passthrough, { passthroughRect });
        }
        catch (...) {}
    }

    void MainWindow::InitializeTrayIcon()
    {
        m_trayManager.SetOnRestore([this]() { RestoreWindow(); });
        m_trayManager.SetOnMinimize([this]() { OnWindowHiddenToTray(); });
        m_trayManager.SetOnToggleState([this]() { ToggleStateFromTray(); });
        m_trayManager.SetOnExit([this]() { ExitApplication(); });
        m_trayManager.SetGetMenuStrings([this](std::wstring& open, std::wstring& action, std::wstring& exit, bool& isRunning) {
            if (m_viewModel)
            {
                open = m_viewModel->TrayOpen();
                action = m_viewModel->IsRunning() ? m_viewModel->TrayStop() : m_viewModel->TrayStart();
                exit = m_viewModel->TrayExit();
                isRunning = m_viewModel->IsRunning();
            }
        });
        m_trayManager.Initialize(static_cast<HWND>(m_hwnd), m_restoreMsg);
    }

    void MainWindow::RemoveTrayIcon()
    {
        m_trayManager.Remove();
    }

    void MainWindow::UpdateTrayTooltip()
    {
        if (m_viewModel)
        {
            m_trayManager.UpdateTooltip(m_viewModel->TrayTip());
        }
    }

    void MainWindow::ShowTrayNotification()
    {
        if (m_viewModel)
        {
            m_trayManager.ShowNotification(m_viewModel->TrayNotificationTitle(), m_viewModel->TrayNotificationBody());
        }
    }

    winrt::fire_and_forget MainWindow::ShowCloseConfirmation()
    {
        auto dialog = Controls::ContentDialog();
        dialog.XamlRoot(this->Content().XamlRoot());
        dialog.Title(box_value(m_viewModel->CloseDialogTitle()));
        dialog.PrimaryButtonText(m_viewModel->CloseDialogMinimizeTray());
        dialog.SecondaryButtonText(m_viewModel->CloseDialogExit());
        dialog.CloseButtonText(m_viewModel->CloseDialogCancel());
        dialog.DefaultButton(Controls::ContentDialogButton::Primary);

        auto msgBlock = Controls::TextBlock();
        msgBlock.Text(m_viewModel->CloseDialogMessage());
        msgBlock.TextWrapping(TextWrapping::Wrap);
        msgBlock.FontSize(14.0);
        msgBlock.Foreground(SolidBrush(255, 203, 213, 225));
        msgBlock.Margin({ 0, 8, 0, 0 });
        msgBlock.Width(380.0);
        dialog.Content(msgBlock);

        auto result = co_await dialog.ShowAsync();
        if (result == Controls::ContentDialogResult::Primary)
        {
            if (m_hwnd)
            {
                ::ShowWindow(static_cast<HWND>(m_hwnd), SW_HIDE);
                ShowTrayNotification();
                OnWindowHiddenToTray();
            }
        }
        else if (result == Controls::ContentDialogResult::Secondary)
        {
            ExitApplication();
        }
    }

    void MainWindow::OnWindowHiddenToTray()
    {
        m_isWindowVisible = false;
        PingService::Instance().Stop();
        TrafficLogger::Instance().SetEnabled(false);
        TrafficLogger::Instance().Clear();
        ClearLogVisuals();
        this->ActionRing().PauseAnimation();
        if (this->AppRootGrid())
        {
            this->AppRootGrid().Visibility(Visibility::Collapsed);
        }
        ::SetProcessWorkingSetSize(::GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
        if (m_trimTimer)
        {
            m_trimTimer.Start();
        }
    }

    void MainWindow::RestoreWindow()
    {
        if (m_hwnd)
        {
            if (m_trimTimer && m_trimTimer.IsEnabled())
            {
                m_trimTimer.Stop();
            }
            if (this->AppRootGrid())
            {
                this->AppRootGrid().Visibility(Visibility::Visible);
            }
            this->AppWindow().Show();
            HWND hWnd = static_cast<HWND>(m_hwnd);
            ::ShowWindow(hWnd, SW_SHOW);
            ::ShowWindow(hWnd, SW_RESTORE);
            ::SetForegroundWindow(hWnd);
            this->Activate();
            m_isWindowVisible = true;
            this->UpdateTitleBarRegions();
            TrafficLogger::Instance().SetEnabled(true);
            PingService::Instance().Start();
            this->ActionRing().ResumeAnimation();
            if (m_isLogOpen)
            {
                PopulateInitialLogs();
            }
        }
    }

    void MainWindow::ToggleStateFromTray()
    {
        if (m_viewModel)
        {
            bool const newState = !m_viewModel->IsRunning();
            m_viewModel->SetRunning(newState);
            this->ActionRing().IsActive(m_viewModel->IsRunning());
            if (!m_isWindowVisible)
            {
                this->ActionRing().PauseAnimation();
            }
            UpdateStatusDisplay();
        }
    }

    void MainWindow::StartAutoService()
    {
        if (m_viewModel)
        {
            m_viewModel->SetRunning(true);
            this->ActionRing().IsActive(m_viewModel->IsRunning());
            if (!m_isWindowVisible)
            {
                this->ActionRing().PauseAnimation();
            }
            UpdateStatusDisplay();
        }
    }

    void MainWindow::ExitApplication()
    {
        PingService::Instance().Stop();
        DpiEngine::Instance().Stop();
        RemoveTrayIcon();
        this->Close();
    }

    void MainWindow::RefreshWhitelistView()
    {
        m_whitelistView.SetItems(m_viewModel->GetWhitelistItems(), m_viewModel->SettingsWhitelistEmpty());
    }

    void MainWindow::AppendLogCard(ConnectionLogItem const& logItem)
    {
        if (!LogItemsContainer()) return;
        TextBlock targetBlock{ nullptr };
        auto card = LogCardFactory::CreateCard(
            logItem,
            m_viewModel->IsWhitelisted(logItem.target),
            m_viewModel->LogStatusProcessed(),
            m_viewModel->LogStatusBypassed(),
            m_viewModel->LogAddWhitelist(),
            m_viewModel->LogRemoveWhitelist(),
            [this](std::wstring const& target) {
                if (m_viewModel->IsWhitelisted(target))
                    m_viewModel->RemoveWhitelistItem(target);
                else
                    m_viewModel->AddWhitelistItem(target);
                RefreshWhitelistView();
            },
            targetBlock);

        if (targetBlock)
        {
            m_logTargetTextBlocks[logItem.id] = targetBlock;
        }

        LogItemsContainer().Children().Append(card);
        while (LogItemsContainer().Children().Size() > 100)
        {
            LogItemsContainer().Children().RemoveAt(0);
        }
    }

    void MainWindow::ClearLogVisuals()
    {
        if (LogItemsContainer())
        {
            LogItemsContainer().Children().Clear();
        }
        m_logTargetTextBlocks.clear();
        m_lastScrollInteractionTime = std::chrono::steady_clock::time_point{};
        if (m_scrollInactivityTimer && m_scrollInactivityTimer.IsEnabled())
        {
            m_scrollInactivityTimer.Stop();
        }
    }

    void MainWindow::CloseSettingsModal()
    {
        if (SettingsOverlay())
        {
            SettingsOverlay().Visibility(Visibility::Collapsed);
        }
        m_presetDropdown.Close();
        m_pingCountryDropdown.Close();
        m_langDropdown.Close();
    }
}

#include "MainWindow.xaml.g.hpp"
