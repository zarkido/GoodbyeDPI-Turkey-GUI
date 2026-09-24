#pragma once
#include <functional>
#include <winrt/base.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>

namespace GoodByDpi_App::UI::Components
{
    class AnimatedToggle
    {
    public:
        AnimatedToggle();

        void Initialize(
            winrt::Microsoft::UI::Xaml::UIElement clickTarget,
            winrt::Microsoft::UI::Xaml::Controls::Border trackBorder,
            winrt::Microsoft::UI::Xaml::Shapes::Ellipse knob);

        void SetIsOn(bool isOn, bool animate = true);
        bool IsOn() const;

        void SetOnToggled(std::function<void(bool)> callback);

    private:
        void StartAnimation();
        void OnTimerTick(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&);
        void UpdateVisuals();

        winrt::Microsoft::UI::Xaml::UIElement m_clickTarget{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Border m_trackBorder{ nullptr };
        winrt::Microsoft::UI::Xaml::Shapes::Ellipse m_knob{ nullptr };
        winrt::Microsoft::UI::Xaml::DispatcherTimer m_timer{ nullptr };

        bool m_isInitialized{ false };
        bool m_isOn{ false };
        double m_currentPos{ 0.0 };
        double m_targetPos{ 0.0 };
        std::function<void(bool)> m_onToggled;
    };
}
