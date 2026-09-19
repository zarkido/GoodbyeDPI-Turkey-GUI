#pragma once
#include <string>
#include <string_view>

namespace GoodByDpi_App::Utils
{
    std::wstring Utf8ToWide(std::string_view str);
    std::string WideToUtf8(std::wstring_view wstr);
    std::string ToLowerAscii(std::string_view str);
    std::string ToLowerUtf8(std::wstring_view wstr);
}
