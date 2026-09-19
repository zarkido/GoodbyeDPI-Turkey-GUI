#pragma once
#include <string>
#include <functional>
#include <winrt/base.h>
#include <winrt/Microsoft.UI.Xaml.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include "Services/TrafficLogger.h"

namespace GoodByDpi_App::UI::Components
{
    class LogCardFactory
    {
    public:
        static winrt::Microsoft::UI::Xaml::Controls::Border CreateCard(
            Services::ConnectionLogItem const& item,
            bool isWhitelisted,
            std::wstring const& statusProcessedText,
            std::wstring const& statusBypassedText,
            std::wstring const& addWhitelistText,
            std::wstring const& removeWhitelistText,
            std::function<void(std::wstring const& target)> onToggleWhitelist,
            winrt::Microsoft::UI::Xaml::Controls::TextBlock& outTargetBlock);
    };
}
