#include "pch.h"
#include "LogCardFactory.h"
#include "Utils/UiUtils.h"
#include <winrt/Microsoft.UI.Text.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace GoodByDpi_App::Utils;

namespace GoodByDpi_App::UI::Components
{
    Border LogCardFactory::CreateCard(
        Services::ConnectionLogItem const& item,
        bool isWhitelisted,
        std::wstring const& statusProcessedText,
        std::wstring const& statusBypassedText,
        std::wstring const& addWhitelistText,
        std::wstring const& removeWhitelistText,
        std::function<void(std::wstring const& target)> onToggleWhitelist,
        TextBlock& outTargetBlock)
    {
        auto card = Border();
        card.Background(SolidBrush(255, 16, 23, 38));
        card.BorderBrush(SolidBrush(255, 31, 41, 61));
        card.BorderThickness(ThicknessHelper::FromUniformLength(1));
        card.CornerRadius(CornerRadiusHelper::FromUniformRadius(8));
        card.Padding({ 12, 10, 12, 10 });

        BrushTransition cardBgTrans;
        cardBgTrans.Duration(std::chrono::milliseconds(180));
        card.BackgroundTransition(cardBgTrans);

        card.PointerEntered([card](auto const&, auto const&) {
            card.Background(SolidBrush(255, 20, 30, 48));
            card.BorderBrush(SolidBrush(255, 51, 65, 85));
        });
        card.PointerExited([card](auto const&, auto const&) {
            card.Background(SolidBrush(255, 16, 23, 38));
            card.BorderBrush(SolidBrush(255, 31, 41, 61));
        });

        auto grid = Grid();
        auto rd0 = RowDefinition();
        rd0.Height(GridLengthHelper::Auto());
        auto rd1 = RowDefinition();
        rd1.Height(GridLengthHelper::Auto());
        grid.RowDefinitions().Append(rd0);
        grid.RowDefinitions().Append(rd1);

        auto topGrid = Grid();
        auto cd0 = ColumnDefinition();
        cd0.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
        auto cd1 = ColumnDefinition();
        cd1.Width(GridLengthHelper::Auto());
        topGrid.ColumnDefinitions().Append(cd0);
        topGrid.ColumnDefinitions().Append(cd1);

        auto targetText = TextBlock();
        targetText.Text(item.target);
        targetText.FontSize(12.0);
        targetText.FontWeight(Microsoft::UI::Text::FontWeights::SemiBold());
        targetText.Foreground(SolidBrush(255, 248, 250, 252));
        targetText.TextTrimming(TextTrimming::CharacterEllipsis);
        targetText.VerticalAlignment(VerticalAlignment::Center);
        Grid::SetColumn(targetText, 0);
        topGrid.Children().Append(targetText);
        outTargetBlock = targetText;

        auto timeText = TextBlock();
        timeText.Text(item.timeString);
        timeText.FontSize(10.5);
        timeText.Foreground(SolidBrush(255, 100, 116, 139));
        timeText.VerticalAlignment(VerticalAlignment::Center);
        Grid::SetColumn(timeText, 1);
        topGrid.Children().Append(timeText);

        Grid::SetRow(topGrid, 0);
        grid.Children().Append(topGrid);

        auto btmGrid = Grid();
        btmGrid.Margin({ 0, 6, 0, 0 });
        auto bcd0 = ColumnDefinition();
        bcd0.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
        auto bcd1 = ColumnDefinition();
        bcd1.Width(GridLengthHelper::Auto());
        btmGrid.ColumnDefinitions().Append(bcd0);
        btmGrid.ColumnDefinitions().Append(bcd1);

        auto btmLeft = StackPanel();
        btmLeft.Orientation(Orientation::Horizontal);
        btmLeft.Spacing(6);
        btmLeft.VerticalAlignment(VerticalAlignment::Center);

        auto protoBlock = TextBlock();
        protoBlock.Text(std::to_wstring(item.port) + L" \u2022 " + item.protocol);
        protoBlock.FontSize(10.5);
        protoBlock.FontWeight(Microsoft::UI::Text::FontWeights::Medium());
        protoBlock.Foreground(SolidBrush(255, 56, 189, 248));
        protoBlock.VerticalAlignment(VerticalAlignment::Center);
        btmLeft.Children().Append(protoBlock);

        auto statusBadge = Border();
        statusBadge.CornerRadius(CornerRadiusHelper::FromUniformRadius(4));
        statusBadge.Padding({ 6, 2, 6, 2 });
        auto statusText = TextBlock();
        statusText.FontSize(9.5);
        statusText.FontWeight(Microsoft::UI::Text::FontWeights::SemiBold());

        if (isWhitelisted)
        {
            statusBadge.Background(SolidBrush(255, 30, 41, 59));
            statusBadge.BorderBrush(SolidBrush(255, 51, 65, 85));
            statusBadge.BorderThickness(ThicknessHelper::FromUniformLength(1));
            statusText.Text(statusBypassedText);
            statusText.Foreground(SolidBrush(255, 148, 163, 184));
        }
        else
        {
            statusBadge.Background(SolidBrush(255, 6, 78, 59));
            statusBadge.BorderBrush(SolidBrush(255, 5, 150, 105));
            statusBadge.BorderThickness(ThicknessHelper::FromUniformLength(1));
            statusText.Text(statusProcessedText);
            statusText.Foreground(SolidBrush(255, 110, 231, 183));
        }
        statusBadge.Child(statusText);
        btmLeft.Children().Append(statusBadge);

        Grid::SetColumn(btmLeft, 0);
        btmGrid.Children().Append(btmLeft);

        auto wlBtn = Border();
        wlBtn.Padding({ 8, 3, 8, 3 });
        wlBtn.CornerRadius(CornerRadiusHelper::FromUniformRadius(5));
        wlBtn.BorderThickness(ThicknessHelper::FromUniformLength(1));
        SetHandCursor(wlBtn);

        BrushTransition btnBgTrans;
        btnBgTrans.Duration(std::chrono::milliseconds(180));
        wlBtn.BackgroundTransition(btnBgTrans);

        auto wlText = TextBlock();
        wlText.FontSize(10.5);
        wlText.FontWeight(Microsoft::UI::Text::FontWeights::Medium());
        wlBtn.Child(wlText);

        auto wlState = std::make_shared<bool>(isWhitelisted);
        auto isWlHovered = std::make_shared<bool>(false);

        auto updateBtnVisual = [wlBtn, wlText, statusBadge, statusText, statusProcessedText, statusBypassedText, addWhitelistText, removeWhitelistText, wlState, isWlHovered]() {
            if (*wlState)
            {
                wlText.Text(removeWhitelistText);
                if (*isWlHovered)
                {
                    wlBtn.Background(SolidBrush(255, 127, 29, 29));
                    wlBtn.BorderBrush(SolidBrush(255, 239, 68, 68));
                    wlText.Foreground(SolidBrush(255, 254, 202, 202));
                }
                else
                {
                    wlBtn.Background(SolidBrush(255, 69, 10, 10));
                    wlBtn.BorderBrush(SolidBrush(255, 153, 27, 27));
                    wlText.Foreground(SolidBrush(255, 252, 165, 165));
                }

                statusBadge.Background(SolidBrush(255, 30, 41, 59));
                statusBadge.BorderBrush(SolidBrush(255, 51, 65, 85));
                statusText.Text(statusBypassedText);
                statusText.Foreground(SolidBrush(255, 148, 163, 184));
            }
            else
            {
                wlText.Text(addWhitelistText);
                if (*isWlHovered)
                {
                    wlBtn.Background(SolidBrush(255, 30, 58, 95));
                    wlBtn.BorderBrush(SolidBrush(255, 56, 189, 248));
                    wlText.Foreground(SolidBrush(255, 186, 230, 253));
                }
                else
                {
                    wlBtn.Background(SolidBrush(255, 22, 34, 56));
                    wlBtn.BorderBrush(SolidBrush(255, 45, 59, 85));
                    wlText.Foreground(SolidBrush(255, 56, 189, 248));
                }

                statusBadge.Background(SolidBrush(255, 6, 78, 59));
                statusBadge.BorderBrush(SolidBrush(255, 5, 150, 105));
                statusText.Text(statusProcessedText);
                statusText.Foreground(SolidBrush(255, 110, 231, 183));
            }
        };

        updateBtnVisual();

        wlBtn.PointerEntered([updateBtnVisual, isWlHovered](auto const&, auto const&) {
            *isWlHovered = true;
            updateBtnVisual();
        });

        wlBtn.PointerExited([updateBtnVisual, isWlHovered](auto const&, auto const&) {
            *isWlHovered = false;
            updateBtnVisual();
        });

        std::wstring target = item.target;
        wlBtn.PointerPressed([updateBtnVisual, wlState, onToggleWhitelist, target](auto const&, auto const& e) {
            e.Handled(true);
            *wlState = !(*wlState);
            updateBtnVisual();
            if (onToggleWhitelist)
            {
                onToggleWhitelist(target);
            }
        });

        Grid::SetColumn(wlBtn, 1);
        btmGrid.Children().Append(wlBtn);

        Grid::SetRow(btmGrid, 1);
        grid.Children().Append(btmGrid);

        card.Child(grid);
        return card;
    }
}
