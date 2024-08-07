#pragma once

#include <array>
#include <string_view>

#include <ext/details/reflection/object_details.h>

namespace ext::reflection::details {

template <class EnumType, auto EnumValue>
[[nodiscard]] constexpr bool is_enum_value_impl()
{
    // value not belong to enum, example: "(enum TestEnum)0xa"
    return get_name_impl<static_cast<EnumType>(EnumValue)>()[0] != '(';
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
template <class EnumType, int EnumValueIndex, int MinEnumValue, int MaxEnumValue, unsigned i, unsigned Size>
constexpr void get_enum_values_impl(std::array<EnumType, Size>& res)
{
    if constexpr (i >= Size || EnumValueIndex > MaxEnumValue)
        return;
    else
    {
        if constexpr (!is_enum_value_impl<EnumType, EnumValueIndex>())
            get_enum_values_impl<EnumType, EnumValueIndex + 1, MinEnumValue, MaxEnumValue, i, Size>(res);
        else
        {
            res[i] = static_cast<EnumType>(EnumValueIndex);
            get_enum_values_impl<EnumType, EnumValueIndex + 1, MinEnumValue, MaxEnumValue, i + 1, Size>(res);
        }
    }
}

// Get list of all enum values with names
template <class EnumType, int EnumValueIndex, int MinEnumValue, int MaxEnumValue, unsigned i, unsigned Size>
constexpr void get_enum_values_with_names_impl(std::array<std::pair<EnumType, std::string_view>, Size>& res)
{
    if constexpr (i >= Size || EnumValueIndex > MaxEnumValue)
        return;
    else
    {
        if constexpr (!is_enum_value_impl<EnumType, EnumValueIndex>())
            get_enum_values_with_names_impl<EnumType, EnumValueIndex + 1, MinEnumValue, MaxEnumValue, i, Size>(res);
        else
        {
            res[i].first = static_cast<EnumType>(EnumValueIndex);
            res[i].second = get_name_impl<EnumType(EnumValueIndex)>();
            get_enum_values_with_names_impl<EnumType, EnumValueIndex + 1, MinEnumValue, MaxEnumValue, i + 1, Size>(res);
        }
    }
}

} // namespace ext::reflection::details