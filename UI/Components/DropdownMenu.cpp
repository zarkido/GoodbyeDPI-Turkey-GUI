#include "pch.h"
#include "DropdownMenu.h"
#include "Utils/UiUtils.h"
#include <winrt/Microsoft.UI.Text.h>
#include <winrt/Windows.UI.Xaml.Interop.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace GoodByDpi_App::Utils;

namespace GoodByDpi_App::UI::Components
{
    DropdownMenu::DropdownMenu()
    {
    }

    void DropdownMenu::Initialize(
        Controls::Border trigger,
        Controls::TextBlock selectedText,
        Controls::FontIcon chevron,
        double dropdownWidth)
    {
        m_trigger = trigger;
        m_selectedText = selectedText;
        m_chevron = chevron;
        m_dropdownWidth = dropdownWidth;

        if (!m_trigger) return;

        SetHandCursor(m_trigger);

        BrushTransition trans;
        trans.Duration(std::chrono::milliseconds(180));
        m_trigger.BackgroundTransition(trans);

        m_trigger.PointerPressed([this](auto const&, auto const& e) {
            e.Handled(true);
            if (m_flyout)
            {
                if (m_flyout.IsOpen())
                {
                    m_flyout.Hide();
                }
                else
                {
                    m_flyout.ShowAt(m_trigger);
                }
            }
        });

        m_trigger.PointerEntered([this](auto const&, auto const&) {
            if (!m_flyout || !m_flyout.IsOpen())
            {
                m_trigger.Background(SolidBrush(255, 20, 30, 48));
                m_trigger.BorderBrush(SolidBrush(255, 45, 59, 85));
            }
        });

        m_trigger.PointerExited([this](auto const&, auto const&) {
            if (!m_flyout || !m_flyout.IsOpen())
            {
                m_trigger.Background(SolidBrush(255, 16, 23, 38));
                m_trigger.BorderBrush(SolidBrush(255, 31, 41, 61));
            }
        });
    }

    void DropdownMenu::SetItems(std::vector<DropdownItem> const& items)
    {
        m_items = items;
        BuildFlyout();
        UpdateSelectionVisuals();
    }

    void DropdownMenu::SetSelectedItem(std::wstring const& tag)
    {
        m_selectedTag = tag;
        UpdateSelectionVisuals();
    }

    void DropdownMenu::SetSelectedText(std::wstring const& text)
    {
        if (m_selectedText)
        {
            m_selectedText.Text(text);
        }
    }

    std::wstring DropdownMenu::GetSelectedTag() const
    {
        return m_selectedTag;
    }

    void DropdownMenu::SetOnSelectionChanged(std::function<void(std::wstring const& tag)> callback)
    {
        m_onSelectionChanged = callback;
    }

    void DropdownMenu::Close()
    {
        if (m_flyout && m_flyout.IsOpen())
        {
            m_flyout.Hide();
        }
    }

    void DropdownMenu::BuildFlyout()
    {
        if (!m_trigger) return;

        m_flyout = Controls::Flyout();
        m_flyout.Placement(Primitives::FlyoutPlacementMode::BottomEdgeAlignedLeft);

        auto style = Style(winrt::xaml_typename<Controls::FlyoutPresenter>());
        style.Setters().Append(Setter(Control::BackgroundProperty(), SolidBrush(0, 0, 0, 0)));
        style.Setters().Append(Setter(Control::BorderBrushProperty(), SolidBrush(0, 0, 0, 0)));
        style.Setters().Append(Setter(Control::BorderThicknessProperty(), box_value(ThicknessHelper::FromUniformLength(0))));
        style.Setters().Append(Setter(Control::PaddingProperty(), box_value(ThicknessHelper::FromUniformLength(0))));
        style.Setters().Append(Setter(FrameworkElement::MinWidthProperty(), box_value(m_dropdownWidth)));
        style.Setters().Append(Setter(FrameworkElement::MaxWidthProperty(), box_value(m_dropdownWidth)));
        style.Setters().Append(Setter(Controls::ScrollViewer::HorizontalScrollBarVisibilityProperty(), box_value(Controls::ScrollBarVisibility::Disabled)));
        style.Setters().Append(Setter(Controls::ScrollViewer::VerticalScrollBarVisibilityProperty(), box_value(Controls::ScrollBarVisibility::Disabled)));
        m_flyout.FlyoutPresenterStyle(style);

        m_flyout.Opened([this](auto const&, auto const&) {
            if (m_chevron)
            {
                m_chevron.Glyph(L"\uE70E");
            }
            if (m_trigger)
            {
                m_trigger.BorderBrush(SolidBrush(255, 56, 189, 248));
            }
        });

        m_flyout.Closed([this](auto const&, auto const&) {
            if (m_chevron)
            {
                m_chevron.Glyph(L"\uE70D");
            }
            if (m_trigger)
            {
                m_trigger.BorderBrush(SolidBrush(255, 31, 41, 61));
                m_trigger.Background(SolidBrush(255, 16, 23, 38));
            }
        });

        auto listBorder = Controls::Border();
        listBorder.Background(SolidBrush(255, 13, 19, 34));
        listBorder.BorderBrush(SolidBrush(255, 45, 59, 85));
        listBorder.BorderThickness(ThicknessHelper::FromUniformLength(1));
        listBorder.CornerRadius(CornerRadiusHelper::FromUniformRadius(8));
        listBorder.Padding(ThicknessHelper::FromUniformLength(4));
        listBorder.Width(m_dropdownWidth);

        m_itemsPanel = Controls::StackPanel();
        m_itemsPanel.Spacing(2);

        m_checkIcons.clear();
        m_itemTexts.clear();

        for (auto const& item : m_items)
        {
            auto itemBorder = Controls::Border();
            itemBorder.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
            itemBorder.Padding({ 10, 8, 10, 8 });
            itemBorder.Background(SolidBrush(0, 0, 0, 0));
            SetHandCursor(itemBorder);

            BrushTransition itemTrans;
            itemTrans.Duration(std::chrono::milliseconds(180));
            itemBorder.BackgroundTransition(itemTrans);

            itemBorder.PointerEntered([itemBorder](auto const&, auto const&) {
                itemBorder.Background(SolidBrush(255, 22, 34, 56));
            });

            itemBorder.PointerExited([itemBorder](auto const&, auto const&) {
                itemBorder.Background(SolidBrush(0, 0, 0, 0));
            });

            auto grid = Controls::Grid();
            auto c0 = Controls::ColumnDefinition();
            c0.Width(GridLengthHelper::Auto());
            auto c1 = Controls::ColumnDefinition();
            c1.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
            auto c2 = Controls::ColumnDefinition();
            c2.Width(GridLengthHelper::Auto());
            grid.ColumnDefinitions().Append(c0);
            grid.ColumnDefinitions().Append(c1);
            grid.ColumnDefinitions().Append(c2);

            if (!item.badge.empty())
            {
                auto badgeBorder = Controls::Border();
                badgeBorder.Background(SolidBrush(255, 22, 34, 56));
                badgeBorder.BorderBrush(SolidBrush(255, 45, 59, 85));
                badgeBorder.BorderThickness(ThicknessHelper::FromUniformLength(1));
                badgeBorder.CornerRadius(CornerRadiusHelper::FromUniformRadius(4));
                badgeBorder.Padding({ 5, 2, 5, 2 });
                badgeBorder.Margin({ 0, 0, 10, 0 });

                auto badgeText = Controls::TextBlock();
                badgeText.Text(item.badge);
                badgeText.FontSize(10.0);
                badgeText.FontWeight(Microsoft::UI::Text::FontWeights::Bold());
                badgeText.Foreground(SolidBrush(255, 56, 189, 248));
                badgeBorder.Child(badgeText);

                Controls::Grid::SetColumn(badgeBorder, 0);
                grid.Children().Append(badgeBorder);
            }

            auto textBlock = Controls::TextBlock();
            textBlock.Text(item.text);
            textBlock.FontSize(12.0);
            textBlock.FontWeight(Microsoft::UI::Text::FontWeights::Medium());
            textBlock.Foreground(SolidBrush(255, 248, 250, 252));
            textBlock.VerticalAlignment(VerticalAlignment::Center);
            textBlock.TextTrimming(TextTrimming::CharacterEllipsis);
            Controls::Grid::SetColumn(textBlock, 1);
            grid.Children().Append(textBlock);
            m_itemTexts[item.tag] = textBlock;

            auto checkIcon = Controls::FontIcon();
            checkIcon.Glyph(L"\uE73E");
            checkIcon.FontSize(12.0);
            checkIcon.Foreground(SolidBrush(255, 56, 189, 248));
            checkIcon.VerticalAlignment(VerticalAlignment::Center);
            checkIcon.Visibility(item.tag == m_selectedTag ? Visibility::Visible : Visibility::Collapsed);
            Controls::Grid::SetColumn(checkIcon, 2);
            grid.Children().Append(checkIcon);
            m_checkIcons[item.tag] = checkIcon;

            itemBorder.Child(grid);

            std::wstring itemTag = item.tag;
            itemBorder.PointerPressed([this, itemTag](auto const&, auto const& e) {
                e.Handled(true);
                m_selectedTag = itemTag;
                UpdateSelectionVisuals();
                if (m_flyout)
                {
                    m_flyout.Hide();
                }
                if (m_onSelectionChanged)
                {
                    m_onSelectionChanged(itemTag);
                }
            });

            m_itemsPanel.Children().Append(itemBorder);
        }

        listBorder.Child(m_itemsPanel);
        m_flyout.Content(listBorder);
        Primitives::FlyoutBase::SetAttachedFlyout(m_trigger, m_flyout);
    }

    void DropdownMenu::UpdateSelectionVisuals()
    {
        for (auto const& [tag, icon] : m_checkIcons)
        {
            if (icon)
            {
                icon.Visibility(tag == m_selectedTag ? Visibility::Visible : Visibility::Collapsed);
            }
        }
        auto it = m_itemTexts.find(m_selectedTag);
        if (it != m_itemTexts.end() && it->second && m_selectedText)
        {
            m_selectedText.Text(it->second.Text());
        }
    }
}
