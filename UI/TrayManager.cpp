#include "pch.h"
#include "TrayManager.h"
#include <filesystem>
#include <vector>

namespace GoodByDpi_App::UI
{
    TrayManager::TrayManager()
    {
    }

    TrayManager::~TrayManager()
    {
        Remove();
    }

    void TrayManager::Initialize(HWND hWnd, UINT restoreMsg)
    {
        m_hWnd = hWnd;
        m_restoreMsg = restoreMsg;

        using fnSetPreferredAppMode = int (WINAPI*)(int);
        HMODULE hUxTheme = ::GetModuleHandleW(L"uxtheme.dll");
        if (!hUxTheme)
        {
            hUxTheme = ::LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        }
        if (hUxTheme)
        {
            auto pSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(::GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135)));
            if (pSetPreferredAppMode)
            {
                pSetPreferredAppMode(2);
            }
        }

        wchar_t buffer[MAX_PATH];
        ::GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        std::filesystem::path exePath(buffer);
        std::filesystem::path icoPath = exePath.parent_path() / L"Assets" / L"app.ico";
        int cx = ::GetSystemMetrics(SM_CXSMICON);
        int cy = ::GetSystemMetrics(SM_CYSMICON);
        HICON hIcon = static_cast<HICON>(::LoadImageW(nullptr, icoPath.c_str(), IMAGE_ICON, cx, cy, LR_LOADFROMFILE));
        if (!hIcon)
        {
            icoPath = exePath.parent_path() / L"app.ico";
            hIcon = static_cast<HICON>(::LoadImageW(nullptr, icoPath.c_str(), IMAGE_ICON, cx, cy, LR_LOADFROMFILE));
        }
        if (!hIcon)
        {
            hIcon = static_cast<HICON>(::LoadImageW(::GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1), IMAGE_ICON, cx, cy, 0));
        }
        if (!hIcon)
        {
            hIcon = ::LoadIconW(nullptr, IDI_APPLICATION);
        }

        ZeroMemory(&m_nid, sizeof(m_nid));
        m_nid.cbSize = sizeof(NOTIFYICONDATAW);
        m_nid.hWnd = m_hWnd;
        m_nid.uID = 1001;
        m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        m_nid.uCallbackMessage = WM_USER + 101;
        m_nid.hIcon = hIcon;

        ::Shell_NotifyIconW(NIM_ADD, &m_nid);
        m_isCreated = true;

        ::SetWindowSubclass(m_hWnd, SubclassProc, 1, reinterpret_cast<DWORD_PTR>(this));
    }

    void TrayManager::Remove()
    {
        if (m_isCreated)
        {
            ::Shell_NotifyIconW(NIM_DELETE, &m_nid);
            m_isCreated = false;
        }
        if (m_hWnd)
        {
            ::RemoveWindowSubclass(m_hWnd, SubclassProc, 1);
        }
    }

    void TrayManager::UpdateTooltip(std::wstring const& tooltip)
    {
        if (!m_isCreated) return;
        wcsncpy_s(m_nid.szTip, sizeof(m_nid.szTip) / sizeof(wchar_t), tooltip.c_str(), _TRUNCATE);
        ::Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    }

    void TrayManager::ShowNotification(std::wstring const& title, std::wstring const& message)
    {
        if (!m_isCreated) return;
        NOTIFYICONDATAW nid = m_nid;
        nid.uFlags |= NIF_INFO;
        wcsncpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle) / sizeof(wchar_t), title.c_str(), _TRUNCATE);
        wcsncpy_s(nid.szInfo, sizeof(nid.szInfo) / sizeof(wchar_t), message.c_str(), _TRUNCATE);
        nid.dwInfoFlags = NIIF_INFO | NIIF_LARGE_ICON;
        ::Shell_NotifyIconW(NIM_MODIFY, &nid);
    }

    void TrayManager::SetOnRestore(std::function<void()> callback)
    {
        m_onRestore = std::move(callback);
    }

    void TrayManager::SetOnMinimize(std::function<void()> callback)
    {
        m_onMinimize = std::move(callback);
    }

    void TrayManager::SetOnToggleState(std::function<void()> callback)
    {
        m_onToggleState = std::move(callback);
    }

    void TrayManager::SetOnExit(std::function<void()> callback)
    {
        m_onExit = std::move(callback);
    }

    void TrayManager::SetGetMenuStrings(std::function<void(std::wstring&, std::wstring&, std::wstring&, bool&)> callback)
    {
        m_getMenuStrings = std::move(callback);
    }

    LRESULT CALLBACK TrayManager::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        auto pThis = reinterpret_cast<TrayManager*>(dwRefData);
        if (!pThis)
        {
            return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
        }

        if (pThis->m_restoreMsg != 0 && uMsg == pThis->m_restoreMsg)
        {
            if (pThis->m_onRestore) pThis->m_onRestore();
            return 0;
        }

        if (uMsg == WM_MEASUREITEM)
        {
            auto pMIS = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
            if (pMIS && pMIS->CtlType == ODT_MENU && pMIS->itemData != 0)
            {
                auto item = reinterpret_cast<TrayMenuItem*>(pMIS->itemData);
                if (item)
                {
                    UINT dpi = ::GetDpiForWindow(hWnd);
                    if (dpi == 0) dpi = 96;

                    if (item->isSeparator)
                    {
                        pMIS->itemWidth = ::MulDiv(200, dpi, 96);
                        pMIS->itemHeight = ::MulDiv(10, dpi, 96);
                    }
                    else
                    {
                        pMIS->itemWidth = ::MulDiv(200, dpi, 96);
                        pMIS->itemHeight = ::MulDiv(38, dpi, 96);
                    }
                    return TRUE;
                }
            }
        }
        else if (uMsg == WM_DRAWITEM)
        {
            auto pDIS = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (pDIS && pDIS->CtlType == ODT_MENU && pDIS->itemData != 0)
            {
                auto item = reinterpret_cast<TrayMenuItem*>(pDIS->itemData);
                if (item)
                {
                    HDC hdc = pDIS->hDC;
                    RECT rc = pDIS->rcItem;
                    bool const selected = (pDIS->itemState & ODS_SELECTED) != 0;

                    UINT dpi = ::GetDpiForWindow(hWnd);
                    if (dpi == 0) dpi = 96;

                    HBRUSH bgBrush = ::CreateSolidBrush(RGB(16, 23, 38));
                    ::FillRect(hdc, &rc, bgBrush);
                    ::DeleteObject(bgBrush);

                    if (item->isSeparator)
                    {
                        int midY = rc.top + (rc.bottom - rc.top) / 2;
                        HPEN sepPen = ::CreatePen(PS_SOLID, 1, RGB(31, 41, 61));
                        HPEN oldPen = static_cast<HPEN>(::SelectObject(hdc, sepPen));
                        ::MoveToEx(hdc, rc.left + ::MulDiv(14, dpi, 96), midY, nullptr);
                        ::LineTo(hdc, rc.right - ::MulDiv(14, dpi, 96), midY);
                        ::SelectObject(hdc, oldPen);
                        ::DeleteObject(sepPen);
                        return TRUE;
                    }

                    if (selected)
                    {
                        RECT rcHighlight = rc;
                        rcHighlight.left += ::MulDiv(5, dpi, 96);
                        rcHighlight.right -= ::MulDiv(5, dpi, 96);
                        rcHighlight.top += ::MulDiv(2, dpi, 96);
                        rcHighlight.bottom -= ::MulDiv(2, dpi, 96);

                        HBRUSH selBrush = ::CreateSolidBrush(RGB(30, 41, 59));
                        HPEN selPen = ::CreatePen(PS_SOLID, 1, RGB(51, 65, 85));
                        HBRUSH oldBrush = static_cast<HBRUSH>(::SelectObject(hdc, selBrush));
                        HPEN oldPen = static_cast<HPEN>(::SelectObject(hdc, selPen));
                        int corner = ::MulDiv(6, dpi, 96);
                        ::RoundRect(hdc, rcHighlight.left, rcHighlight.top, rcHighlight.right, rcHighlight.bottom, corner, corner);
                        ::SelectObject(hdc, oldBrush);
                        ::SelectObject(hdc, oldPen);
                        ::DeleteObject(selBrush);
                        ::DeleteObject(selPen);
                    }

                    ::SetBkMode(hdc, TRANSPARENT);

                    if (item->icon != 0)
                    {
                        int iconSize = ::MulDiv(13, dpi, 96);
                        HFONT hIconFont = ::CreateFontW(
                            -iconSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                            L"Segoe Fluent Icons");

                        if (!hIconFont)
                        {
                            hIconFont = ::CreateFontW(
                                -iconSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                L"Segoe MDL2 Assets");
                        }

                        COLORREF iconColor = RGB(148, 163, 184);
                        if (item->isToggleItem)
                        {
                            if (item->isRunning)
                            {
                                iconColor = selected ? RGB(52, 211, 153) : RGB(16, 185, 129);
                            }
                            else
                            {
                                iconColor = selected ? RGB(248, 113, 113) : RGB(239, 68, 68);
                            }
                        }
                        else if (item->id == 3)
                        {
                            iconColor = selected ? RGB(248, 113, 113) : RGB(148, 163, 184);
                        }
                        else if (selected)
                        {
                            iconColor = RGB(248, 250, 252);
                        }

                        ::SetTextColor(hdc, iconColor);
                        HFONT oldFont = static_cast<HFONT>(::SelectObject(hdc, hIconFont));

                        RECT rcIcon = rc;
                        rcIcon.left += ::MulDiv(14, dpi, 96);
                        rcIcon.right = rcIcon.left + ::MulDiv(20, dpi, 96);
                        wchar_t iconStr[2] = { item->icon, L'\0' };
                        ::DrawTextW(hdc, iconStr, 1, &rcIcon, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

                        ::SelectObject(hdc, oldFont);
                        ::DeleteObject(hIconFont);
                    }

                    int textSize = ::MulDiv(12, dpi, 96);
                    HFONT hTextFont = ::CreateFontW(
                        -textSize, 0, 0, 0, selected ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                        L"Segoe UI Variable Display");

                    if (!hTextFont)
                    {
                        hTextFont = ::CreateFontW(
                            -textSize, 0, 0, 0, selected ? FW_SEMIBOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                            L"Segoe UI");
                    }

                    COLORREF textColor = selected ? RGB(255, 255, 255) : RGB(226, 232, 240);
                    ::SetTextColor(hdc, textColor);
                    HFONT oldFont = static_cast<HFONT>(::SelectObject(hdc, hTextFont));

                    RECT rcText = rc;
                    rcText.left += ::MulDiv(42, dpi, 96);
                    rcText.right -= ::MulDiv(30, dpi, 96);
                    ::DrawTextW(hdc, item->text.c_str(), static_cast<int>(item->text.length()), &rcText, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

                    ::SelectObject(hdc, oldFont);
                    ::DeleteObject(hTextFont);

                    if (item->isToggleItem)
                    {
                        int dotSize = ::MulDiv(6, dpi, 96);
                        int dotX = rc.right - ::MulDiv(20, dpi, 96);
                        int dotY = rc.top + (rc.bottom - rc.top - dotSize) / 2;

                        COLORREF dotColor = item->isRunning ? RGB(16, 185, 129) : RGB(239, 68, 68);
                        HBRUSH dotBrush = ::CreateSolidBrush(dotColor);
                        HPEN dotPen = ::CreatePen(PS_SOLID, 1, dotColor);
                        HBRUSH oldB = static_cast<HBRUSH>(::SelectObject(hdc, dotBrush));
                        HPEN oldP = static_cast<HPEN>(::SelectObject(hdc, dotPen));
                        ::Ellipse(hdc, dotX, dotY, dotX + dotSize, dotY + dotSize);
                        ::SelectObject(hdc, oldB);
                        ::SelectObject(hdc, oldP);
                        ::DeleteObject(dotBrush);
                        ::DeleteObject(dotPen);
                    }

                    return TRUE;
                }
            }
        }
        else if (uMsg == WM_USER + 101)
        {
            if (lParam == NIN_BALLOONUSERCLICK || lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK)
            {
                if (pThis->m_onRestore) pThis->m_onRestore();
                return 0;
            }
            else if (lParam == WM_RBUTTONUP)
            {
                POINT pt;
                ::GetCursorPos(&pt);

                std::wstring openStr = L"Open";
                std::wstring actionStr = L"Start";
                std::wstring exitStr = L"Exit";
                bool isRunning = false;
                if (pThis->m_getMenuStrings)
                {
                    pThis->m_getMenuStrings(openStr, actionStr, exitStr, isRunning);
                }

                std::vector<TrayMenuItem> items;
                items.push_back({ 1, openStr, L'\uE740', false, false, false });
                items.push_back({ 2, actionStr, L'\uE7E8', false, true, isRunning });
                items.push_back({ 0, L"", 0, true, false, false });
                items.push_back({ 3, exitStr, L'\uE711', false, false, false });

                HMENU hMenu = ::CreatePopupMenu();
                if (hMenu)
                {
                    MENUINFO mi = { sizeof(MENUINFO) };
                    mi.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
                    HBRUSH hBackBrush = ::CreateSolidBrush(RGB(16, 23, 38));
                    mi.hbrBack = hBackBrush;
                    ::SetMenuInfo(hMenu, &mi);

                    for (size_t i = 0; i < items.size(); ++i)
                    {
                        if (items[i].isSeparator)
                        {
                            ::AppendMenuW(hMenu, MF_OWNERDRAW | MF_SEPARATOR, items[i].id, reinterpret_cast<LPCWSTR>(&items[i]));
                        }
                        else
                        {
                            ::AppendMenuW(hMenu, MF_OWNERDRAW, items[i].id, reinterpret_cast<LPCWSTR>(&items[i]));
                        }
                    }

                    ::SetForegroundWindow(hWnd);
                    int const cmd = ::TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, nullptr);
                    ::DestroyMenu(hMenu);
                    ::DeleteObject(hBackBrush);

                    if (cmd == 1 && pThis->m_onRestore)
                    {
                        pThis->m_onRestore();
                    }
                    else if (cmd == 2 && pThis->m_onToggleState)
                    {
                        pThis->m_onToggleState();
                    }
                    else if (cmd == 3 && pThis->m_onExit)
                    {
                        pThis->m_onExit();
                    }
                }
                return 0;
            }
        }
        else if (uMsg == WM_SIZE)
        {
            if (wParam == SIZE_MINIMIZED)
            {
                pThis->m_wasMinimized = true;
                if (pThis->m_onMinimize)
                {
                    pThis->m_onMinimize();
                }
            }
            else if (wParam == SIZE_RESTORED)
            {
                if (pThis->m_wasMinimized)
                {
                    pThis->m_wasMinimized = false;
                    if (pThis->m_onRestore)
                    {
                        pThis->m_onRestore();
                    }
                }
            }
        }
        else if (uMsg == WM_NCDESTROY)
        {
            pThis->Remove();
        }

        return ::DefSubclassProc(hWnd, uMsg, wParam, lParam);
    }
}
