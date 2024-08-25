#include "gtest/gtest.h"

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20

#include <chrono>
#include <ctime>
#include <filesystem>
#include <map>
#include <set>

#include "samples_helper.h"

#include <ext/serialization/serializer.h>

using namespace ext::serializable;
using namespace ext::serializer;

namespace {

constexpr auto kSampleTextDefault = "serialization/json_c++20_default.txt";
constexpr auto kSampleTextModified = "serialization/json_c++20_modification.txt";

struct ISerializableInterface : ISerializableArray
{
    virtual void changeValue() = 0;
};

struct SerializableInterfaceImpl : ISerializableInterface
{
private:
// ISerializableArray
    // Collection size
    [[nodiscard]] virtual size_t Size() const noexcept override { return 1; }
    // Get collection element by index
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get(const size_t& index) override
    {
        EXPECT_EQ(0, index);
        return details::get_as_serializable(flagTest);
    }

public:
    void changeValue() override final
    {
        flagTest = false;
    }

    bool flagTest = true;
};

struct CustomTimeFormat : ISerializableValue {

    static inline constexpr auto kTimeFormat = L"%Y/%m/%d %H:%M:%S";

    CustomTimeFormat() = default;
    CustomTimeFormat(int sec, int min, int hour, int day, int month, int year)
    {
        std::tm timeinfo = {};
        timeinfo.tm_sec = sec;
        timeinfo.tm_min = min;
        timeinfo.tm_hour = hour;
        timeinfo.tm_mday = day;
        timeinfo.tm_mon = month - 1;
        timeinfo.tm_year = year - 1900;
#ifdef _WIN32
        std::time_t time_t_point = _mkgmtime(&timeinfo);
#else
        std::time_t time_t_point = timegm(&timeinfo);
#endif
        m_time = std::chrono::system_clock::from_time_t(time_t_point);
    }

// ISerializableValue
    [[nodiscard]] SerializableValue SerializeValue() const override
    {
        const std::time_t t = std::chrono::system_clock::to_time_t(m_time);
        std::tm gmt;
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        EXT_EXPECT(gmtime_s(&gmt, &t) == 0);
#else
        EXT_EXPECT(gmtime_r(&t, &gmt) != 0);
#endif
        return (std::wstringstream() << std::put_time(&gmt, kTimeFormat)).str();
    }

    void DeserializeValue(const SerializableValue& value) override
    {
        std::tm tm = {};
        std::wistringstream(value) >> std::get_time(&tm, kTimeFormat);
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        std::time_t time_t_point = _mkgmtime(&tm);
#else
        std::time_t time_t_point = timegm(&tm);
#endif
        m_time = std::chrono::system_clock::from_time_t(time_t_point);
    }

private:
    std::chrono::system_clock::time_point m_time;
};

struct SerializableFieldImpl : ISerializableField {
    // ISerializableField
    [[nodiscard]] virtual SerializableValue GetName() const override { return L"Serializable field name"; }
    [[nodiscard]] virtual std::shared_ptr<ISerializable> GetField() override { return ext::serializable::details::get_as_serializable(val); }
    void ChangeValue() { val = 303; }
    int val = 10;
};

struct BaseTypes
{
    long value = 0;
    std::string string;
    std::wstring wstring;

    std::list<int> valueList;
    std::vector<float> valueVector;
    std::set<double> valueSet;
    std::multiset<int> valueMultiSet;
    std::map<int, long> valueMap;
    std::multimap<unsigned, long> valueMultimap;

    std::filesystem::path path;

    std::chrono::system_clock::time_point system_time;

    std::chrono::nanoseconds duration_nanoseconds = {};
    std::chrono::microseconds duration_microseconds = {};
    std::chrono::milliseconds duration_milliseconds = {};
    std::chrono::seconds duration_seconds = {};
    std::chrono::minutes duration_minutes = {};
    std::chrono::hours duration_hours = {};
    std::chrono::days duration_days = {};
    std::chrono::weeks duration_weeks = {};
    std::chrono::months duration_months = {};
    std::chrono::years duration_years = {};

    std::optional<bool> optional;
    std::optional<std::pair<int, bool>> optionalPair;
    std::vector<std::optional<bool>> vectorOptional;
    std::pair<int, bool> pair;
    std::list<std::pair<int, bool>> listPair;

    void SetFieldValues()
    {
        value = 213;
        string = R"(text,\s2\"}]{[\val\)";
        wstring = LR"(wtext,\s2\"}]{[\val\)";

        valueMap = { {0, 2} };
        valueMultimap = { {0, 2} };
        valueList = { {2}, {5} };
        valueVector = { {2.4f}, {5.7f}, {NAN} };
        valueSet = { {2.4}, {5.7} };
        valueMultiSet = { {2}, {5} };

        path = "C:\\Test";

        std::tm timeinfo = {};
        timeinfo.tm_sec = 7;
        timeinfo.tm_min = 6;
        timeinfo.tm_hour = 5;
        timeinfo.tm_mday = 4;
        timeinfo.tm_mon = 3 - 1;
        timeinfo.tm_year = 2007 - 1900;
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        std::time_t time_t_point = _mkgmtime(&timeinfo);
#else
        std::time_t time_t_point = timegm(&timeinfo);
#endif
        system_time = std::chrono::system_clock::from_time_t(time_t_point);

        duration_nanoseconds = std::chrono::nanoseconds(12345678);
        duration_microseconds = std::chrono::microseconds(2);
        duration_milliseconds = std::chrono::milliseconds(3);
        duration_seconds = std::chrono::seconds(4);
        duration_minutes = std::chrono::minutes(5);
        duration_hours = std::chrono::hours(6);
        duration_days = std::chrono::days(7);
        duration_weeks = std::chrono::weeks(8);
        duration_months = std::chrono::months(9);
        duration_years = std::chrono::years(10);

        optional = false;
        optionalPair = { 453, true };
        vectorOptional = { false, true };
        pair = { 7, false };
        listPair = { { 5, true }, { 88, false} };
    }
};

struct SerializableCustomValue : ISerializableValue
{
    [[nodiscard]] SerializableValue SerializeValue() const override { return L"test"; }
    void DeserializeValue(const SerializableValue& value) override { EXPECT_STREQ(L"test", value.c_str()); }
};

struct SerializableTypes
{
    std::shared_ptr<ISerializableInterface> sharedSerializableInterface = std::make_unique<SerializableInterfaceImpl>();
    std::unique_ptr<ISerializableInterface> uniqueSerializableInterface = std::make_unique<SerializableInterfaceImpl>();
    std::optional<SerializableInterfaceImpl> optionalSerializableInterface;
    SerializableInterfaceImpl serializableInterface;

    SerializableCustomValue serializableValueField;
    CustomTimeFormat customTimeValue;

    BaseTypes baseTypesField;
    std::list<BaseTypes> serializableList;

    std::shared_ptr<BaseTypes> serializableSharedPtr;
    std::unique_ptr<BaseTypes> serializableUniquePtr;
    std::vector<std::shared_ptr<BaseTypes>> serializableUniquePtrList;

    std::optional<BaseTypes> serializableOptional;

    std::map<int, std::shared_ptr<BaseTypes>> serializableStructsMap;
    std::map<std::shared_ptr<BaseTypes>, int> serializableStructsMapShared;
    std::multimap<int, std::shared_ptr<BaseTypes>> serializableStructsMultiMap;
    std::multimap<std::shared_ptr<BaseTypes>, unsigned> serializableStructsMultiMapShared;
    std::set<std::shared_ptr<BaseTypes>> serializableStructsSet;
    std::multiset<std::shared_ptr<BaseTypes>> serializableStructsMultiSet;

    std::vector<std::string> strings;
    std::list<std::wstring> wstrings;

    void SetFieldValues()
    {
        sharedSerializableInterface->changeValue();
        uniqueSerializableInterface->changeValue();
        serializableInterface.changeValue();
        optionalSerializableInterface.emplace(serializableInterface);

        customTimeValue = CustomTimeFormat(1, 2, 3, 4, 5, 2019);
        baseTypesField.SetFieldValues();

        BaseTypes baseWithValues;
        baseWithValues.SetFieldValues();

        serializableList.emplace_back(baseWithValues);
        serializableList.emplace_back(baseWithValues);


        serializableSharedPtr = std::make_shared<BaseTypes>(baseWithValues);
        serializableUniquePtr = std::make_unique<BaseTypes>(baseWithValues);
        serializableUniquePtrList = { std::make_shared<BaseTypes>(baseWithValues) };

        serializableOptional.emplace(baseWithValues);

        serializableStructsMap = { {1,std::make_shared<BaseTypes>(baseWithValues) } };
        serializableStructsMapShared = { {std::make_shared<BaseTypes>(baseWithValues), 534 } };
        serializableStructsMultiMap = { {1,std::make_shared<BaseTypes>(baseWithValues) } };
        serializableStructsMultiMapShared = { {std::make_shared<BaseTypes>(baseWithValues), 534 } };
        serializableStructsSet = { {std::make_shared<BaseTypes>(baseWithValues) } };
        serializableStructsMultiSet = { {std::make_shared<BaseTypes>(baseWithValues)} };

        strings = { "123", "Test1" };
        wstrings = { L"123", L"Test2" };
    }
};

} // namespace

TEST(serialization_c_plus_plus_20_test, text_default)
{
    SerializableTypes testStruct;
    testStruct.sharedSerializableInterface = nullptr;
    testStruct.uniqueSerializableInterface = nullptr;

    std::wstring defaultJson;
    ASSERT_NO_THROW(SerializeToJson(testStruct, defaultJson));
    test::samples::expect_equal_to_sample(defaultJson, kSampleTextDefault);
}

TEST(serialization_c_plus_plus_20_test, text_modified)
{
    SerializableTypes testStruct;
    testStruct.SetFieldValues();

    std::wstring jsonAfterModification;
    ASSERT_NO_THROW(SerializeToJson(testStruct, jsonAfterModification));
    test::samples::expect_equal_to_sample(jsonAfterModification, kSampleTextModified);
}

TEST(deserialization_c_plus_plus_20_test, text_default)
{
    const auto expectedText = std::widen(test::samples::load_sample_file(kSampleTextDefault));

    SerializableTypes testStruct;
    ASSERT_NO_THROW(DeserializeFromJson(testStruct, expectedText));

    std::wstring textAfterDeserialization;
    ASSERT_NO_THROW(SerializeToJson(testStruct, textAfterDeserialization));

    EXPECT_STREQ(expectedText.c_str(), textAfterDeserialization.c_str());
}

TEST(deserialization_c_plus_plus_20_test, text_modified)
{
    const auto expectedText = std::widen(test::samples::load_sample_file(kSampleTextModified));

    SerializableTypes testStruct;
    ASSERT_NO_THROW(DeserializeFromJson(testStruct, expectedText));

    std::wstring textAfterDeserialization;
    ASSERT_NO_THROW(SerializeToJson(testStruct, textAfterDeserialization));

    EXPECT_STREQ(expectedText.c_str(), textAfterDeserialization.c_str());
}

#endif // C++20