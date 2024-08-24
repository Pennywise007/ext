#include "gtest/gtest.h"

#include <ext/reflection/enum.h>

#if OBJECT_NAME_AVAILABLE

enum class TestEnum
{
    eEnumValue1,
    eEnumValue2,
    eEnumValue5 = 5,
    eEnumValue101 = 101,
};

template <class T>
struct TestClass
{
    enum class InternalEnum
    {
        eInternalEnumValue1,
    };
};

static_assert(ext::reflection::enum_name<TestEnum(0)> == "TestEnum::eEnumValue1",
    "Failed to get enum value name");
static_assert(ext::reflection::enum_name<TestEnum::eEnumValue2> == "TestEnum::eEnumValue2",
    "Failed to get enum value name");
static_assert(ext::reflection::enum_name<TestEnum(101)> == "TestEnum::eEnumValue101",
    "Failed to get enum value name");
static_assert(ext::reflection::enum_name<TestClass<int>::InternalEnum::eInternalEnumValue1> == "TestClass<int>::InternalEnum::eInternalEnumValue1",
    "Failed to get internal enum value name");

#if defined(__clang__) || defined(__GNUC__)
    static_assert(ext::reflection::enum_name<TestEnum(-1)> == "(TestEnum)-1",
        "Failed to get enum value of non existing enum value");
#elif defined(_MSC_VER)
    static_assert(ext::reflection::enum_name<TestEnum(-1)> == "(enum TestEnum)0xffffffffffffffff",
        "Failed to get enum value of non existing enum value");
#else
    static_assert(false, "Unsupported compiler");
#endif

static_assert(ext::reflection::is_enum_value<TestEnum, TestEnum::eEnumValue1>(),
    "Failed to detect if value belongs to enum");
static_assert(ext::reflection::is_enum_value<TestEnum, TestEnum::eEnumValue2>(),
    "Failed to detect if value belongs to enum");
static_assert(!ext::reflection::is_enum_value<TestEnum, -100>(),
    "Failed to detect if value belongs to enum");
    
static_assert(ext::reflection::get_enum_size<TestEnum>() == 3,
    "Failed to calc enum size");
static_assert(ext::reflection::get_enum_size<TestEnum, 0, 101>() == 4,
    "Failed to calc enum size with 101 elements");

static_assert(ext::reflection::get_enum_value<TestEnum, 0>() == TestEnum::eEnumValue1);
static_assert(ext::reflection::get_enum_value<TestEnum, 1>() == TestEnum::eEnumValue2);
static_assert(ext::reflection::get_enum_value<TestEnum, 2>() == TestEnum::eEnumValue5);
static_assert(ext::reflection::get_enum_value<TestEnum, 3, 0, 101>() == TestEnum::eEnumValue101);

TEST(reflection_enum, is_enum_value)
{
    EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(TestEnum::eEnumValue1));
    EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(0));
    EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(TestEnum::eEnumValue2));
    EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(1));
    EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(TestEnum::eEnumValue5));
    EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(5));
    EXPECT_FALSE(ext::reflection::is_enum_value<TestEnum>(-1));
    EXPECT_FALSE(ext::reflection::is_enum_value<TestEnum>(3));
}

TEST(reflection_enum, get_enum_values)
{
    EXPECT_EQ((ext::reflection::get_enum_values<TestEnum>()), (std::array{ TestEnum::eEnumValue1, TestEnum::eEnumValue2, TestEnum::eEnumValue5 }));
    EXPECT_EQ((ext::reflection::get_enum_values<TestEnum, 0, 101>()), (std::array{ TestEnum::eEnumValue1, TestEnum::eEnumValue2, TestEnum::eEnumValue5, TestEnum::eEnumValue101 }));
}

template <typename T, std::size_t N>
constexpr bool compare_arrays(const std::array<T, N>& a, const std::array<T, N>& b)
{
    for (std::size_t i = 0; i < N; ++i)
    {
        if (a[i] != b[i])
            return false;
    }
    return true;
}

using namespace std::literals;
static_assert(compare_arrays(ext::reflection::get_enum_values_with_names<TestEnum>(), std::array{
    std::pair{TestEnum::eEnumValue1, "TestEnum::eEnumValue1"sv},
    std::pair{TestEnum::eEnumValue2, "TestEnum::eEnumValue2"sv},
    std::pair{TestEnum::eEnumValue5, "TestEnum::eEnumValue5"sv},
}));

TEST(reflection_enum, values_search)
{
    EXPECT_STREQ(std::string(ext::reflection::enum_to_string(TestEnum::eEnumValue1)).c_str(), "TestEnum::eEnumValue1");
    EXPECT_STREQ(std::string(ext::reflection::enum_to_string(TestEnum::eEnumValue2)).c_str(), "TestEnum::eEnumValue2");
    EXPECT_STREQ(std::string(ext::reflection::enum_to_string(TestEnum::eEnumValue5)).c_str(), "TestEnum::eEnumValue5");
    
    EXPECT_EQ(ext::reflection::get_enum_value_by_name<TestEnum>("TestEnum::eEnumValue1"), TestEnum::eEnumValue1);
    EXPECT_EQ(ext::reflection::get_enum_value_by_name<TestEnum>("TestEnum::eEnumValue2"), TestEnum::eEnumValue2);
    EXPECT_EQ(ext::reflection::get_enum_value_by_name<TestEnum>("TestEnum::eEnumValue5"), TestEnum::eEnumValue5);
}

enum UnscopedEnum
{
    eValue1,
    eValue2
};

static_assert(ext::reflection::enum_name<UnscopedEnum(0)> == "eValue1",
    "Failed to get unscoped enum value name");
static_assert(ext::reflection::enum_name<UnscopedEnum::eValue2> == "eValue2",
    "Failed to get unscoped enum value name");

static_assert(ext::reflection::is_enum_value<UnscopedEnum, UnscopedEnum::eValue1>(),
    "Failed to detect if value belongs to unscoped enum");
static_assert(!ext::reflection::is_enum_value<UnscopedEnum, -1>(),
    "Failed to detect if value belongs to unscoped enum");

static_assert(ext::reflection::get_enum_size<UnscopedEnum>() == 2,
    "Failed to calc unscoped enum size");

static_assert(ext::reflection::get_enum_value<UnscopedEnum, 0>() == UnscopedEnum::eValue1);
static_assert(ext::reflection::get_enum_value<UnscopedEnum, UnscopedEnum::eValue2>() == UnscopedEnum::eValue2);

TEST(reflection_enum, unscoped_enum)
{
    EXPECT_TRUE(ext::reflection::is_enum_value<UnscopedEnum>(UnscopedEnum::eValue1));
    EXPECT_TRUE(ext::reflection::is_enum_value<UnscopedEnum>(0));
    EXPECT_TRUE(ext::reflection::is_enum_value<UnscopedEnum>(UnscopedEnum::eValue2));
    EXPECT_TRUE(ext::reflection::is_enum_value<UnscopedEnum>(1));
    EXPECT_FALSE(ext::reflection::is_enum_value<UnscopedEnum>(-1));
}

TEST(reflection_enum, get_unscoped_enum_values)
{
    EXPECT_EQ((ext::reflection::get_enum_values<UnscopedEnum>()), (std::array{ UnscopedEnum::eValue1, UnscopedEnum::eValue2 }));
}

#endif