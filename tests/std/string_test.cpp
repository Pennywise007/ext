#include "gtest/gtest.h"

#include <ext/std/string.h>

TEST(string_test, string_sprintf)
{
    EXPECT_STREQ(std::string_sprintf("test %d - %s - %S", 10, "wow", L"test").c_str(), "test 10 - wow - test");
    EXPECT_STREQ(std::string_sprintf("test %%s %%S", "wow", L"tt").c_str(), "test %s %S");
    EXPECT_STREQ(std::string_sprintf("test %%%s %%%S", "wow", L"tt").c_str(), "test %wow %tt");
    EXPECT_STREQ(std::string_sprintf("test /%%s /%%S", "wow", L"tt").c_str(), "test /%s /%S");
    EXPECT_STREQ(std::string_sprintf("test %/%s %/%S", "wow", L"tt").c_str(), "test /wow /tt");
    EXPECT_STREQ(std::string_sprintf("test s%% S%%", "wow", L"tt").c_str(), "test s% S%");
}

TEST(string_test, string_swprintf)
{
    EXPECT_STREQ(std::string_swprintf(L"test %d - %s - %S", 10, L"wow", "test").c_str(), L"test 10 - wow - test");
    EXPECT_STREQ(std::string_swprintf(L"test %%s %%S", L"wow", "tt").c_str(), L"test %s %S");
    EXPECT_STREQ(std::string_swprintf(L"test %%%s %%%S", L"wow", "tt").c_str(), L"test %wow %tt");
    EXPECT_STREQ(std::string_swprintf(L"test /%%s /%%S", L"wow", "tt").c_str(), L"test /%s /%S");
    EXPECT_STREQ(std::string_swprintf(L"test %/%s %/%S", L"wow", "tt").c_str(), L"test /wow /tt");
    EXPECT_STREQ(std::string_swprintf(L"test s%% S%%", L"wow", "tt").c_str(), L"test s% S%");
}

TEST(string_test, trim_all)
{
    std::wstring wstring_text = L"  \t\r\nt e\ts\nt\r\na\t\r\n  ";
    std::string_trim_all(wstring_text);
    EXPECT_STREQ(wstring_text.c_str(), L"t e\ts\nt\r\na");

    std::string string_text = "  \t\r\nt e\ts\nt\r\na\t\r\n  ";
    std::string_trim_all(string_text);
    EXPECT_STREQ(string_text.c_str(), "t e\ts\nt\r\na");
}

TEST(string_test, narrow_english)
{
    EXPECT_STREQ(std::narrow(L"test").c_str(), "test");
    EXPECT_STREQ(std::narrow(L"BigTest").c_str(), "BigTest");
}

TEST(string_test, narrow_numbers)
{
    EXPECT_STREQ(std::narrow(L"0123456789").c_str(), "0123456789");
}

TEST(string_test, narrow_cyrillic)
{
    EXPECT_STREQ(std::narrow(L"тест").c_str(), "тест");
    EXPECT_STREQ(std::narrow(L"Большой тест").c_str(), "Большой тест");
}

TEST(string_test, widen_english)
{
    EXPECT_STREQ(std::widen("test").c_str(), L"test");
    EXPECT_STREQ(std::widen("BigTest").c_str(), L"BigTest");
}

TEST(string_test, widen_cyrillic)
{
    EXPECT_STREQ(std::widen("тест").c_str(), L"тест");
    EXPECT_STREQ(std::widen("Большой тест").c_str(), L"Большой тест");
}