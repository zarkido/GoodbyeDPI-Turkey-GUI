#pragma once
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <winrt/base.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Controls.Primitives.h>
#include <winrt/Microsoft.UI.Xaml.Media.h>
#include <winrt/Microsoft.UI.Xaml.Input.h>

namespace GoodByDpi_App::UI::Components
{
    struct DropdownItem
    {
        std::wstring tag;
        std::wstring badge;
        std::wstring text;
    };

    class DropdownMenu
    {
    public:
        DropdownMenu();

        void Initialize(
            winrt::Microsoft::UI::Xaml::Controls::Border trigger,
            winrt::Microsoft::UI::Xaml::Controls::TextBlock selectedText,
            winrt::Microsoft::UI::Xaml::Controls::FontIcon chevron,
            double dropdownWidth = 462.0);

        void SetItems(std::vector<DropdownItem> const& items);
        void SetSelectedItem(std::wstring const& tag);
        void SetSelectedText(std::wstring const& text);
        std::wstring GetSelectedTag() const;
        void SetOnSelectionChanged(std::function<void(std::wstring const& tag)> callback);
        void Close();

    private:
        void BuildFlyout();
        void UpdateSelectionVisuals();

        winrt::Microsoft::UI::Xaml::Controls::Border m_trigger{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_selectedText{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::FontIcon m_chevron{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Flyout m_flyout{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::StackPanel m_itemsPanel{ nullptr };

        double m_dropdownWidth{ 462.0 };
        std::vector<DropdownItem> m_items;
        std::wstring m_selectedTag;
        std::function<void(std::wstring const& tag)> m_onSelectionChanged;
        std::unordered_map<std::wstring, winrt::Microsoft::UI::Xaml::Controls::FontIcon> m_checkIcons;
        std::unordered_map<std::wstring, winrt::Microsoft::UI::Xaml::Controls::TextBlock> m_itemTexts;
    };
}
