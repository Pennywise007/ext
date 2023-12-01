#pragma once

/*
Compile time extension for strings, allow to combine and check text in compile time
Example:
    constexpr ext::constexpr_string textFirst = "test";
    constexpr ext::constexpr_string textSecond = "second";

    constexpr auto TextCombination = textFirst + "_" + textSecond;
    static_assert(TextCombination == "test_second");

In C++20 can be used to store text as a template argument:
    template <ext::constexpr_string name__>
    struct Object {
        constexpr std::string_view Name() const {
            return name__.str();
        }
        ...
    };

    Object<"object_name"> object;
    static_assert(object.Name() == std::string_view("object_name"));
*/

#include <string_view>

#include <ext/std/array.h>

namespace ext {

template <size_t N>
struct constexpr_string {
    constexpr constexpr_string(const char (&_str)[N])
     : value_(std::to_array(_str))
    {}

    constexpr std::string_view str() const {
        return std::string_view(value_.data(), N - 1);
    }
    constexpr size_t size() const {
        return N - 1;
    }

    constexpr char operator[](std::size_t i) const {
        return value_[i];
    }

    template<std::size_t N2>
    constexpr constexpr_string<N+N2-1> operator+(const constexpr_string<N2>& str) const {
        char resultText[N+N2-1] {};
        for (int i = 0; i < N - 1; ++i) {
            resultText[i] = value_[i];
        }
        for (int i = 0; i < N2; ++i) {
            resultText[N + i - 1] = str.value_[i];
        }
        return resultText;
    }

    constexpr bool operator==(const constexpr_string<N> str) const {
        for (int i = 0; i < N; ++i) {
            if (value_[i] != str.value_[i])
                return false;
        }
        return true;
    }

    template<std::size_t N2>
    constexpr bool operator==(const constexpr_string<N2> str) const {
        return false;
    }

    template<std::size_t N2>
    constexpr bool operator!=(const constexpr_string<N2> str) const {
        return !operator==(str);
    }

    const std::array<char, N> value_;
};

template<std::size_t N1, std::size_t N2>
constexpr auto operator+(constexpr_string<N1> cStr, const char (&str)[N2]) {
    return cStr + constexpr_string<N2>(str);
}

template<std::size_t N1, std::size_t N2>
constexpr auto operator+(const char (&str)[N1], constexpr_string<N2> cStr) {
    return constexpr_string<N1>(str) + cStr;
}

template<std::size_t N1, std::size_t N2>
constexpr auto operator==(constexpr_string<N1> cStr, const char (&str)[N2]) {
    return cStr == constexpr_string<N2>(str);
}

template<std::size_t N1, std::size_t N2>
constexpr auto operator==(const char (&str)[N1], constexpr_string<N2> cStr) {
    return constexpr_string<N1>(str) == cStr;
}

template<std::size_t N1, std::size_t N2>
constexpr auto operator!=(constexpr_string<N1> cStr, const char (&str)[N2]) {
    return !operator==(cStr, str);
}

template<std::size_t N1, std::size_t N2>
constexpr auto operator!=(const char (&str)[N1], constexpr_string<N2> cStr) {
    return !operator==(str, cStr);
}

} // namespace ext
