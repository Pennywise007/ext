#include "gtest/gtest.h"

#include <ext/utils/reflection.h>

struct DetailTestDefConstructor
{
    DetailTestDefConstructor() = default;
    DetailTestDefConstructor(const DetailTestDefConstructor &) = default;
    DetailTestDefConstructor(DetailTestDefConstructor &&) = default;
};
static_assert(ext::reflection::constructor_size<DetailTestDefConstructor> == 0, "Failed to determine default constructor size");

struct DetailTestOneArgumentConstructor
{
    DetailTestOneArgumentConstructor(int _val) : val(_val) {}
    DetailTestOneArgumentConstructor(const DetailTestOneArgumentConstructor &) = default;
    DetailTestOneArgumentConstructor(DetailTestOneArgumentConstructor &&) = default;
    int val;
};
static_assert(ext::reflection::constructor_size<DetailTestOneArgumentConstructor> == 1, "Failed to determine one argument constructor size");

struct DetailTestMultipleArgumentConstructor
{
    DetailTestMultipleArgumentConstructor(int _val, long, bool) : val(_val) {}
    DetailTestMultipleArgumentConstructor(const DetailTestMultipleArgumentConstructor &) = default;
    DetailTestMultipleArgumentConstructor(DetailTestMultipleArgumentConstructor &&) = default;
    int val;
};
static_assert(ext::reflection::constructor_size<DetailTestMultipleArgumentConstructor> == 3, "Failed to determine multiple argument constructor size");

struct TestStruct  {
    std::string name;
    std::string name1;
    std::string name2;
    std::string name3;

    void existingFunction(int) {}
};

static_assert(ext::reflection::brace_constructor_size<TestStruct> == 4, "Failed to determine fields count");

static_assert(HAS_FIELD(TestStruct, name), "Failed to find name field");
static_assert(!HAS_FIELD(TestStruct, unknown), "Failed to find unknown field");

static_assert(HAS_FUNCTION(TestStruct, existingFunction), "Failed to detect `existingFunction`");
static_assert(!HAS_FUNCTION(TestStruct, unknownFunction), "Found not existingFunction");

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

static_assert(ext::reflection::get_enum_name<TestEnum(0)>() == "TestEnum::eEnumValue1",
    "Failed to get enum value name");
static_assert(ext::reflection::get_enum_name<TestEnum::eEnumValue2>() == "TestEnum::eEnumValue2",
    "Failed to get enum value name");
static_assert(ext::reflection::get_enum_name<TestEnum(101)>() == "TestEnum::eEnumValue101",
    "Failed to get enum value name");
static_assert(ext::reflection::get_enum_name<TestClass<int>::InternalEnum::eInternalEnumValue1>() == "TestClass<int>::InternalEnum::eInternalEnumValue1",
    "Failed to get internal enum value name");
    
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
