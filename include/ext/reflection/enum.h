#pragma once

#include <ext/details/reflection/enum_details.h>

namespace ext::reflection {

// Check if object is scoped enum
template <class EnumType>
constexpr bool is_scoped_enum_v = std::is_enum_v<EnumType> &&
    !std::is_convertible_v<EnumType, std::underlying_type_t<EnumType>>;

template <class EnumType, unsigned MinEnumValue = 0, int MaxEnumValue = 100,
    unsigned EnumSize = get_enum_size<EnumType, MinEnumValue, MaxEnumValue>()>
[[nodiscard]] constexpr std::array<EnumType, EnumSize> get_enum_values();

/*
Get enum value name

Usage:
enum class TestEnum { eEnumValue };

ext::reflection::get_enum_name<TestEnum(0)>() == "TestEnum::eEnumValue"
ext::reflection::get_enum_name<TestEnum::eEnumValue>() == "TestEnum::eEnumValue"
ext::reflection::get_enum_name<TestEnum(-1)>() == "(enum TestEnum)0xffffffffffffffff"

@return string presentation of the enum value or (enum %ENUM_NAME%)0xF if not found
*/
template <auto EnumType>
[[nodiscard]] constexpr std::string_view get_enum_name() {
    return ext::details::reflection::get_enum_name_impl<EnumType>();
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
[[nodiscard]] constexpr bool is_enum_value() {
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    return ext::details::reflection::is_enum_value_impl<EnumType, EnumValue>();
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
[[nodiscard]] bool is_enum_value(T value) {
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    constexpr auto enumValues = get_enum_values<EnumType, MinEnumValue, MaxEnumValue>();
    return std::find(enumValues.cbegin(), enumValues.cend(), static_cast<EnumType>(value)) != enumValues.end();
}

/*
Get enum items size

Usage:
enum class TestEnum { eEnumValue = 0, eEnumValue2 = 101 };
ext::reflection::get_enum_size<TestEnum>() == 1
ext::reflection::get_enum_size<TestEnum, 0, 101>() == 2
*/
template <class EnumType, int MinEnumValue = 0, int MaxEnumValue = 100>
[[nodiscard]] constexpr unsigned get_enum_size() {
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    return ext::details::reflection::get_enum_size_impl<EnumType, MinEnumValue, MaxEnumValue, MinEnumValue>();
}

/*
Get enum value by index

Usage:
enum class TestEnum { eEnumValue1, eEnumValue2 };
ext::reflection::get_enum_value<TestEnum, 0>() == TestEnum::eEnumValue1
ext::reflection::get_enum_value<TestEnum, 1>() == TestEnum::eEnumValue2
*/
template <class EnumType, unsigned EnumValueIndex, int MinEnumValue = 0, int MaxEnumValue = 100>
[[nodiscard]] constexpr EnumType get_enum_value() {
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");
    static_assert(get_enum_size<EnumType, MinEnumValue, MaxEnumValue>() > EnumValueIndex, "Value with given index doesn't exist in enum");

    return ext::details::reflection::get_enum_value_impl<EnumType, EnumValueIndex, MinEnumValue, MaxEnumValue, MinEnumValue, 0>();
}

/*
Get all enum values

Usage:
enum class TestEnum { eEnumValue1, eEnumValue2 };
ext::reflection::get_enum_values<TestEnum>() == { TestEnum::eEnumValue1, TestEnum::eEnumValue2 }
*/
template <class EnumType, unsigned MinEnumValue, int MaxEnumValue, unsigned EnumSize>
[[nodiscard]] constexpr std::array<EnumType, EnumSize> get_enum_values() {
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    std::array<EnumType, EnumSize> res = {};
    ext::details::reflection::get_enum_values_impl<EnumType, MinEnumValue, MaxEnumValue, 0, EnumSize>(res);
    return res;
}

} // namespace ext::reflection