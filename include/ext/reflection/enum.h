#pragma once

#include <string_view>

#include <ext/core/check.h>
#include <ext/details/reflection/enum_details.h>
#include <ext/types/utils.h>

namespace ext::reflection {

// Check if object is scoped enum
// like: vvvvv
// enum  class TestEnum { eEnumValue };
template <class EnumType>
constexpr bool is_scoped_enum_v = std::is_enum_v<EnumType> &&
    !std::is_convertible_v<EnumType, std::underlying_type_t<EnumType>>;

/*
Convert enum value to string representation

Usage:
enum class TestEnum { 
    eEnumValue,  
    eEnumValue5 = 5,
};
ext::reflection::enum_to_string(TestEnum::eEnumValue) == "TestEnum::eEnumValue"
ext::reflection::enum_to_string(TestEnum(5)) == "TestEnum::eEnumValue5"
*/
template <int MinEnumValue = 0, int MaxEnumValue = 100, typename EnumType>
[[nodiscard]] constexpr std::string_view enum_to_string(EnumType value) EXT_THROWS();

/*
Get enum value name

Usage:
enum class TestEnum { eEnumValue };

ext::reflection::enum_to_string<TestEnum(0)>() == "TestEnum::eEnumValue"
ext::reflection::enum_to_string<TestEnum::eEnumValue>() == "TestEnum::eEnumValue"
ext::reflection::enum_to_string<TestEnum(-1)>() == "(enum TestEnum)0xffffffffffffffff"

@return string presentation of the enum value or (enum %ENUM_NAME%)0xF if not found
*/
template <auto EnumType>
[[nodiscard]] constexpr std::string_view enum_to_string()
{
    return ext::reflection::details::get_name_impl<EnumType>();
}

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
    ext::reflection::details::get_enum_values_impl<EnumType, MinEnumValue, MinEnumValue, MaxEnumValue, 0, EnumSize>(res);
    return res;
}

/*
Get all enum values with names

Usage:
enum class TestEnum { eEnumValue1, eEnumValue2 };
ext::reflection::get_enum_values_with_names<TestEnum>() == { 
    { TestEnum::eEnumValue1, "TestEnum::eEnumValue1" }, 
    { TestEnum::eEnumValue2, "TestEnum::eEnumValue2" },
}
*/
template <class EnumType, unsigned MinEnumValue = 0, int MaxEnumValue = 100,
          unsigned EnumSize = get_enum_size<EnumType, MinEnumValue, MaxEnumValue>()>
[[nodiscard]] constexpr std::array<std::pair<EnumType, std::string_view>, EnumSize> get_enum_values_with_names()
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    std::array<std::pair<EnumType, std::string_view>, EnumSize> res = {};
    ext::reflection::details::get_enum_values_with_names_impl<EnumType, MinEnumValue, MinEnumValue, MaxEnumValue, 0, EnumSize>(res);
    return res;
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
[[nodiscard]] constexpr bool is_enum_value(T value)
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    constexpr auto enumValues = get_enum_values<EnumType, MinEnumValue, MaxEnumValue>();
    return std::find(enumValues.cbegin(), enumValues.cend(), static_cast<EnumType>(value)) != enumValues.end();
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

/*
Get enum value name in runtime

Usage:
enum class TestEnum { eEnumValue };
ext::reflection::enum_to_string(TestEnum::eEnumValue) == "TestEnum::eEnumValue"
*/
template <int MinEnumValue, int MaxEnumValue, typename EnumType>
[[nodiscard]] constexpr std::string_view enum_to_string(EnumType value)
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    constexpr auto enumValuesWithNames = get_enum_values_with_names<EnumType, MinEnumValue, MaxEnumValue>();
    
    const auto it = std::find_if(enumValuesWithNames.cbegin(), enumValuesWithNames.cend(),
        [value](const std::pair<EnumType, std::string_view>& item) {
            return item.first == value;        
        });
    if (it == enumValuesWithNames.cend())
    {
        EXT_ASSERT(false) << "Unknown value" + std::to_string((int)value) + " of enum " + ext::type_name<EnumType>();
        return "Unknown enum value";
    }
    return it->second;
}

/*
Get enum value by name in runtime

Usage:
enum class TestEnum { eEnumValue };
ext::reflection::get_enum_value_by_name<TextEnum>("TestEnum::eEnumValue") == TestEnum::eEnumValue
*/
template <typename EnumType, int MinEnumValue = 0, int MaxEnumValue = 100>
[[nodiscard]] constexpr EnumType get_enum_value_by_name(std::string_view name) EXT_THROWS()
{
    static_assert(std::is_enum_v<EnumType>, "Type is not a enum");
    static_assert(MaxEnumValue > MinEnumValue, "Invalid params");

    constexpr auto enumValuesWithNames = get_enum_values_with_names<EnumType, MinEnumValue, MaxEnumValue>();
    
    const auto it = std::find_if(enumValuesWithNames.cbegin(), enumValuesWithNames.cend(),
        [name](const std::pair<EnumType, std::string_view>& item) {
            return item.second == name;        
        });
    EXT_EXPECT(it != enumValuesWithNames.cend());
    return it->first;
}

} // namespace ext::reflection