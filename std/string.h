#pragma once

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <string>
#include <sstream>
#include <xlocale>
#include <xstring>

#include <ext/core/defines.h>
#include <ext/error/dump_writer.h>

namespace std {

#if __cplusplus >= 202002L
// C++20 (and later) code

#include <format>
#include <string_view>

template <typename... Args>
EXT_NODISCARD std::string string_sprintf(std::string_view rt_fmt_str, Args&&... args)
{
    return std::vformat(rt_fmt_str, std::make_format_args(args...));
}

#else
template <typename... Args>
EXT_NODISCARD std::string string_sprintf(const char* format, Args&&... args) EXT_THROWS()
{
    const int size_s = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...) + 1; // + '\0'
    if (size_s <= 0) { EXT_DUMP_IF(true); throw std::runtime_error("Error during formatting."); }
    const auto size = static_cast<size_t>(size_s);
    std::string string(size, {});
    std::snprintf(string.data(), size, format, std::forward<Args>(args)...);
    string.pop_back(); // - '\0'
    return string;
}

template <typename... Args>
EXT_NODISCARD std::wstring string_swprintf(const wchar_t* format, Args&&... args) EXT_THROWS()
{
    const int size_s = std::swprintf(nullptr, 0, format, std::forward<Args>(args)...) + 1; // + '\0'
    if (size_s <= 0) { EXT_DUMP_IF(true); throw std::runtime_error("Error during formatting."); }
    const auto size = static_cast<size_t>(size_s);
    std::wstring string(size, {});
    std::swprintf(string.data(), size, format, std::forward<Args>(args)...);
    string.pop_back(); // - '\0'
    return string;
}
#endif

template <typename CharType>
void string_trim_all(std::basic_string<CharType, std::char_traits<CharType>, std::allocator<CharType>>& text)
{
    auto isNotSpace = [](const CharType ch)
    {
        if constexpr (std::is_same_v<CharType, wchar_t>)
            return !std::iswspace(ch);
        else
            return !std::isspace(ch);
    };
    const auto indexFirstNotSpace = std::distance(text.begin(), std::find_if(text.begin(), text.end(), isNotSpace));
    const auto indexLastNotSpace = std::distance(text.rbegin(), std::find_if(text.rbegin(), text.rend(), isNotSpace));
    text = text.substr(indexFirstNotSpace, text.length() - indexLastNotSpace - indexFirstNotSpace);
}

EXT_NODISCARD inline std::wstring widen(const char* str)
{
    const auto length = strlen(str);

    std::wstring result;
    result.resize(length);

    // do not forget about std::locale::global(std::locale("")); for some characters
    const auto& facet = use_facet<std::ctype<wchar_t>>(std::wostringstream().getloc());
    std::transform(str, str + length, result.begin(), [&facet](const char ch)
    {
        return facet.widen(ch);
    });
    return result;
}

EXT_NODISCARD inline std::string narrow(const wchar_t* str)
{
    const auto length = wcslen(str);

    std::string result;
    result.resize(length);

    // do not forget about std::locale::global(std::locale("")); for some characters
    const auto& facet = use_facet<std::ctype<wchar_t>>(std::ostringstream().getloc());
    std::transform(str, str + length, result.begin(), [&facet](const wchar_t ch)
    {
        return facet.narrow(ch);
    });
    return result;
}

inline auto& operator<<(std::ostringstream& stream, const wchar_t* text)
{
    return stream << narrow(text);
}

inline auto& operator<<(std::wostringstream& stream, const char* text)
{
    return stream << widen(text);
}

template <typename StreamCharType, typename CharType>
auto& operator<<(std::basic_ostringstream<StreamCharType>& stream, const std::basic_string<CharType>& string)
{
    return stream << string.c_str();
}

template <typename StreamCharType, typename CharType>
auto& operator<<(std::basic_ostringstream<StreamCharType>& stream, const std::basic_string_view<CharType>& string)
{
    return stream << string.data();
}

} // namespace std
