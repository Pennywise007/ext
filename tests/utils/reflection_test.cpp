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
};

static_assert(ext::reflection::brace_constructor_size<TestStruct> == 4, "Failed to determine fields count");

static_assert(HAS_FIELD(TestStruct, name), "Failed to find name field");
static_assert(!HAS_FIELD(TestStruct, unknown), "Failed to find unknown field");
