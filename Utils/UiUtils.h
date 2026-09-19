#pragma once
#include <algorithm>
#include <cstdint>
#include <winrt/Windows.UI.h>
#include <winrt/Microsoft.UI.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Input.h>

namespace GoodByDpi_App::Utils
{
    inline void SetHandCursor(winrt::Microsoft::UI::Xaml::UIElement const& element)
    {
        if (element)
        {
            element.as<winrt::Microsoft::UI::Xaml::IUIElementProtected>().ProtectedCursor(
                winrt::Microsoft::UI::Input::InputSystemCursor::Create(winrt::Microsoft::UI::Input::InputSystemCursorShape::Hand));
        }
    }

    inline winrt::Windows::UI::Color LerpColor(winrt::Windows::UI::Color const& from, winrt::Windows::UI::Color const& to, double t)
    {
        double factor = std::clamp(t, 0.0, 1.0);
        uint8_t a = static_cast<uint8_t>(from.A + factor * (to.A - from.A));
        uint8_t r = static_cast<uint8_t>(from.R + factor * (to.R - from.R));
        uint8_t g = static_cast<uint8_t>(from.G + factor * (to.G - from.G));
        uint8_t b = static_cast<uint8_t>(from.B + factor * (to.B - from.B));
        return winrt::Microsoft::UI::ColorHelper::FromArgb(a, r, g, b);
    }

    inline winrt::Microsoft::UI::Xaml::Media::SolidColorBrush SolidBrush(winrt::Windows::UI::Color const& color)
    {
        return winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(color);
    }

    inline winrt::Microsoft::UI::Xaml::Media::SolidColorBrush SolidBrush(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
    {
        return winrt::Microsoft::UI::Xaml::Media::SolidColorBrush(winrt::Microsoft::UI::ColorHelper::FromArgb(a, r, g, b));
    }

    inline int GetWindowBorderX(HWND hwnd)
    {
        if (!hwnd) return 16;
        RECT rcClient{}, rcWind{};
        ::GetClientRect(hwnd, &rcClient);
        ::GetWindowRect(hwnd, &rcWind);
        int borderX = (rcWind.right - rcWind.left) - (rcClient.right - rcClient.left);
        return (borderX <= 0 || borderX > 100) ? 16 : borderX;
    }
}
