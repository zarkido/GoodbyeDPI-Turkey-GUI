#pragma once
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <functional>

namespace GoodByDpi_App::UI
{
    struct TrayMenuItem
    {
        UINT id;
        std::wstring text;
        wchar_t icon;
        bool isSeparator;
        bool isToggleItem;
        bool isRunning;
    };

    class TrayManager
    {
    public:
        TrayManager();
        ~TrayManager();

        void Initialize(HWND hWnd, UINT restoreMsg);
        void Remove();
        void UpdateTooltip(std::wstring const& tooltip);
        void ShowNotification(std::wstring const& title, std::wstring const& message);

        void SetOnRestore(std::function<void()> callback);
        void SetOnMinimize(std::function<void()> callback);
        void SetOnToggleState(std::function<void()> callback);
        void SetOnExit(std::function<void()> callback);
        void SetGetMenuStrings(std::function<void(std::wstring& open, std::wstring& action, std::wstring& exit, bool& isRunning)> callback);

    private:
        static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        HWND m_hWnd{ nullptr };
        NOTIFYICONDATAW m_nid{};
        bool m_isCreated{ false };
        bool m_wasMinimized{ false };
        UINT m_restoreMsg{ 0 };

        std::function<void()> m_onRestore;
        std::function<void()> m_onMinimize;
        std::function<void()> m_onToggleState;
        std::function<void()> m_onExit;
        std::function<void(std::wstring&, std::wstring&, std::wstring&, bool&)> m_getMenuStrings;
    };
}
