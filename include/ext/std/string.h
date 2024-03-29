#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <codecvt> 
#include <cwctype>
#include <locale>
#include <string>
#include <sstream>

#include <ext/core/defines.h>

namespace std {

template <typename CharType>
void string_trim_all(std::basic_string<CharType, std::char_traits<CharType>, std::allocator<CharType>>& text)
{
    auto isNotSpace = [](CharType ch)
    {
        if (ch < 0 || ch > 255)
            return true;

        if constexpr (std::is_same_v<CharType, wchar_t>)
            return !std::iswspace(ch);
        else
            return !std::isspace(ch);
    };
    const auto indexFirstNotSpace = std::distance(text.begin(), std::find_if(text.begin(), text.end(), isNotSpace));
    const auto indexLastNotSpace = std::distance(text.rbegin(), std::find_if(text.rbegin(), text.rend(), isNotSpace));
    text = text.substr(indexFirstNotSpace, text.length() - indexLastNotSpace - indexFirstNotSpace);
}

template <typename CharType>
void string_trim_left(std::basic_string<CharType, std::char_traits<CharType>, std::allocator<CharType>>& text)
{
    auto isNotSpace = [](CharType ch)
    {
        if (ch < 0 || ch > 255)
            return true;

        if constexpr (std::is_same_v<CharType, wchar_t>)
            return !std::iswspace(ch);
        else
            return !std::isspace(ch);
    };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), isNotSpace));
}

template <typename CharType>
auto string_trim_left(std::basic_string_view<CharType, std::char_traits<CharType>>& text)
{
    auto isNotSpace = [](CharType ch)
    {
        if (ch < 0 || ch > 255)
            return true;

        if constexpr (std::is_same_v<CharType, wchar_t>)
            return !std::iswspace(ch);
        else
            return !std::isspace(ch);
    };
    return text.substr(std::distance(text.begin(), std::find_if(text.begin(), text.end(), isNotSpace)));
}

template <typename CharType>
void string_trim_right(std::basic_string<CharType, std::char_traits<CharType>, std::allocator<CharType>>& text)
{
    auto isNotSpace = [](CharType ch)
    {
        if (ch < 0 || ch > 255)
            return true;

        if constexpr (std::is_same_v<CharType, wchar_t>)
            return !std::iswspace(ch);
        else
            return !std::isspace(ch);
    };
    const auto charsToRemove = std::distance(text.rbegin(), std::find_if(text.rbegin(), text.rend(), isNotSpace));
    text.resize(text.size() - charsToRemove);
}

template <typename CharType>
auto string_trim_right(const std::basic_string_view<CharType, std::char_traits<CharType>>& text)
{
    auto isNotSpace = [](CharType ch)
    {
        if (ch < 0 || ch > 255)
            return true;

        if constexpr (std::is_same_v<CharType, wchar_t>)
            return !std::iswspace(ch);
        else
            return !std::isspace(ch);
    };
    return text.substr(0, text.size() - std::distance(text.rbegin(), std::find_if(text.rbegin(), text.rend(), isNotSpace)));
}

[[nodiscard]] inline std::wstring widen(const char* str)
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
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
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
#endif
}

[[nodiscard]] inline std::wstring widen(const std::string& str)
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    std::wstring result;
    result.reserve(str.size());
    // do not forget about std::locale::global(std::locale("")); for some characters
    const auto& facet = use_facet<std::ctype<wchar_t>>(std::wostringstream().getloc());
    for (char ch : str)
    {
        result.append(1, facet.widen(ch));
    }
    return result;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
#endif
}

[[nodiscard]] inline std::string narrow(const wchar_t* str)
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    const auto length = wcslen(str);

    std::string result;
    result.resize(length);

    // do not forget about std::locale::global(std::locale("")); for some characters
    const auto& facet = use_facet<std::ctype<wchar_t>>(std::ostringstream().getloc());
    std::transform(str, str + length, result.begin(), [&facet](const wchar_t ch)
    {
        return facet.narrow(ch, '\0');
    });
    return result;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(str);
#endif
}

[[nodiscard]] inline std::string narrow(const std::wstring& str)
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    std::string result;
    result.reserve(str.size());
    // do not forget about std::locale::global(std::locale("")); for some characters
    const auto& facet = use_facet<std::ctype<wchar_t>>(std::ostringstream().getloc());
    for (wchar_t ch : str)
    {
        result.append(1, facet.narrow(ch, '\0'));
    }
    return result;
#else
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(str);
#endif
}

#if defined(__cplusplus) && __cplusplus >= 202004L
// C++20 (and later) code

#include <format>
#include <string_view>

template <typename... Args>
[[nodiscard]] std::string string_sprintf(std::string_view rt_fmt_str, Args&&... args)
{
    return std::vformat(rt_fmt_str, std::make_format_args(args...));
}

#else // not c++ 20
template <typename... Args>
[[nodiscard]] std::string string_sprintf(const char* format, Args&&... args) EXT_THROWS()
{
    const int size_s = std::snprintf(nullptr, 0, format, std::forward<Args>(args)...) + 1; // + '\0'
    if (size_s <= 0) { assert(false); throw std::runtime_error("Error during formatting."); }
    const auto size = static_cast<size_t>(size_s);
    std::string string(size, {});
    std::snprintf(string.data(), size, format, std::forward<Args>(args)...);
    string.pop_back(); // - '\0'
    return string;
}

template <typename... Args>
[[nodiscard]] std::wstring string_swprintf(const wchar_t* format, Args&&... args) EXT_THROWS()
{
#if defined(__GNUC__) // linux
    // swprintf doesn't work properly on linux, we will use string_sprintf
    std::wstring formatStr(format);
    for (size_t i = 0, length = formatStr.size(); i + 1 < length; ++i) {
        if (formatStr[i] == L'%') {
            auto& symb = formatStr[++i];
            switch (symb) {
                case L's':
                    symb = L'S';
                    break;
                case L'S':
                    symb = L's';
                    break;
            }
        }
    }
    return widen(string_sprintf(narrow(formatStr).c_str(), std::forward<Args>(args)...));
#else
    const int size_s = std::swprintf(nullptr, 0, format, std::forward<Args>(args)...) + 1; // + '\0'
    if (size_s <= 0) { assert(false); throw std::runtime_error("Error during formatting."); }
    const auto size = static_cast<size_t>(size_s);
    std::wstring string(size, {});
    std::swprintf(string.data(), size, format, std::forward<Args>(args)...);
    string.pop_back(); // - '\0'
    return string;
#endif
}
#endif

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

inline auto& operator<<(std::ostream& stream, const wchar_t* text)
{
    return stream << narrow(text);
}

inline auto& operator<<(std::wostream& stream, const char* text)
{
    return stream << widen(text);
}

inline auto& operator<<(std::ostream& stream, const std::wstring& text)
{
    return stream << narrow(text).c_str();
}

inline auto& operator<<(std::wostream& stream, const std::string& text)
{
    return stream << widen(text).c_str();
}

} // namespace std