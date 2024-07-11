#include "gtest/gtest.h"

#include <ext/reflection/object.h>

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

struct TestStruct {
    int intField;
    bool booleanField;
    std::string_view charArrayField;

    void existingFunction(int) {}
};

static_assert(ext::reflection::fields_count<TestStruct> == 3, "Failed to determine fields count");

static_assert(HAS_FIELD(TestStruct, booleanField), "Failed to find booleanField field");
static_assert(!HAS_FIELD(TestStruct, unknown), "Failed to find unknown field");

static_assert(HAS_FUNCTION(TestStruct, existingFunction), "Failed to detect `existingFunction`");
static_assert(!HAS_FUNCTION(TestStruct, unknownFunction), "Found not existingFunction");

constexpr auto kGlobalObj = TestStruct{ 100, true, "test"};

static_assert(std::is_same_v<std::tuple<const int&, const bool&, const std::string_view&>, decltype(ext::reflection::get_object_fields(kGlobalObj))>,
              "Failed to get_object_fields");
static_assert(std::get<0>(ext::reflection::get_object_fields(kGlobalObj)) == 100,
              "Failed to get_object_fields");
static_assert(std::get<1>(ext::reflection::get_object_fields(kGlobalObj)) == true,
              "Failed to get_object_fields");
static_assert(std::get<2>(ext::reflection::get_object_fields(kGlobalObj)) == "test",
              "Failed to get_object_fields");

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20

static_assert(ext::reflection::field_name<decltype(kGlobalObj), 0> == "intField",
              "Failed to get_field_name");
static_assert(ext::reflection::field_name<TestStruct, 1> == "booleanField",
              "Failed to get_field_name");
static_assert(ext::reflection::field_name<TestStruct, 2> == "charArrayField",
              "Failed to get_field_name");

#endif // C++20

TEST(object_test, fields_reflection)
{
    auto obj = TestStruct{ 100, true, "wow"};
    std::get<0>(ext::reflection::get_object_fields(obj)) = -2;
    std::get<1>(ext::reflection::get_object_fields(obj)) = false;
    EXPECT_STREQ("wow", std::get<2>(ext::reflection::get_object_fields(obj)).data());

    EXPECT_EQ(-2, obj.intField);
    EXPECT_FALSE(obj.booleanField);
}