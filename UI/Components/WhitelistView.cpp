#include "pch.h"
#include "WhitelistView.h"
#include "Utils/UiUtils.h"
#include <winrt/Microsoft.UI.Text.h>
#include <cwctype>

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace GoodByDpi_App::Utils;

namespace GoodByDpi_App::UI::Components
{
    WhitelistView::WhitelistView()
    {
    }

    void WhitelistView::Initialize(
        StackPanel container,
        TextBox inputBox,
        Button addBtn,
        TextBlock addBtnText)
    {
        m_container = container;
        m_inputBox = inputBox;
        m_addBtn = addBtn;
        m_addBtnText = addBtnText;

        if (m_addBtn)
        {
            SetHandCursor(m_addBtn);

            m_addBtn.PointerEntered([this](auto const&, auto const&) {
                if (m_addBtn)
                {
                    m_addBtn.Background(SolidBrush(255, 30, 58, 95));
                    m_addBtn.BorderBrush(SolidBrush(255, 56, 189, 248));
                }
            });

            m_addBtn.PointerExited([this](auto const&, auto const&) {
                if (m_addBtn)
                {
                    m_addBtn.Background(SolidBrush(255, 22, 34, 56));
                    m_addBtn.BorderBrush(SolidBrush(255, 45, 59, 85));
                }
            });

            m_addBtn.Click([this](auto const&, auto const&) {
                HandleAdd();
            });
        }

        if (m_inputBox)
        {
            m_inputBox.KeyDown([this](auto const&, auto const& e) {
                if (e.Key() == winrt::Windows::System::VirtualKey::Enter)
                {
                    e.Handled(true);
                    HandleAdd();
                }
            });
        }
    }

    void WhitelistView::SetItems(
        std::vector<std::wstring> const& items,
        std::wstring const& emptyMessage)
    {
        if (!m_container) return;
        m_container.Children().Clear();

        if (items.empty())
        {
            auto emptyBlock = TextBlock();
            emptyBlock.Text(emptyMessage);
            emptyBlock.FontSize(11.5);
            emptyBlock.Foreground(SolidBrush(255, 100, 116, 139));
            emptyBlock.HorizontalAlignment(HorizontalAlignment::Center);
            emptyBlock.Margin({ 0, 10, 0, 10 });
            m_container.Children().Append(emptyBlock);
            return;
        }

        for (auto const& itemStr : items)
        {
            auto itemBorder = Border();
            itemBorder.Background(SolidBrush(255, 16, 23, 38));
            itemBorder.BorderBrush(SolidBrush(255, 31, 41, 61));
            itemBorder.BorderThickness(ThicknessHelper::FromUniformLength(1));
            itemBorder.CornerRadius(CornerRadiusHelper::FromUniformRadius(6));
            itemBorder.Padding({ 10, 6, 8, 6 });

            BrushTransition itemBgTrans;
            itemBgTrans.Duration(std::chrono::milliseconds(180));
            itemBorder.BackgroundTransition(itemBgTrans);

            itemBorder.PointerEntered([itemBorder](auto const&, auto const&) {
                itemBorder.Background(SolidBrush(255, 20, 30, 48));
                itemBorder.BorderBrush(SolidBrush(255, 45, 59, 85));
            });
            itemBorder.PointerExited([itemBorder](auto const&, auto const&) {
                itemBorder.Background(SolidBrush(255, 16, 23, 38));
                itemBorder.BorderBrush(SolidBrush(255, 31, 41, 61));
            });

            auto grid = Grid();
            auto c0 = ColumnDefinition();
            c0.Width(GridLengthHelper::FromValueAndType(1, GridUnitType::Star));
            auto c1 = ColumnDefinition();
            c1.Width(GridLengthHelper::Auto());
            grid.ColumnDefinitions().Append(c0);
            grid.ColumnDefinitions().Append(c1);

            auto textBlock = TextBlock();
            textBlock.Text(itemStr);
            textBlock.FontSize(12.0);
            textBlock.FontWeight(Microsoft::UI::Text::FontWeights::Medium());
            textBlock.Foreground(SolidBrush(255, 241, 245, 249));
            textBlock.VerticalAlignment(VerticalAlignment::Center);
            textBlock.TextTrimming(TextTrimming::CharacterEllipsis);
            Grid::SetColumn(textBlock, 0);
            grid.Children().Append(textBlock);

            auto delBtn = Border();
            delBtn.Padding({ 6, 4, 6, 4 });
            delBtn.Background(SolidBrush(0, 0, 0, 0));
            delBtn.CornerRadius(CornerRadiusHelper::FromUniformRadius(4));
            SetHandCursor(delBtn);

            BrushTransition delBgTrans;
            delBgTrans.Duration(std::chrono::milliseconds(180));
            delBtn.BackgroundTransition(delBgTrans);

            delBtn.PointerEntered([delBtn](auto const&, auto const&) {
                delBtn.Background(SolidBrush(255, 69, 10, 10));
            });
            delBtn.PointerExited([delBtn](auto const&, auto const&) {
                delBtn.Background(SolidBrush(0, 0, 0, 0));
            });

            auto delIcon = FontIcon();
            delIcon.Glyph(L"\uE74D");
            delIcon.FontSize(11.0);
            delIcon.Foreground(SolidBrush(255, 239, 68, 68));
            delBtn.Child(delIcon);

            delBtn.PointerPressed([this, itemStr](auto const&, auto const& e) {
                e.Handled(true);
                if (m_onDelete)
                {
                    m_onDelete(itemStr);
                }
            });

            Grid::SetColumn(delBtn, 1);
            grid.Children().Append(delBtn);

            itemBorder.Child(grid);
            m_container.Children().Append(itemBorder);
        }
    }

    void WhitelistView::SetOnAdd(std::function<void(std::wstring const& item)> onAdd)
    {
        m_onAdd = onAdd;
    }

    void WhitelistView::SetOnDelete(std::function<void(std::wstring const& item)> onDelete)
    {
        m_onDelete = onDelete;
    }

    void WhitelistView::SetAddButtonText(std::wstring const& text)
    {
        if (m_addBtnText)
        {
            m_addBtnText.Text(text);
        }
    }

    void WhitelistView::SetInputPlaceholder(std::wstring const& placeholder)
    {
        if (m_inputBox)
        {
            m_inputBox.PlaceholderText(placeholder);
        }
    }

    void WhitelistView::HandleAdd()
    {
        if (!m_inputBox) return;
        std::wstring text = m_inputBox.Text().c_str();
        while (!text.empty() && iswspace(text.front())) text.erase(text.begin());
        while (!text.empty() && iswspace(text.back())) text.pop_back();

        if (!text.empty())
        {
            m_inputBox.Text(L"");
            if (m_onAdd)
            {
                m_onAdd(text);
            }
        }
    }
}
