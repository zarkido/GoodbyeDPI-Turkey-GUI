#include "pch.h"
#include "StringUtils.h"
#include <algorithm>
#include <cctype>

namespace GoodByDpi_App::Utils
{
    std::wstring Utf8ToWide(std::string_view str)
    {
        if (str.empty()) return {};
        int size = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
        if (size <= 0) return {};
        std::wstring result(size, 0);
        ::MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);
        return result;
    }

    std::string WideToUtf8(std::wstring_view wstr)
    {
        if (wstr.empty()) return {};
        int size = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        if (size <= 0) return {};
        std::string result(size, 0);
        ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), size, nullptr, nullptr);
        return result;
    }

    std::string ToLowerAscii(std::string_view str)
    {
        std::string res(str);
        std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return res;
    }

    std::string ToLowerUtf8(std::wstring_view wstr)
    {
        std::string utf8 = WideToUtf8(wstr);
        return ToLowerAscii(utf8);
    }
}
