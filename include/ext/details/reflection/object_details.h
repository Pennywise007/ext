#pragma once

#include <type_traits>
#include <string_view>

#include <ext/core/mpl.h>
#include <ext/std/type_traits.h>

#include <ext/std/source_location.h>

namespace ext::reflection::details {

// We support only clang, linux and msvc with __builtin_FUNCSIG on board
#if defined(__clang__) || defined(__GNUC__) || (defined(_MSC_VER) && _MSC_VER >= 1935)
#define OBJECT_NAME_AVAILABLE 1
#else
#define OBJECT_NAME_AVAILABLE 0
#endif

// Using __builtin_ functions to get object name with template type
template <auto Name>
[[nodiscard]] constexpr std::string_view get_name_impl() {
#if !OBJECT_NAME_AVAILABLE
#if defined(_MSC_VER)
    static_assert(false, "Only MSVS with version 1935 or higher supports this function.");
#else
    static_assert(false, "Unsupported compiler");
#endif
#elif defined(__clang__) || defined(__GNUC__)
    constexpr auto func_name = std::string_view{__PRETTY_FUNCTION__};

    constexpr auto prefix = "Name = ";
    constexpr auto suffixDelimer = ';';

    constexpr auto split = func_name.substr(0, func_name.find_last_of(suffixDelimer));
    return split.substr(split.find(prefix) + std::string_view(prefix).size());
#elif defined(_MSC_VER)

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20
    constexpr std::string_view func_name = std::source_location::current().function_name();
#else // not C++20
    constexpr auto func_name = std::string_view{__builtin_FUNCSIG()};
#endif // not C++20

    constexpr auto prefix = "get_name_impl<";
    constexpr auto suffix = ">(void)";

    constexpr auto split = func_name.substr(0, func_name.size() - std::string_view(suffix).size());
    return split.substr(split.find(prefix) + std::string_view(prefix).size());
#else
    static_assert(false, "Unsupported compiler");
#endif
}

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20

// Getting name and extracting everything which is after '->' symbol
template <auto Ptr>
[[nodiscard]] consteval auto get_field_name_impl() -> std::string_view {
    constexpr auto name = get_name_impl<Ptr>();
#if defined(__GNUC__)
    constexpr auto ind = name.find_last_of(":") + 1;
    // extracting everything which is after ':' symbol and before ')'
    return name.substr(ind, name.size() - ind - 1);
#else // not GNUC
    // extracting everything which is after '->' symbol
    return name.substr(name.find_last_of("->") + 1);
#endif // not GNUC
}

#endif // C++20

struct convertible_to_any
{
    template <typename AnyType>
    constexpr operator AnyType&& () const noexcept;
};

template <typename ExcludedType>
struct convertible_to_any_except
{
    template <typename AnyType, typename = std::enable_if_t<!std::is_base_of_v<std::remove_const_ref_t<AnyType>, ExcludedType>>>
    constexpr operator AnyType&& () const noexcept;
};

template <typename Type, size_t Index>
struct make_constructor_signature_impl
{
    using type = ext::mpl::push_back<
        typename make_constructor_signature_impl<Type, (Index - 1)>::type,
        convertible_to_any
    >;
};

template <typename Type>
struct make_constructor_signature_impl<Type, 0>
{
    using type = ext::mpl::list<Type>;
};

template <typename Type>
struct make_constructor_signature_impl<Type, 1>
{
    // excluding copy constructor
    using type = ext::mpl::list<Type, convertible_to_any_except<Type>>;
};

template <typename Type>
struct make_constructor_signature_impl<Type, 2>
{
    using type = ext::mpl::list<Type, convertible_to_any, convertible_to_any>;
};

template <typename Type, size_t Index>
using make_constructor_signature = typename make_constructor_signature_impl<Type, Index>::type;

template <typename Type, size_t Index>
inline constexpr bool constructible = ext::mpl::apply<std::is_constructible, make_constructor_signature<Type, Index>>::value;

template <typename Type, size_t Index, size_t End>
struct find_min_constructor : std::conditional_t<
        constructible<Type, Index>,
        make_constructor_signature<Type, Index>,
        find_min_constructor<Type, Index + 1, End>
    >
{};

template <typename Type, size_t End>
struct find_min_constructor<Type, End, End>
{
    using type = typename Type::error_constructor_signature_is_not_recognized;
};

template <class T, class... TArgs>
decltype(void(T{std::declval<TArgs>()...}), std::true_type{}) test_is_braces_constructible(int);
template <class, class...>
std::false_type test_is_braces_constructible(...);

template <class T, class... TArgs>
using is_braces_constructible = decltype(test_is_braces_constructible<T, TArgs...>(0));

template <typename Type, size_t Index>
inline constexpr bool braces_constructible = ext::mpl::apply<is_braces_constructible, make_constructor_signature<Type, Index>>::value;

template <typename Type, size_t Index, size_t End>
struct find_max_brace_constructor : std::conditional_t<
        braces_constructible<Type, Index>,
        make_constructor_signature<Type, Index>,
        find_max_brace_constructor<Type, Index - 1, End>
    >
{};

template <typename Type, size_t End>
struct find_max_brace_constructor<Type, End, End>
{
    // Failed to find a constructor signature, probably Type is not constructible with braces
    using type = typename Type::error_constructor_signature_is_not_recognized;
};

} // namespace ext::reflection::details