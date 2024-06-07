#include "gtest/gtest.h"

#include <string_view>

#include <ext/constexpr/string.h>

using ext::constexpr_string;

constexpr constexpr_string textFirst = "test";
constexpr constexpr_string textSecond = "second";

TEST(constexpr_string_test, text_and_size) {
    static_assert(textFirst == std::string_view("test"));
    static_assert(textFirst.size() == 4);
    static_assert(textSecond.str() == std::string_view("second"));
    static_assert(textSecond.size() == 6);

    constexpr constexpr_string emptyText = "";
    static_assert(emptyText == std::string_view(""));
    static_assert(emptyText.size() == 0);
}

TEST(constexpr_string_test, empty_strings) {
    constexpr constexpr_string emptyText = "";
    constexpr constexpr_string nonEmptyText = "test";

    static_assert(emptyText + nonEmptyText == std::string_view("test"));
    static_assert(nonEmptyText + emptyText == std::string_view("test"));
    static_assert(emptyText + emptyText == std::string_view(""));
}

TEST(constexpr_string_test, chars) {
    static_assert(textFirst[0] == 't');
    static_assert(textFirst[1] == 'e');
    static_assert(textFirst[2] == 's');
    static_assert(textFirst[3] == 't');
    static_assert(textFirst[4] == '\0');
}

TEST(constexpr_string_test, uniting_text) {
    static_assert(textFirst + "_" + textSecond == std::string_view("test_second"));
    static_assert("some_" + textFirst == std::string_view("some_test"));
}

TEST(constexpr_string_test, equal_text) {
    static_assert(constexpr_string("wow") == constexpr_string("wow"));
    static_assert(constexpr_string("text") == "text");
    static_assert("text" == constexpr_string("text"));
}

TEST(constexpr_string_test, not_equal_text) {
    static_assert(constexpr_string("wow") != constexpr_string("waw"));
    static_assert(constexpr_string("wow") != constexpr_string("wo"));
    static_assert(constexpr_string("text") != "tost");
    static_assert("text" != constexpr_string("tost"));
}

TEST(constexpr_string_test, string_view) {
    constexpr std::string_view sw = textFirst;
    static_assert(sw == "test");
}
