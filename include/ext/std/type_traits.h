#pragma once

#include <type_traits>

namespace std {

template<typename T>
using remove_const_ref_t = std::remove_const_t<std::remove_reference_t<T>>;

template<typename T>
using remove_const_pointer_t = std::remove_const_t<std::remove_pointer_t<T>>;

/*
 * True for:
 * decay_same_t<int, int>
 * decay_same_t<int&, int>
 * decay_same_t<int&&, int>
 * decay_same_t<const int&, int>
 * decay_same_t<int[2], int*>
 * decay_same_t<int(int), int(*)(int)>
*/
template <typename T, typename U>
inline constexpr auto decay_same_v = std::is_same_v<decay_t<T>, decay_t<U>>;

// Extract template type, ComPtr<ISerializable> => ISerializable, std::shared_ptr<int> => int
template<typename U>
struct extract_value_type { typedef U value_type; };
template<typename U>
struct extract_value_type<std::unique_ptr<U>> { typedef U value_type; };
template<template<typename> class X, typename U>
struct extract_value_type<X<U>> { typedef U value_type; };
template<class U>
using extract_value_type_v = typename extract_value_type<std::remove_const_t<U>>::value_type;

} // namespace std