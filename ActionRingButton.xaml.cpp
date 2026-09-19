#include "pch.h"
#include "ActionRingButton.xaml.h"
#if __has_include("ActionRingButton.g.cpp")
#include "ActionRingButton.g.cpp"
#endif
#include <cmath>
#include <algorithm>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;

namespace winrt::GoodByDpi_App::implementation
{
    ActionRingButton::ActionRingButton()
    {
        InitializeComponent();
        MainActionButton().as<winrt::Microsoft::UI::Xaml::IUIElementProtected>().ProtectedCursor(winrt::Microsoft::UI::Input::InputSystemCursor::Create(winrt::Microsoft::UI::Input::InputSystemCursorShape::Hand));
        InitializeAnimationTimer();
        UpdateRingGeometry(120.0);
        UpdateVisualTheme();
    }

    bool ActionRingButton::IsActive()
    {
        return m_isActive;
    }

    void ActionRingButton::IsActive(bool value)
    {
        if (m_isActive != value)
        {
            m_isActive = value;
            m_targetSplit = m_isActive ? 1.0 : 0.0;
            m_targetVelocity = m_isActive ? 5.5 : 0.0;
            if (!m_animTimer.IsEnabled())
            {
                m_animTimer.Start();
            }
            UpdateVisualTheme();
        }
    }

    winrt::event_token ActionRingButton::StateToggled(winrt::Windows::Foundation::EventHandler<bool> const& handler)
    {
        return m_stateToggledEvent.add(handler);
    }

    void ActionRingButton::StateToggled(winrt::event_token const& token) noexcept
    {
        m_stateToggledEvent.remove(token);
    }

    void ActionRingButton::PauseAnimation()
    {
        if (m_animTimer && m_animTimer.IsEnabled())
        {
            m_animTimer.Stop();
        }
    }

    void ActionRingButton::ResumeAnimation()
    {
        if (m_isActive)
        {
            if (m_animTimer && !m_animTimer.IsEnabled())
            {
                m_animTimer.Start();
            }
        }
    }

    void ActionRingButton::InitializeAnimationTimer()
    {
        m_animTimer = DispatcherTimer();
        m_animTimer.Interval(std::chrono::milliseconds(16));
        m_animTimer.Tick({ this, &ActionRingButton::OnAnimationTick });
    }

    void ActionRingButton::MainActionButton_Click(winrt::Windows::Foundation::IInspectable const&, RoutedEventArgs const&)
    {
        IsActive(!m_isActive);
        m_stateToggledEvent(*this, m_isActive);
    }

    void ActionRingButton::MainActionButton_PointerEntered(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&)
    {
        m_targetHover = 1.0;
        if (!m_animTimer.IsEnabled())
        {
            m_animTimer.Start();
        }
    }

    void ActionRingButton::MainActionButton_PointerExited(winrt::Windows::Foundation::IInspectable const&, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const&)
    {
        m_targetHover = 0.0;
        if (!m_animTimer.IsEnabled())
        {
            m_animTimer.Start();
        }
    }

    void ActionRingButton::OnAnimationTick(winrt::Windows::Foundation::IInspectable const&, winrt::Windows::Foundation::IInspectable const&)
    {
        double const splitFriction = m_isActive ? 0.07 : 0.05;
        m_splitProgress += (m_targetSplit - m_splitProgress) * splitFriction;

        double const hoverFriction = 0.18;
        m_hoverProgress += (m_targetHover - m_hoverProgress) * hoverFriction;

        double const spanDegrees = 120.0 - (m_splitProgress * 36.0);
        UpdateRingGeometry(spanDegrees);

        UpdateVisualTheme();

        double const accelFriction = m_isActive ? 0.05 : 0.03;
        m_currentVelocity += (m_targetVelocity - m_currentVelocity) * accelFriction;
        m_currentAngle += m_currentVelocity;
        if (m_currentAngle >= 360.0)
        {
            m_currentAngle -= 360.0;
        }
        RingRotation().Angle(m_currentAngle);

        if (m_isActive)
        {
            if (std::abs(m_targetSplit - m_splitProgress) < 0.005)
            {
                m_splitProgress = 1.0;
            }
            if (std::abs(m_targetHover - m_hoverProgress) < 0.005)
            {
                m_hoverProgress = m_targetHover;
            }
        }
        else
        {
            if (m_splitProgress < 0.005)
            {
                m_splitProgress = 0.0;
            }
            if (std::abs(m_targetHover - m_hoverProgress) < 0.005)
            {
                m_hoverProgress = m_targetHover;
            }
            if (m_currentVelocity < 0.02)
            {
                m_currentVelocity = 0.0;
                m_currentAngle = 0.0;
                RingRotation().Angle(0.0);
                if (m_splitProgress == 0.0 && m_hoverProgress == m_targetHover)
                {
                    m_animTimer.Stop();
                }
            }
        }
    }

    void ActionRingButton::UpdateRingGeometry(double spanDegrees)
    {
        auto pathGeometry = winrt::Microsoft::UI::Xaml::Media::PathGeometry();
        double const centerX = 170.0;
        double const centerY = 170.0;
        double const radius = 120.0;
        double const halfSpan = spanDegrees / 2.0;
        double const pi = 3.14159265358979323846;

        for (int i = 0; i < 3; ++i)
        {
            double const centerAngle = 270.0 + i * 120.0;
            double const startAngle = centerAngle - halfSpan;
            double const endAngle = centerAngle + halfSpan;

            double const startRad = startAngle * (pi / 180.0);
            double const endRad = endAngle * (pi / 180.0);

            float const startX = static_cast<float>(centerX + radius * std::cos(startRad));
            float const startY = static_cast<float>(centerY + radius * std::sin(startRad));
            float const endX = static_cast<float>(centerX + radius * std::cos(endRad));
            float const endY = static_cast<float>(centerY + radius * std::sin(endRad));

            auto figure = winrt::Microsoft::UI::Xaml::Media::PathFigure();
            figure.StartPoint({ startX, startY });
            figure.IsClosed(false);

            auto arc = winrt::Microsoft::UI::Xaml::Media::ArcSegment();
            arc.Point({ endX, endY });
            arc.Size({ static_cast<float>(radius), static_cast<float>(radius) });
            arc.SweepDirection(winrt::Microsoft::UI::Xaml::Media::SweepDirection::Clockwise);
            arc.IsLargeArc(spanDegrees > 180.0);
            arc.RotationAngle(0.0);

            figure.Segments().Append(arc);
            pathGeometry.Figures().Append(figure);
        }

        if (spanDegrees >= 119.5)
        {
            SingleRing().StrokeStartLineCap(winrt::Microsoft::UI::Xaml::Media::PenLineCap::Flat);
            SingleRing().StrokeEndLineCap(winrt::Microsoft::UI::Xaml::Media::PenLineCap::Flat);
        }
        else
        {
            SingleRing().StrokeStartLineCap(winrt::Microsoft::UI::Xaml::Media::PenLineCap::Round);
            SingleRing().StrokeEndLineCap(winrt::Microsoft::UI::Xaml::Media::PenLineCap::Round);
        }

        SingleRing().Data(pathGeometry);
    }

    void ActionRingButton::UpdateVisualTheme()
    {
        double const s = std::clamp(m_splitProgress, 0.0, 1.0);
        double const h = std::clamp(m_hoverProgress, 0.0, 1.0);

        double const rStop = 239.0 + (248.0 - 239.0) * h;
        double const gStop = 68.0 + (113.0 - 68.0) * h;
        double const bStop = 68.0 + (113.0 - 68.0) * h;

        double const rRun = 16.0 + (52.0 - 16.0) * h;
        double const gRun = 185.0 + (211.0 - 185.0) * h;
        double const bRun = 129.0 + (153.0 - 129.0) * h;

        uint8_t const ringR = static_cast<uint8_t>(rStop * (1.0 - s) + rRun * s);
        uint8_t const ringG = static_cast<uint8_t>(gStop * (1.0 - s) + gRun * s);
        uint8_t const ringB = static_cast<uint8_t>(bStop * (1.0 - s) + bRun * s);

        SingleRing().Stroke(SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(255, ringR, ringG, ringB)));
        ButtonGlow().Fill(SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(255, ringR, ringG, ringB)));
        ButtonGlow().Opacity(0.20 + 0.35 * h);
        MainActionButton().BorderBrush(SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(255, ringR, ringG, ringB)));

        double const r1Stop = 127.0 + (185.0 - 127.0) * h;
        double const g1Stop = 29.0 + (28.0 - 29.0) * h;
        double const b1Stop = 29.0 + (28.0 - 29.0) * h;

        double const r1Run = 5.0 + (16.0 - 5.0) * h;
        double const g1Run = 150.0 + (185.0 - 150.0) * h;
        double const b1Run = 105.0 + (129.0 - 105.0) * h;

        uint8_t const r1 = static_cast<uint8_t>(r1Stop * (1.0 - s) + r1Run * s);
        uint8_t const g1 = static_cast<uint8_t>(g1Stop * (1.0 - s) + g1Run * s);
        uint8_t const b1 = static_cast<uint8_t>(b1Stop * (1.0 - s) + b1Run * s);

        double const r2Stop = 69.0 + (127.0 - 69.0) * h;
        double const g2Stop = 10.0 + (29.0 - 10.0) * h;
        double const b2Stop = 10.0 + (29.0 - 10.0) * h;

        double const r2Run = 4.0 + (5.0 - 4.0) * h;
        double const g2Run = 120.0 + (150.0 - 120.0) * h;
        double const b2Run = 87.0 + (105.0 - 87.0) * h;

        uint8_t const r2 = static_cast<uint8_t>(r2Stop * (1.0 - s) + r2Run * s);
        uint8_t const g2 = static_cast<uint8_t>(g2Stop * (1.0 - s) + g2Run * s);
        uint8_t const b2 = static_cast<uint8_t>(b2Stop * (1.0 - s) + b2Run * s);

        LinearGradientBrush buttonBg;
        buttonBg.StartPoint({ 0.5, 0.0 });
        buttonBg.EndPoint({ 0.5, 1.0 });
        GradientStop stop1, stop2;
        stop1.Offset(0.0);
        stop1.Color(Microsoft::UI::ColorHelper::FromArgb(255, r1, g1, b1));
        stop2.Offset(1.0);
        stop2.Color(Microsoft::UI::ColorHelper::FromArgb(255, r2, g2, b2));
        buttonBg.GradientStops().Append(stop1);
        buttonBg.GradientStops().Append(stop2);
        MainActionButton().Background(buttonBg);

        double const iconRStop = 252.0 + (255.0 - 252.0) * h;
        double const iconGStop = 165.0 + (255.0 - 165.0) * h;
        double const iconBStop = 165.0 + (255.0 - 165.0) * h;

        uint8_t const iconR = static_cast<uint8_t>(iconRStop * (1.0 - s) + 255.0 * s);
        uint8_t const iconG = static_cast<uint8_t>(iconGStop * (1.0 - s) + 255.0 * s);
        uint8_t const iconB = static_cast<uint8_t>(iconBStop * (1.0 - s) + 255.0 * s);
        ActionIcon().Foreground(SolidColorBrush(Microsoft::UI::ColorHelper::FromArgb(255, iconR, iconG, iconB)));
    }
}

#include "ActionRingButton.xaml.g.hpp"
