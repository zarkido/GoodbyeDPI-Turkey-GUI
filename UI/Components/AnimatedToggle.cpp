#include "pch.h"
#include "AnimatedToggle.h"
#include "Utils/UiUtils.h"
#include <cmath>
#include <algorithm>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace GoodByDpi_App::Utils;

namespace GoodByDpi_App::UI::Components
{
    AnimatedToggle::AnimatedToggle()
    {
    }

    void AnimatedToggle::Initialize(
        winrt::Microsoft::UI::Xaml::UIElement clickTarget,
        winrt::Microsoft::UI::Xaml::Controls::Border trackBorder,
        winrt::Microsoft::UI::Xaml::Shapes::Ellipse knob)
    {
        if (m_isInitialized && m_clickTarget == clickTarget && m_trackBorder == trackBorder) return;

        m_clickTarget = clickTarget;
        m_trackBorder = trackBorder;
        m_knob = knob;
        m_isInitialized = true;

        if (m_clickTarget)
        {
            SetHandCursor(m_clickTarget);
            m_clickTarget.PointerPressed([this](auto const&, auto const& e) {
                e.Handled(true);
                SetIsOn(!m_isOn, true);
                if (m_onToggled)
                {
                    m_onToggled(m_isOn);
                }
            });
        }
        else if (m_trackBorder)
        {
            SetHandCursor(m_trackBorder);
            m_trackBorder.PointerPressed([this](auto const&, auto const& e) {
                e.Handled(true);
                SetIsOn(!m_isOn, true);
                if (m_onToggled)
                {
                    m_onToggled(m_isOn);
                }
            });
        }

        m_timer = DispatcherTimer();
        m_timer.Interval(std::chrono::milliseconds(16));
        m_timer.Tick({ this, &AnimatedToggle::OnTimerTick });

        UpdateVisuals();
    }

    void AnimatedToggle::SetIsOn(bool isOn, bool animate)
    {
        m_isOn = isOn;
        m_targetPos = isOn ? 1.0 : 0.0;
        if (!animate)
        {
            m_currentPos = m_targetPos;
            if (m_timer && m_timer.IsEnabled())
            {
                m_timer.Stop();
            }
            UpdateVisuals();
        }
        else
        {
            StartAnimation();
        }
    }

    bool AnimatedToggle::IsOn() const
    {
        return m_isOn;
    }

    void AnimatedToggle::SetOnToggled(std::function<void(bool)> callback)
    {
        m_onToggled = callback;
    }

    void AnimatedToggle::StartAnimation()
    {
        if (m_timer && !m_timer.IsEnabled())
        {
            m_timer.Start();
        }
    }

    void AnimatedToggle::OnTimerTick(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&)
    {
        double const friction = 0.22;
        m_currentPos += (m_targetPos - m_currentPos) * friction;

        UpdateVisuals();

        if (std::abs(m_targetPos - m_currentPos) < 0.005)
        {
            m_currentPos = m_targetPos;
            UpdateVisuals();
            if (m_timer)
            {
                m_timer.Stop();
            }
        }
    }

    void AnimatedToggle::UpdateVisuals()
    {
        if (m_knob)
        {
            Canvas::SetLeft(m_knob, 4.0 + m_currentPos * 20.0);

            uint8_t const kR = static_cast<uint8_t>(148.0 + m_currentPos * (255.0 - 148.0));
            uint8_t const kG = static_cast<uint8_t>(163.0 + m_currentPos * (255.0 - 163.0));
            uint8_t const kB = static_cast<uint8_t>(184.0 + m_currentPos * (255.0 - 184.0));
            m_knob.Fill(SolidBrush(255, kR, kG, kB));
        }

        if (m_trackBorder)
        {
            uint8_t const bgR = static_cast<uint8_t>(30.0 + m_currentPos * (5.0 - 30.0));
            uint8_t const bgG = static_cast<uint8_t>(41.0 + m_currentPos * (150.0 - 41.0));
            uint8_t const bgB = static_cast<uint8_t>(59.0 + m_currentPos * (105.0 - 59.0));
            m_trackBorder.Background(SolidBrush(255, bgR, bgG, bgB));

            uint8_t const brR = static_cast<uint8_t>(51.0 + m_currentPos * (16.0 - 51.0));
            uint8_t const brG = static_cast<uint8_t>(65.0 + m_currentPos * (185.0 - 65.0));
            uint8_t const brB = static_cast<uint8_t>(85.0 + m_currentPos * (129.0 - 85.0));
            m_trackBorder.BorderBrush(SolidBrush(255, brR, brG, brB));
        }
    }
}
