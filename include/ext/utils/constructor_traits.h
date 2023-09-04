#pragma once

#include <type_traits>

#include <ext/core/mpl.h>
#include <ext/std/type_traits.h>

namespace ext::detail {
namespace constructor {

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

template <typename Type, size_t Index, size_t End>
struct find_constructor : std::conditional_t<
        ext::mpl::apply<std::is_constructible, make_constructor_signature<Type, Index>>::value,
        make_constructor_signature<Type, Index>,
        find_constructor<Type, Index + 1, End>
    >
{};

template <typename Type, size_t End>
struct find_constructor<Type, End, End>
{
    using type = typename Type::error_constructor_signature_is_not_recognized;
};
} // namespace constructor

template <typename T>
inline constexpr size_t constructor_size = constructor::find_constructor<T, 0, 16>::Count() - 1;

} // namespace ext::detail