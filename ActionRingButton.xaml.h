#pragma once
#include "ActionRingButton.g.h"
#include "ActionRingButton.xaml.g.h"

namespace winrt::GoodByDpi_App::implementation
{
    struct ActionRingButton : ActionRingButtonT<ActionRingButton>
    {
        ActionRingButton();

        bool IsActive();
        void IsActive(bool value);
        winrt::event_token StateToggled(winrt::Windows::Foundation::EventHandler<bool> const& handler);
        void StateToggled(winrt::event_token const& token) noexcept;
        void PauseAnimation();
        void ResumeAnimation();

        void MainActionButton_Click(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::RoutedEventArgs const&);
        void MainActionButton_PointerEntered(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);
        void MainActionButton_PointerExited(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&);

    private:
        void InitializeAnimationTimer();
        void OnAnimationTick(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&);
        void UpdateRingGeometry(double spanDegrees);
        void UpdateVisualTheme();

        bool m_isActive{ false };
        double m_hoverProgress{ 0.0 };
        double m_targetHover{ 0.0 };
        double m_splitProgress{ 0.0 };
        double m_targetSplit{ 0.0 };
        double m_currentAngle{ 0.0 };
        double m_currentVelocity{ 0.0 };
        double m_targetVelocity{ 0.0 };
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_animTimer{ nullptr };
        winrt::event<winrt::Windows::Foundation::EventHandler<bool>> m_stateToggledEvent;
    };
}

namespace winrt::GoodByDpi_App::factory_implementation
{
    struct ActionRingButton : ActionRingButtonT<ActionRingButton, implementation::ActionRingButton>
    {
    };
}
