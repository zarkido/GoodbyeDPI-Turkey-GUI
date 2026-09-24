#pragma once
#include <functional>
#include <winrt/base.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>

namespace GoodByDpi_App::UI::Components
{
    class HoverButton
    {
    public:
        HoverButton();

        void Initialize(
            winrt::Microsoft::UI::Xaml::Controls::Button button,
            winrt::Microsoft::UI::Xaml::Controls::FontIcon icon,
            winrt::Microsoft::UI::Xaml::Controls::TextBlock text = nullptr);

        void SetColors(
            winrt::Windows::UI::Color defaultBg,
            winrt::Windows::UI::Color hoverBg,
            winrt::Windows::UI::Color defaultFg,
            winrt::Windows::UI::Color hoverFg);

        void SetBorders(
            winrt::Windows::UI::Color defaultBorder,
            winrt::Windows::UI::Color hoverBorder);

        void SetActive(bool active);
        void SetActiveColors(
            winrt::Windows::UI::Color activeBg,
            winrt::Windows::UI::Color activeBorder,
            winrt::Windows::UI::Color activeFg);

        void SetOnClick(std::function<void()> onClick);

    private:
        void UpdateVisuals();

        winrt::Microsoft::UI::Xaml::Controls::Button m_button{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::FontIcon m_icon{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_text{ nullptr };

        bool m_isInitialized{ false };
        bool m_isHovered{ false };
        bool m_isActive{ false };
        bool m_hasActiveState{ false };
        bool m_hasBorders{ false };

        winrt::Windows::UI::Color m_defaultBg;
        winrt::Windows::UI::Color m_hoverBg;
        winrt::Windows::UI::Color m_defaultFg;
        winrt::Windows::UI::Color m_hoverFg;
        winrt::Windows::UI::Color m_defaultBorder;
        winrt::Windows::UI::Color m_hoverBorder;
        winrt::Windows::UI::Color m_activeBg;
        winrt::Windows::UI::Color m_activeBorder;
        winrt::Windows::UI::Color m_activeFg;

        std::function<void()> m_onClick;
    };
}
