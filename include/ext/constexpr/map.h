#pragma once

/*
Implementation of the map which can be used in a compile time.
Example:

    constexpr ext::constexpr_map my_map = {{std::pair{11, 10}, {std::pair{22, 33}}}};
    static_assert(my_map.size() == 2);

    static_assert(10 == my_map.get_value(11));
    static_assert(33 == my_map.get_value(22));
*/

#include <algorithm>
#include <array>
#include <limits>
#include <utility>

namespace ext {

class search_index {
public:
    constexpr search_index(std::size_t val) noexcept : index_(val){};

    // Index of not found element
    [[nodiscard]] static constexpr search_index not_found_index() { return search_index(not_found_index_value()); }

    [[nodiscard]] explicit constexpr operator bool() const { return valid(); }
    [[nodiscard]] constexpr bool valid() const { return index_ != not_found_index_value(); }
    [[nodiscard]] constexpr std::size_t get() const { return index_; }

private:
    [[nodiscard]] inline static constexpr std::size_t not_found_index_value() { return std::numeric_limits<std::size_t>::max(); }

private:
    const std::size_t index_;
};

// constexpr Map, allows to execute search and different checks in a compile time
// Example:
//    constexpr constexpr_map kMapValues = { std::pair{1, "one"}, std::pair{2, "two"}};
//
//    static_assert(!kMapValues.contain_duplicate_keys());
//    static_assert(kMapValues.contains_key(1));
//    static_assert(kMapValues.get_value(1) == "one");
template <typename Key, typename Value, std::size_t Size>
struct constexpr_map : std::array<std::pair<Key, Value>, Size>
{
    using BaseArray = std::array<std::pair<Key, Value>, Size>;

    constexpr constexpr_map(std::array<std::pair<Key, Value>, Size> other) noexcept
        : BaseArray(std::move(other)) {}

    // Creating with std::pair initializer list, Example:
    // constexpr constexpr_map array = {{ std::pair{K1,V1}, std::pair(K2, V2)}}
    constexpr constexpr_map(const std::pair<Key, Value>(&&array)[Size]) 
        : BaseArray(create_array(std::move(array), std::make_index_sequence<Size>{})) {}

    // Creating with std::pair, Example:
    // constexpr constexpr_map array = { std::pair{K1,V1}, std::pair(K2, V2) }
    template <typename... Pair>
    constexpr constexpr_map(std::pair<Key, Value>&& firstPair, Pair&&... otherPairs) 
        : BaseArray({std::move(firstPair), std::forward<Pair>(otherPairs)...}) {}

    [[nodiscard]] constexpr bool contains_key(const Key& key) const noexcept { return find_key_index(key).valid(); }
    [[nodiscard]] constexpr bool contains_value(const Value& value) const noexcept { return find_value_index(value).valid(); }

    // Get value by key, throws out_of_range exception if key not found
    [[nodiscard]] constexpr const Value& get_value(const Key& key) const& { return get_value(find_key_index(key)); }
    // Get value by key, throws out_of_range exception if key not found
    [[nodiscard]] constexpr const Value& get_value(const search_index& arrayIndex) const& { return BaseArray::at(arrayIndex.get()).second; }
    // Search fot value by key, if key not exist - return notFoundValue
    template <typename _Up>
    [[nodiscard]] constexpr Value get_value_or(const Key& key, _Up&& notFoundValue) const noexcept(std::is_nothrow_move_constructible_v<Value>)
    {
        return get_value_or<_Up, 0>(key, std::forward<_Up>(notFoundValue));
    }

    // Get key by value, throws out_of_range exception if value not found
    [[nodiscard]] constexpr const Key& get_key(const Value& value) const& { return get_key(find_value_index(value)); }
    // Get key by value, throws out_of_range exception if value not found
    [[nodiscard]] constexpr const Key& get_key(const search_index& arrayIndex) const& { return BaseArray::at(arrayIndex.get()).first; }
    // Search fot key by value, if value not exist - return notFoundValue
    template <typename _Up>
    [[nodiscard]] constexpr Key get_key_or(const Value& value, _Up&& notFoundValue) const noexcept(std::is_nothrow_move_constructible_v<Key>)
    {
        return get_key_or<_Up, 0>(value, std::forward<_Up>(notFoundValue));
    }

    // Checks if duplicate keys exist
    [[nodiscard]] constexpr bool contain_duplicate_keys() const noexcept { return contain_duplicate_keys<0>(); }

    // Find key index in array, if not found - returns kNotFoundIndex
    [[nodiscard]] constexpr search_index find_key_index(const Key& key) const noexcept { return find_key_index<0>(key); }
    // Find value index in array, if not found - returns kNotFoundIndex
    [[nodiscard]] constexpr search_index find_value_index(const Value& value) const noexcept { return find_value_index<0>(value); }

 private:
    template <std::size_t StartSearchIndex>
    [[nodiscard]] constexpr bool contain_duplicate_keys() const noexcept
    {
        if constexpr (StartSearchIndex >= Size)
            return false;
        else
            return find_key_index<StartSearchIndex + 1>(BaseArray::operator[](StartSearchIndex).first).valid()
                        ? true
                        : contain_duplicate_keys<StartSearchIndex + 1>();
    }

    template <typename _Up, std::size_t StartSearchIndex>
    [[nodiscard]] constexpr Value get_value_or(const Key& key, _Up&& notFoundValue) const noexcept(std::is_nothrow_move_constructible_v<Value>)
    {
        static_assert(std::is_move_constructible_v<Value>);
        static_assert(std::is_convertible_v<_Up&&, Value>);

        if constexpr (StartSearchIndex >= Size)
            return Value(std::forward<_Up>(notFoundValue));
        else
            return (BaseArray::operator[](StartSearchIndex).first == key)
                    ? BaseArray::operator[](StartSearchIndex).second
                    : get_value_or<_Up, StartSearchIndex + 1>(key, std::forward<_Up>(notFoundValue));
    }

    template <typename _Up, std::size_t StartSearchIndex>
    [[nodiscard]] constexpr Key get_key_or(const Value& value, _Up&& notFoundValue) const noexcept(std::is_nothrow_move_constructible_v<Key>)
    {
        static_assert(std::is_move_constructible_v<Key>);
        static_assert(std::is_convertible_v<_Up&&, Key>);

        if constexpr (StartSearchIndex >= Size)
            return Value(std::forward<_Up>(notFoundValue));
        else
            return (BaseArray::operator[](StartSearchIndex).second == value)
                    ? BaseArray::operator[](StartSearchIndex).first
                    : get_key_or<_Up, StartSearchIndex + 1>(value, std::forward<_Up>(notFoundValue));
    }

    template <std::size_t StartSearchIndex>
    [[nodiscard]] constexpr search_index find_key_index(const Key& key) const noexcept
    {
        if constexpr (StartSearchIndex >= Size)
            return search_index::not_found_index();
        else
            return (BaseArray::operator[](StartSearchIndex).first == key)
                ? search_index(StartSearchIndex)
                : find_key_index<StartSearchIndex + 1>(key);
    }

    template <std::size_t StartSearchIndex>
    [[nodiscard]] constexpr search_index find_value_index(const Value& value) const noexcept {
        if constexpr (StartSearchIndex >= Size)
            return search_index::not_found_index();
        else
            return (BaseArray::operator[](StartSearchIndex).second == value)
                ? search_index(StartSearchIndex)
                : find_value_index<StartSearchIndex + 1>(value);
    }

    template <std::size_t... Is>
    [[nodiscard]] constexpr static std::array<std::pair<Key, Value>, Size> 
        create_array(const std::pair<Key, Value>(&&array)[Size], std::index_sequence<Is...>)
    {
        return std::array<std::pair<Key, Value>, Size>{std::move(array[Is])...};
    }
};

// template deduction guide to create pair like: constexpr constexpr_map val = { std::pair{1, 2} };
template <typename Key, typename Value, typename... Pair>
constexpr_map(std::pair<Key, Value>, Pair...)
    -> constexpr_map<std::enable_if_t<(std::is_same_v<std::pair<Key, Value>, Pair> && ...), Key>, Value,
                    sizeof...(Pair) + 1>;

}  // namespace ext