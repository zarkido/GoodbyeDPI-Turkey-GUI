#pragma once
#include <string>
#include <vector>
#include <functional>
#include <winrt/base.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>

namespace GoodByDpi_App::UI::Components
{
    class WhitelistView
    {
    public:
        WhitelistView();

        void Initialize(
            winrt::Microsoft::UI::Xaml::Controls::StackPanel container,
            winrt::Microsoft::UI::Xaml::Controls::TextBox inputBox,
            winrt::Microsoft::UI::Xaml::Controls::Button addBtn,
            winrt::Microsoft::UI::Xaml::Controls::TextBlock addBtnText);

        void SetItems(
            std::vector<std::wstring> const& items,
            std::wstring const& emptyMessage);

        void SetOnAdd(std::function<void(std::wstring const& item)> onAdd);
        void SetOnDelete(std::function<void(std::wstring const& item)> onDelete);
        void SetAddButtonText(std::wstring const& text);
        void SetInputPlaceholder(std::wstring const& placeholder);

    private:
        void HandleAdd();

        winrt::Microsoft::UI::Xaml::Controls::StackPanel m_container{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBox m_inputBox{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::Button m_addBtn{ nullptr };
        winrt::Microsoft::UI::Xaml::Controls::TextBlock m_addBtnText{ nullptr };

        bool m_isInitialized{ false };
        std::function<void(std::wstring const& item)> m_onAdd;
        std::function<void(std::wstring const& item)> m_onDelete;
    };
}
