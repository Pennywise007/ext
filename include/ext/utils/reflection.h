#pragma once

#include <type_traits>
#include <string_view>

#include <ext/details/reflection_details.h>

#include <ext/std/source_location.h>

namespace ext::reflection {

/*  Get object minimum constructor size.
    Example:
struct TestStruct  {
    TestStruct() = default;
    TestStruct(int) {}
};
static_assert(ext::reflection::constructor_size<TestStruct> == 0);*/
template <typename T, size_t MaxConstructorSize = 16>
inline constexpr size_t constructor_size = ext::details::refletion::find_min_constructor<T, 0, MaxConstructorSize>::Count() - 1;

/*  Check if object constructible with {}, usefull for getting fields count.
    Note: object shouldn't have a constructors.
    Example:
struct TestStruct  {
    std::string field_1;
    std::string field_2;
};
static_assert(ext::reflection::brace_constructor_size<TestStruct> == 2);*/
template <typename T, size_t MaxConstructorSize = 100>
inline constexpr size_t brace_constructor_size = ext::details::refletion::find_max_brace_constructor<T, MaxConstructorSize, 0>::Count() - 1;

///////////////////////////////////////////////////////////////////////////////////////////////////
// Check if object has a field
#define HAS_FIELD(Type, FieldName)                                      \
    []() {                                                              \
        constexpr auto check = [](auto u) -> decltype(u.FieldName) {};  \
        return std::is_invocable_v<decltype(check), Type>;              \
    }()

// Check if object has a function
#define HAS_FUNCTION(Type, FunctionName)                                                \
    []() {                                                                              \
        constexpr auto check = [](auto u) -> decltype(&decltype(u)::FunctionName) {};   \
        return std::is_invocable_v<decltype(check), Type>;                              \
    }()

///////////////////////////////////////////////////////////////////////////////////////////////////
// Old Fashioned way to check field/function exist. Can be useful when you need to check private fields.

// Declare checker for field existing, checks if object has field by name
// Example:
//      struct A { bool someField; };
//      DECLARE_CHECK_FIELD_EXISTS(someField);
//      static_assert(has_someField_field_v<A>);
#define DECLARE_CHECK_FIELD_EXISTS(FieldName)                       \
template<typename T>                                                \
struct has_##FieldName##_field {                                    \
    template<typename U, typename = decltype(std::declval<U>().FieldName)>  \
    static std::true_type test(int);                                \
    template<typename>                                              \
    static std::false_type test(...);                               \
    static constexpr bool value = decltype(test<T>(0))::value;      \
};                                                                  \
template<class T>                                                   \
inline constexpr bool has_##FieldName##_field_v = has_##FieldName##_field<T>::value

// Declare checker for function name, checks if object has function by name
// Example:
//      DECLARE_CHECK_FUNCTION(reserve);
//      static_assert(has_reserve_function_v<std::vector<int>>);
#define DECLARE_CHECK_FUNCTION_EXISTS(FunctionName)                 \
template<typename Class, class = std::void_t<>>                     \
struct has_##FunctionName##_function : std::false_type {};          \
template<typename Class>                                            \
struct has_##FunctionName##_function<Class, std::enable_if_t<std::is_member_function_pointer_v<decltype(&Class::FunctionName)>>> : std::true_type {}; \
template<class T>                                                   \
inline constexpr bool has_##FunctionName##_function_v = has_##FunctionName##_function<T>::value
///////////////////////////////////////////////////////////////////////////////////////////////////

template <auto EnumType>
[[nodiscard]] constexpr std::string_view get_enum_name() {
#if defined(__clang__) && defined(_MSC_VER)
    // For Clang on Windows we use a compiler-specific macro
    constexpr auto func_name = std::string_view{__PRETTY_FUNCTION__};
#else
#if _HAS_CXX20
    constexpr auto func_name =
        std::string_view{std::source_location::current().function_name()};
#else // _HAS_CXX20
    // For c++17 we need to use __builtin_FUNCSIG instead of __builtin_FUNCTION
    constexpr auto func_name = std::string_view{__builtin_FUNCSIG()};
#endif // _HAS_CXX20
#endif // defined(__clang__) && defined(_MSC_VER)

#if defined(__clang__) || defined(__GNUC__)
    constexpr auto split = func_name.substr(0, func_name.size() - 1);
    return split.substr(split.find("EnumType = ") + 4);
#elif defined(_MSC_VER)
    constexpr auto prefix = "get_enum_name<";
    constexpr auto suffix = ">(void)";
    constexpr auto split = func_name.substr(0, func_name.size() - std::string_view(suffix).size());
    return split.substr(split.find(prefix) + std::string_view(prefix).size());
#else
    static_assert(false, "Unsupported compiler");
#endif
}

template <class EnumType>
constexpr bool is_scoped_enum_v = std::is_enum_v<EnumType> &&
    !std::is_convertible_v<EnumType, std::underlying_type_t<EnumType>>;

template <class EnumType, auto EnumValue>
[[nodiscard]] constexpr bool is_enum_value()
{
    // value not belong to enum, example: "(enum TestEnum)0xa"
    return get_enum_name<static_cast<EnumType>(EnumValue)>()[0] != '(';
}

template <class EnumType, int minValue, int maxValue, int i>
[[nodiscard]] constexpr unsigned get_enum_size_impl()
{
    if constexpr (i > maxValue)
        return 0;
    else
    {
        if constexpr (!is_enum_value<EnumType, i>())
            return get_enum_size_impl<EnumType, minValue, maxValue, i + 1>();
        else
            return 1 + get_enum_size_impl<EnumType, minValue, maxValue, i + 1>();
    }
}

template <class EnumType, int indexToSearch, int startSearchValue = 0, int endSearchValue = 100, int i, unsigned foundItems>
[[nodiscard]] constexpr EnumType get_enum_value_impl()
{
    if constexpr (i > endSearchValue)
        return 0;
    else
    {
        constexpr auto name = get_enum_name<static_cast<EnumType>(i)>();
        if constexpr (!is_enum_value<EnumType, i>())
            return get_enum_value_impl<EnumType, indexToSearch, startSearchValue, endSearchValue, i + 1, foundItems>();
        else if constexpr (indexToSearch == foundItems)
            return static_cast<EnumType>(i);
        else
            return get_enum_value_impl<EnumType, indexToSearch, startSearchValue, endSearchValue, i + 1, foundItems + 1>();
    }
}

template <class EnumType, int startSearchValue = 0, int endSearchValue = 100>
[[nodiscard]] constexpr unsigned get_enum_size()
{
    static_assert(is_scoped_enum_v<EnumType>, "Type is not a enum type");
    static_assert(endSearchValue > startSearchValue, "Invalid params");

    return get_enum_size_impl<EnumType, startSearchValue, endSearchValue, startSearchValue>();
}

template <class EnumType, unsigned searchIndex, int startSearchValue = 0, int endSearchValue = 100>
[[nodiscard]] constexpr EnumType get_enum_value()
{
    static_assert(is_scoped_enum_v<EnumType>, "Type is not a enum type");
    static_assert(endSearchValue > startSearchValue, "Invalid params");
    static_assert(get_enum_size<EnumType, startSearchValue, endSearchValue>() > searchIndex, "Value with given index doesn't exist in enum");

    return get_enum_value_impl<EnumType, searchIndex, startSearchValue, endSearchValue, startSearchValue, 0>();
}

} // namespace ext::reflection