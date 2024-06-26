#pragma once

#include <ext/details/reflection/enum_details.h>

namespace ext::reflection {

// Check if object is scoped enum
// like: vvvvv
// enum  class TestEnum { eEnumValue };
template <class EnumType>
constexpr bool is_scoped_enum_v = std::is_enum_v<EnumType> &&
    !std::is_convertible_v<EnumType, std::underlying_type_t<EnumType>>;

/*
Get enum value name

Usage:
enum class TestEnum { eEnumValue };

enum_name<TestEnum(0)> == "TestEnum::eEnumValue"
enum_name<TestEnum::eEnumValue>() == "TestEnum::eEnumValue"
enum_name<TestEnum(-1)> == "(enum TestEnum)0xffffffffffffffff"

@return string presentation of the enum value or (enum %ENUM_NAME%)0xF if not found
*/
template <auto EnumType>
constexpr std::string_view enum_name = ext::reflection::details::get_name_impl<EnumType>();

/*
Get enum items size

Usage:
enum class TestEnum { eEnumValue = 0, eEnumValue2 = 101 };
ext::reflection::get_enum_size<TestEnum>() == 1
ext::reflection::get_enum_size<TestEnum, 0, 101>() == 2
*/
template <class EnumType, int MinEnumValue = 0, int MaxEnumValue = 100>
[[nodiscard]] constexpr unsigned get_enum_size()
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    return ext::reflection::details::get_enum_size_impl<EnumType, MinEnumValue, MaxEnumValue, MinEnumValue>();
}

/*
Compile time check if value belongs to enum

Usage:
enum class TestEnum { eEnumValue };
ext::reflection::is_enum_value<TestEnum(0)>() == true
ext::reflection::is_enum_value<TestEnum(TestEnum::eEnumValue)>() == true
ext::reflection::is_enum_value<TestEnum(-1)>() == false
ext::reflection::is_enum_value<-1>() == false
*/
template <class EnumType, auto EnumValue>
[[nodiscard]] constexpr bool is_enum_value()
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    return ext::reflection::details::is_enum_value_impl<EnumType, EnumValue>();
}

/*
Run time check if value belongs to enum

Usage:
enum class TestEnum { eEnumValue };
ext::reflection::is_enum_value<TestEnum>(0) == true
ext::reflection::is_enum_value<TestEnum>(TestEnum::eEnumValue) == true
ext::reflection::is_enum_value<TestEnum>(-1) == false
*/
template <class EnumType, unsigned MinEnumValue = 0, int MaxEnumValue = 100, typename T>
[[nodiscard]] bool is_enum_value(T value)
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    constexpr auto EnumSize = get_enum_size<EnumType, MinEnumValue, MaxEnumValue>();
    std::array<EnumType, EnumSize> enumValues = {};
    ext::reflection::details::get_enum_values_impl<EnumType, MinEnumValue, MaxEnumValue, 0, EnumSize>(enumValues);
    return std::find(enumValues.cbegin(), enumValues.cend(), static_cast<EnumType>(value)) != enumValues.end();
}

/*
Get all enum values

Usage:
enum class TestEnum { eEnumValue1, eEnumValue2 };
ext::reflection::get_enum_values<TestEnum>() == { TestEnum::eEnumValue1, TestEnum::eEnumValue2 }
*/
template <class EnumType, unsigned MinEnumValue = 0, int MaxEnumValue = 100,
          unsigned EnumSize = get_enum_size<EnumType, MinEnumValue, MaxEnumValue>()>
[[nodiscard]] constexpr std::array<EnumType, EnumSize> get_enum_values()
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    std::array<EnumType, EnumSize> res = {};
    ext::reflection::details::get_enum_values_impl<EnumType, MinEnumValue, MaxEnumValue, 0, EnumSize>(res);
    return res;
}

/*
Get enum value by index

Usage:
enum class TestEnum { eEnumValue1, eEnumValue2 };
ext::reflection::get_enum_value<TestEnum, 0>() == TestEnum::eEnumValue1
ext::reflection::get_enum_value<TestEnum, 1>() == TestEnum::eEnumValue2
*/
template <class EnumType, unsigned EnumValueIndex, int MinEnumValue = 0, int MaxEnumValue = 100>
[[nodiscard]] constexpr EnumType get_enum_value()
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");
    static_assert(get_enum_size<EnumType, MinEnumValue, MaxEnumValue>() > EnumValueIndex, "Value with given index doesn't exist in enum");

    return ext::reflection::details::get_enum_value_impl<EnumType, EnumValueIndex, MinEnumValue, MaxEnumValue, MinEnumValue, 0>();
}

} // namespace ext::reflection