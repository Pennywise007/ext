#pragma once

#include <type_traits>

#include <ext/details/reflection/object_details.h>
#include <ext/details/reflection/object_fields_details.h>

namespace ext::reflection {

/*  Get object minimum constructor size.
    Example:
struct TestStruct  {
    TestStruct() = default;
    TestStruct(int) {}
};
static_assert(ext::reflection::constructor_size<TestStruct> == 0);*/
template <typename T, size_t MaxConstructorSize = 16>
inline constexpr size_t constructor_size = ext::reflection::details::find_min_constructor<T, 0, MaxConstructorSize>::Count() - 1;

/*  Check if object constructible with {}, useful for getting fields count.
    Note: object shouldn't have a constructors.
    Example:
struct TestStruct  {
    std::string field_1;
    std::string field_2;
};
static_assert(ext::reflection::brace_constructor_size<TestStruct> == 2);*/
template <typename T, size_t MaxConstructorSize = 100>
inline constexpr size_t brace_constructor_size = ext::reflection::details::find_max_brace_constructor<T, MaxConstructorSize, 0>::Count() - 1;

/*
Getting object fields(works with public fields only)
Example:

struct Foo {
    int intField;
    bool booleanField;
};

Foo object {};
constexpr std::tie<int&, bool&> fields = get_object_fields(object);
std::get<0>(fields) = 5;
std::get<1>(fields) = true;
*/
template <typename Type>
constexpr auto get_object_fields(Type&& obj)
{
    constexpr auto fieldsCount = brace_constructor_size<std::decay_t<Type>>;
    static_assert(fieldsCount != 0, "Failed to determine fields count");

    return details::get_object_fields_impl<fieldsCount>(obj);
}

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20

/*
Getting field name
Example:

struct Foo {
    int intField;
    bool booleanField;
};

ext::reflection::get_field_name<Foo, 0> == "intField"
ext::reflection::get_field_name<Foo, 1> == "booleanField"
*/
template<class Type, auto FieldIndex>
constexpr auto get_field_name = details::get_field_name_impl<&std::get<FieldIndex>(get_object_fields(details::external<Type>))>();
#endif // C++20

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

} // namespace ext::reflection