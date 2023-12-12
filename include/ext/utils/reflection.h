#pragma once

#include <type_traits>

#include <ext/details/reflection_details.h>

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

// Check if object got a field
#define HAS_FIELD(Type, FieldName)                              \
    []() {                                                      \
        auto check = [](auto u) -> decltype(u.FieldName) {};    \
        return std::is_invocable_v<decltype(check), Type>;      \
    }()


} // namespace ext::reflection