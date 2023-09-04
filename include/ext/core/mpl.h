#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

#include <ext/core/defines.h>

namespace ext::mpl {

template <typename TypeToFound, typename... Types>
struct contain_type;

template <typename TypeToFound, typename Type>
struct contain_type<TypeToFound, Type> : std::is_same<TypeToFound, Type> {};

template <typename TypeToFound, typename Type, typename... OtherTypes>
struct contain_type<TypeToFound, Type, OtherTypes...>
{
    static constexpr bool value = contain_type<TypeToFound, Type>::value || contain_type<TypeToFound, OtherTypes...>::value;
};

template <typename TypeToFound, typename... Types>
inline constexpr bool contain_type_v = contain_type<TypeToFound, Types...>::value;

/*
Get type from types list by index
find_type_by_index<0, 0, int, float>::type == int;
find_type_by_index<0, 1, int, float>::type == float;
*/
template<size_t CurrentIndex, size_t SearchedIndex, typename... Types>
struct find_type_by_index;

// CurrentIndex == SearchedIndex and no more types are left
template <size_t SearchedIndex, typename CurrentType>
struct find_type_by_index<SearchedIndex, SearchedIndex, CurrentType>
{
    using type = CurrentType;
};

// CurrentIndex == SearchedIndex and more types are left
template <size_t SearchedIndex, typename CurrentType, typename... Types>
struct find_type_by_index<SearchedIndex, SearchedIndex, CurrentType, Types...>
{
    using type = CurrentType;
};

// CurrentIndex != SearchedIndex and no more types are left
template <size_t CurrentIndex, size_t SearchedIndex, typename CurrentType, typename... Types>
struct find_type_by_index<CurrentIndex, SearchedIndex, CurrentType, Types...>
{
    using type = typename find_type_by_index<CurrentIndex + 1, SearchedIndex, Types...>::type;
};

// Index out of bounds
template <size_t CurrentIndex, size_t SearchedIndex, typename CurrentType>
struct find_type_by_index<CurrentIndex, SearchedIndex, CurrentType>
{
    using type = typename CurrentType::index_out_of_bounds;
};

/*
Get type from types list by index
get_type_t<0, int, float> == int;
get_type_t<1, int, float> == float;
*/
template <size_t SearchedIndex, typename... Types>
using get_type_t = typename find_type_by_index<0, SearchedIndex, Types...>::type;

/*
Multiple template arguments list

Example:
struct struct_first  { void test() {} };
struct struct_second { void test() {} };

ext::mpl::list<struct_first, struct_second> list;
*/
template <typename... T>
struct list : std::tuple<T...>
{
    /*
    * Call function for each type in list
    * !!! Do not forget that the function will receive a pointer to the class to avoid unnecessary constructions.
    *
    * Example:
    * ext::mpl::list<struct_first, struct_second>::ForEachType([](auto* type)
        {
            std::remove_pointer_t<decltype(type)>().test();
        });
    */
    template <typename Function>
    constexpr static void ForEachType(Function&& function)
    {
        std::apply([&function](auto&&... val)
                   {
                       ((function(std::forward<decltype(val)>(val))), ...);
                   }, std::tuple<T*...>());
    }

    // Get count of elements
    EXT_NODISCARD constexpr static auto Count()
    {
        return std::tuple_size_v<std::tuple<T*...>>;
    }

    // Get element from list
    template <std::size_t index>
    EXT_NODISCARD constexpr decltype(auto) Get()
    {
        return std::get<index>(*this);
    }

    // Get list type
    // list<bool, float>::ElementType<1> == float&&)
    template <std::size_t index>
    using ElementType = get_type_t<index, T...>;

    // Get element from list
    template <std::size_t index>
    EXT_NODISCARD constexpr decltype(auto) GetItem()
    {
        return std::get<index>(*this);
    }
};

/*
* Add template parameter to obj
* push_back<list<int, bool>, long> => list<int, bool, long>
*/
template <typename L, typename... T>
struct push_back_impl
{};
template <template <typename...> class L, typename... U, typename... T>
struct push_back_impl<L<U...>, T...>
{
    using type = L<U..., T...>;
};
template <typename L, typename... T>
using push_back = typename push_back_impl<L, T...>::type;

/*
* apply function to types list
* ssh::mpl::apply<std::is_constructible, list<Test, int>> => std::is_constructible<Test, int>
*/
template <template <typename...> class Function, typename List>
struct apply_impl
{};
template <template <typename...> class Function, template <typename...> class List, typename... Types>
struct apply_impl<Function, List<Types...>>
{
    using result = Function<Types...>;
};
template <template <typename...> class Function, typename List>
using apply = typename apply_impl<Function, List>::result;

/*
* Help struct to call function for each element in list.
* !!! Do not forget that the function will receive a pointer to the class to avoid unnecessary constructions.
*
* Example:
*
* ext::mpl::list<struct_first, struct_second> list;
* ext::mpl::ForEach<decltype(list)>::Call([](auto* ss)
    {
        std::remove_pointer_t<decltype(type)>().test();
    });
*/
template<typename T>
struct ForEach;

template<typename Type, typename... TConverters>
struct ForEach<list<Type, TConverters...>>
{
    template<typename Function>
    static void Call(Function&& function)
    {
        ForEach<list<Type>>::Call(std::forward<Function>(function));
        ForEach<list<TConverters...>>::Call(std::forward<Function>(function));
    }
};

template<typename Type>
struct ForEach<list<Type>>
{
    static void Call(std::function<void(Type*)>&& function)
    {
        Type* converter = nullptr;
        function(converter);
    }
};

namespace visitor {

template <class... Types>
struct Visitor;

template <class Type, class... RestTypes>
struct Visitor<Type, RestTypes...> : Type, Visitor<RestTypes...>
{
    Visitor(Type&& type, RestTypes&&... restTypes)
        : Type(type)
        , Visitor<RestTypes...>(std::forward<RestTypes>(restTypes)...)
    {}

    using Type::operator();
    using Visitor<RestTypes...>::operator();
};

template <class Type>
struct Visitor<Type> : Type
{
    explicit Visitor(Type&& type)
        : Type(type)
    {}

    using Type::operator();
};
} // namespace visitor

/*
  using my_variant = std::variant<int, long>;
  my_variant types = 0;
  std::visit(ext::mpl::make_visitor([](int t)  { std::cout << "int"  << t; },
                                    [](long t) { std::cout << "long" << t; }),
             types);
Better than:
  std::visit([](auto t) { std::cout << t; }, types);
*/
template <class... Types>
auto make_visitor(Types&&... types)
{
    return visitor::Visitor<Types...>(std::forward<Types>(types)...);
}

} // namespace ext::mpl
