#include "pch.h"
#include "HoverButton.h"
#include "Utils/UiUtils.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace GoodByDpi_App::Utils;

namespace GoodByDpi_App::UI::Components
{
    HoverButton::HoverButton()
    {
    }

    void HoverButton::Initialize(
        Controls::Button button,
        Controls::FontIcon icon,
        Controls::TextBlock text)
    {
        if (m_isInitialized && m_button == button) return;

        m_button = button;
        m_icon = icon;
        m_text = text;

        if (!m_button) return;

        m_isInitialized = true;
        SetHandCursor(m_button);

        m_button.PointerEntered([this](auto const&, auto const&) {
            m_isHovered = true;
            UpdateVisuals();
        });

        m_button.PointerExited([this](auto const&, auto const&) {
            m_isHovered = false;
            UpdateVisuals();
        });

        m_button.Click([this](auto const&, auto const&) {
            if (m_onClick)
            {
                m_onClick();
            }
        });
    }

    void HoverButton::SetColors(
        Windows::UI::Color defaultBg,
        Windows::UI::Color hoverBg,
        Windows::UI::Color defaultFg,
        Windows::UI::Color hoverFg)
    {
        m_defaultBg = defaultBg;
        m_hoverBg = hoverBg;
        m_defaultFg = defaultFg;
        m_hoverFg = hoverFg;
        UpdateVisuals();
    }

    void HoverButton::SetBorders(
        Windows::UI::Color defaultBorder,
        Windows::UI::Color hoverBorder)
    {
        m_hasBorders = true;
        m_defaultBorder = defaultBorder;
        m_hoverBorder = hoverBorder;
        UpdateVisuals();
    }

    void HoverButton::SetActive(bool active)
    {
        m_isActive = active;
        UpdateVisuals();
    }

    void HoverButton::SetActiveColors(
        Windows::UI::Color activeBg,
        Windows::UI::Color activeBorder,
        Windows::UI::Color activeFg)
    {
        m_hasActiveState = true;
        m_activeBg = activeBg;
        m_activeBorder = activeBorder;
        m_activeFg = activeFg;
        UpdateVisuals();
    }

    void HoverButton::SetOnClick(std::function<void()> onClick)
    {
        m_onClick = onClick;
    }

    void HoverButton::UpdateVisuals()
    {
        if (!m_button) return;

        if (m_hasActiveState && m_isActive)
        {
            m_button.Background(SolidBrush(m_activeBg));
            if (m_hasBorders)
            {
                m_button.BorderBrush(SolidBrush(m_activeBorder));
            }
            if (m_icon)
            {
                m_icon.Foreground(SolidBrush(m_activeFg));
            }
            if (m_text)
            {
                m_text.Foreground(SolidBrush(m_activeFg));
            }
            return;
        }

        if (m_isHovered)
        {
            m_button.Background(SolidBrush(m_hoverBg));
            if (m_hasBorders)
            {
                m_button.BorderBrush(SolidBrush(m_hoverBorder));
            }
            if (m_icon)
            {
                m_icon.Foreground(SolidBrush(m_hoverFg));
            }
            if (m_text)
            {
                m_text.Foreground(SolidBrush(m_hoverFg));
            }
        }
        else
        {
            m_button.Background(SolidBrush(m_defaultBg));
            if (m_hasBorders)
            {
                m_button.BorderBrush(SolidBrush(m_defaultBorder));
            }
            if (m_icon)
            {
                m_icon.Foreground(SolidBrush(m_defaultFg));
            }
            if (m_text)
            {
                m_text.Foreground(SolidBrush(m_defaultFg));
            }
        }
    }
}
