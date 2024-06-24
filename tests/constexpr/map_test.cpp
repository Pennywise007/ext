#include "gtest/gtest.h"

#include <string_view>

#include <ext/constexpr/map.h>

using namespace std::literals::string_view_literals;

static constexpr std::array kStringString = {std::pair{"one"sv, "1"sv}, std::pair{"two"sv, "2"sv}, std::pair{"three"sv, "3"sv}};
static constexpr std::array kIntString = {std::pair{1, "one"sv}, std::pair{2, "two"sv}, std::pair{3, "three"sv}};
static constexpr std::array kStringInt = {std::pair{"one"sv, 1}, std::pair{"two"sv, 2}, std::pair{"three"sv, 3}};

TEST(constexpr_map_test, at_string_string) {
    constexpr auto arrayPair = ext::constexpr_map<std::string_view, std::string_view, kStringString.size()>{{kStringString}};

    static_assert("1" == arrayPair.get_value("one"));

    EXPECT_EQ("1", arrayPair.get_value("one"));
    EXPECT_EQ("2", arrayPair.get_value("two"));
    EXPECT_EQ("3", arrayPair.get_value("three"));
    EXPECT_THROW((void)arrayPair.get_value("four"), std::out_of_range);
}

TEST(constexpr_map_test, at_int_string) {
    constexpr auto arrayPair = ext::constexpr_map<int, std::string_view, kStringString.size()>{{kIntString}};

    static_assert("one" == arrayPair.get_value(1));
    EXPECT_EQ("one", arrayPair.get_value(1));
    EXPECT_EQ("two", arrayPair.get_value(2));
    EXPECT_EQ("three", arrayPair.get_value(3));
    EXPECT_THROW((void)arrayPair.get_value(4), std::out_of_range);
}

TEST(constexpr_map_test, at_string_int) {
    constexpr auto arrayPair = ext::constexpr_map<std::string_view, int, kStringString.size()>{{kStringInt}};

    static_assert(1 == arrayPair.get_value("one"));
    EXPECT_EQ(1, arrayPair.get_value("one"));
    EXPECT_EQ(2, arrayPair.get_value("two"));
    EXPECT_EQ(3, arrayPair.get_value("three"));
    EXPECT_THROW((void)arrayPair.get_value("four"), std::out_of_range);
}

TEST(constexpr_map_test, create_array_from_initializer_list) {
    constexpr ext::constexpr_map array = {{std::pair{11, 10}, {std::pair{22, 33}}}};
    static_assert(array.size() == 2);

    static_assert(10 == array.get_value(11));
    static_assert(33 == array.get_value(22));
    EXPECT_EQ(33, array.get_value(22));
}

TEST(constexpr_map_test, create_array_from_several_pairs) {
    constexpr ext::constexpr_map arrayWithTemplateDeductionGuide = {std::pair{1, 2}, std::pair{2, 3}};
    constexpr ext::constexpr_map arrayWithTemplateDeductionGuideOneElement = {std::pair{1, 2}};

    static_assert(arrayWithTemplateDeductionGuideOneElement.size() == 1);

    static_assert(arrayWithTemplateDeductionGuide.size() == 2);
    static_assert(arrayWithTemplateDeductionGuide.get_value(1) == 2);
    EXPECT_EQ(3, arrayWithTemplateDeductionGuide.get_value(2));
}

TEST(constexpr_map_test, get_key) {
    constexpr ext::constexpr_map array = {{std::pair{"zero", 10}, std::pair{"one", 33}, std::pair{"two", 55}}};
    static_assert(array.size() == 3);
    static_assert(std::string_view("one") == array.get_key(33));

    EXPECT_STREQ("zero", array.get_key(10));
    EXPECT_STREQ("one", array.get_key(33));
    EXPECT_STREQ("two", array.get_key(55));
    EXPECT_THROW((void)array.get_key(0), std::out_of_range);
}

TEST(constexpr_map_test, contains_key) {
    constexpr ext::constexpr_map array = {{std::pair{11, 10}, std::pair{22, 33}, std::pair{44, 55}}};
    static_assert(array.size() == 3);

    static_assert(array.contains_key(11));
    static_assert(array.contains_key(22));
    static_assert(array.contains_key(44));
    static_assert(!array.contains_key(0));
}

TEST(constexpr_map_test, contains_value) {
    constexpr ext::constexpr_map array = {{std::pair{11, 10}, std::pair{22, 33}, std::pair{44, 55}}};
    static_assert(array.size() == 3);

    static_assert(array.contains_value(10));
    static_assert(array.contains_value(33));
    static_assert(array.contains_value(55));
    static_assert(!array.contains_value(0));
}

TEST(constexpr_map_test, get_value_or) {
    constexpr ext::constexpr_map array = {{std::pair{11, 10}, std::pair{22, 33}, std::pair{44, 55}}};
    static_assert(array.size() == 3);

    static_assert(array.get_value_or(11, 99) == 10);
    static_assert(array.get_value_or(-1, 99) == 99);
}

TEST(constexpr_map_test, get_key_or) {
    constexpr ext::constexpr_map array = {{std::pair{11, 10}, std::pair{22, 33}, std::pair{44, 55}}};
    static_assert(array.size() == 3);

    static_assert(array.get_key_or(33, 99) == 22);
    static_assert(array.get_key_or(-1, 99) == 99);
}

TEST(constexpr_map_test, contains_duplicate_keys) {
    constexpr ext::constexpr_map arrayWithoutDuplicateValues = {{std::pair{1, 2}, std::pair{2, 3}}};
    constexpr ext::constexpr_map arrayWithDuplicateKeys = {{std::pair{1, 2}, std::pair{1, 2}}};

    static_assert(!arrayWithoutDuplicateValues.contain_duplicate_keys());
    static_assert(!arrayWithoutDuplicateValues.contain_duplicate_values());
    static_assert(arrayWithDuplicateKeys.contain_duplicate_keys());
    static_assert(arrayWithDuplicateKeys.contain_duplicate_values());
}

TEST(constexpr_map_test, find_key_index) {
    constexpr ext::constexpr_map array = {{std::pair{1, 2}, std::pair{2, 3}}};

    static_assert(0 == array.find_key_index(1).get());
    static_assert(1 == array.find_key_index(2).get());
    static_assert(array.find_key_index(2).valid());
    static_assert(!array.find_key_index(-100).valid());
}

TEST(constexpr_map_test, find_value_index) {
    constexpr ext::constexpr_map array = {{std::pair{1, 2}, std::pair{2, 3}}};

    static_assert(0 == array.find_value_index(2).get());
    static_assert(1 == array.find_value_index(3).get());
    static_assert(array.find_value_index(3).valid());
    static_assert(!array.find_key_index(-100).valid());
}
