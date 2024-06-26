#pragma once

#include <array>
#include <string_view>

namespace ext::details::reflection {

// Using __builtin_ functions to get object name with template type
template <auto EnumType>
[[nodiscard]] constexpr std::string_view get_enum_name_impl() {

#if defined(__clang__) || defined(__GNUC__)
    constexpr auto func_name = std::string_view{__PRETTY_FUNCTION__};

    constexpr auto prefix = "EnumType = ";
    constexpr auto suffixDelimer = ';';

    constexpr auto split = func_name.substr(0, func_name.find_last_of(suffixDelimer));
    return split.substr(split.find(prefix) + std::string_view(prefix).size());
#elif defined(_MSC_VER)
    constexpr auto func_name = std::string_view{__builtin_FUNCSIG()};

    constexpr auto prefix = "get_enum_name_impl<";
    constexpr auto suffix = ">(void)";

    constexpr auto split = func_name.substr(0, func_name.size() - std::string_view(suffix).size());
    return split.substr(split.find(prefix) + std::string_view(prefix).size());
#else
    static_assert(false, "Unsupported compiler");
#endif
}

template <class EnumType, auto EnumValue>
[[nodiscard]] constexpr bool is_enum_value_impl()
{
    // value not belong to enum, example: "(enum TestEnum)0xa"
    return get_enum_name_impl<static_cast<EnumType>(EnumValue)>()[0] != '(';
}

// Search over numbers and check if number belongs to enum
template <class EnumType, int MinEnumValue, int MaxEnumValue, int i>
[[nodiscard]] constexpr unsigned get_enum_size_impl()
{
    if constexpr (i > MaxEnumValue)
        return 0;
    else
    {
        if constexpr (!is_enum_value_impl<EnumType, i>())
            return get_enum_size_impl<EnumType, MinEnumValue, MaxEnumValue, i + 1>();
        else
            return 1 + get_enum_size_impl<EnumType, MinEnumValue, MaxEnumValue, i + 1>();
    }
}

// Get enum value by index
template <class EnumType, int EnumValueIndex, int MinEnumValue = 0, int MaxEnumValue = 100, int i, unsigned FoundEnumItems>
[[nodiscard]] constexpr EnumType get_enum_value_impl()
{
    if constexpr (i > MaxEnumValue)
        return 0;
    else
    {
        if constexpr (!is_enum_value_impl<EnumType, i>())
            return get_enum_value_impl<EnumType, EnumValueIndex, MinEnumValue, MaxEnumValue, i + 1, FoundEnumItems>();
        else if constexpr (EnumValueIndex == FoundEnumItems)
            return static_cast<EnumType>(i);
        else
            return get_enum_value_impl<EnumType, EnumValueIndex, MinEnumValue, MaxEnumValue, i + 1, FoundEnumItems + 1>();
    }
}

// Get list of all enum values
template <class EnumType, unsigned MinEnumValue, int MaxEnumValue, unsigned i, unsigned Size>
constexpr void get_enum_values_impl(std::array<EnumType, Size>& res)
{
    if constexpr (i >= Size)
        return;
    else
    {
        res[i] = get_enum_value_impl<EnumType, i, MinEnumValue, MaxEnumValue, MinEnumValue, 0>();
        get_enum_values_impl<EnumType, MinEnumValue, MaxEnumValue, i + 1, Size>(res);
    }
}

} // namespace ext::details::reflection