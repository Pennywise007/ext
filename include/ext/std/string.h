#pragma once

#include <algorithm>
#include <cctype>
#include <locale>
#include <cwctype>
#include <string>
#include <sstream>

#include <codecvt> 

#include <ext/core/defines.h>
#include <ext/error/dump_writer.h>

namespace std {

template <typename CharType>
void string_trim_all(std::basic_string<CharType, std::char_traits<CharType>, std::allocator<CharType>>& text)
{
    auto isNotSpace = [](const CharType ch)
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

[[nodiscard]] inline std::wstring widen(const char* str)
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

[[nodiscard]] inline std::wstring widen(const std::string& str)
{
    std::wstring result;
    result.reserve(str.size());
    // do not forget about std::locale::global(std::locale("")); for some characters
    const auto& facet = use_facet<std::ctype<wchar_t>>(std::wostringstream().getloc());
    for (char ch : str)
    {
        result.append(1, facet.widen(ch));
    }
    return result;
}

[[nodiscard]] inline std::string narrow(const wchar_t* str)
{
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
}

[[nodiscard]] inline std::string narrow(const std::wstring& str)
{
    std::string result;
    result.reserve(str.size());
    // do not forget about std::locale::global(std::locale("")); for some characters
    const auto& facet = use_facet<std::ctype<wchar_t>>(std::ostringstream().getloc());
    for (wchar_t ch : str)
    {
        result.append(1, facet.narrow(ch, '\0'));
    }
    return result;
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
    if (size_s <= 0) { EXT_DUMP_IF(true); throw std::runtime_error("Error during formatting."); }
    const auto size = static_cast<size_t>(size_s);
    std::string string(size, {});
    std::snprintf(string.data(), size, format, std::forward<Args>(args)...);
    string.pop_back(); // - '\0'
    return string;
}

template <typename... Args>
[[nodiscard]] std::wstring string_swprintf(const wchar_t* format, Args&&... args) EXT_THROWS()
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    const int size_s = std::swprintf(nullptr, 0, format, std::forward<Args>(args)...) + 1; // + '\0'
    if (size_s <= 0) { EXT_DUMP_IF(true); throw std::runtime_error("Error during formatting."); }
    const auto size = static_cast<size_t>(size_s);
    std::wstring string(size, {});
    std::swprintf(string.data(), size, format, std::forward<Args>(args)...);
    string.pop_back(); // - '\0'
    return string;
#elif defined(__GNUC__) // linux
    // swprintf doesn't calculate string length in linux
    return widen(string_sprintf(narrow(format).c_str(), std::forward<Args>(args)...));
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

} // namespace std
