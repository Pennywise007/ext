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
_INLINE_VAR constexpr auto decay_same_v = std::is_same_v<decay_t<T>, decay_t<U>>;

} // namespace std