#include "gtest/gtest.h"

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20

#include <set>
#include <map>

#include <fstream>

#include "samples_helper.h"

#include <ext/serialization/iserializable.h>

#include <ext/std/filesystem.h>

using namespace ext::serializable;
using namespace ext::serializer;

namespace {

constexpr auto kSampleTextDefault = "serialization/text_c++20_default.txt";
constexpr auto kSampleTextModified = "serialization/text_c++20_modification.txt";

} // namespace

struct ISerializableInterface : ISerializableCollection
{
    virtual void changeValue() = 0;
};

struct SerializableInterfaceImpl : ISerializableInterface
{
private:
    // ISerializable
    [[nodiscard]] const char* GetName() const noexcept override { return ext::type_name<SerializableInterfaceImpl>(); }
    // ISerializableCollection
        // Collection size
    [[nodiscard]] virtual size_t Size() const noexcept override { return 1; }
    // Get collection element by index
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        EXPECT_EQ(0, index);
        return details::get_as_serializable("flagTest", &const_cast<bool&>(flagTest));
    }

public:
    void changeValue() override final
    {
        flagTest = false;
    }

    bool flagTest = true;
};

struct BaseTypes
{
    long value = 0;
    std::string string;
    std::wstring wstring;

    std::map<int, long> valueMap;
    std::multimap<unsigned, long> valueMultimap;
    std::list<int> valueList;
    std::vector<float> valueVector;
    std::set<double> valueSet;
    std::multiset<int> valueMultiSet;

    std::filesystem::path path;

    std::optional<bool> optional;
    std::optional<std::pair<int, bool>> optionalPair;
    std::vector<std::optional<bool>> vectorOptional;
    std::pair<int, bool> pair;
    std::list<std::pair<int, bool>> listPair;

    void SetFieldValues()
    {
        value = 213;
        string = "text,\\s2\\\"\\d";
        wstring = L"wtext,\\s2\\\"\\d";

        valueMap = { {0, 2} };
        valueMultimap = { {0, 2} };
        valueList = { {2}, {5} };
        valueVector = { {2.4f}, {5.7f}, {NAN} };
        valueSet = { {2.4}, {5.7} };
        valueMultiSet = { {2}, {5} };

        path = "C:\\Test";

        optional = false;
        optionalPair = { 453, true };
        vectorOptional = { false, true };
        pair = { 7, false };
        listPair = { { 5, true }, { 88, false} };
    }
};

struct SerializableField : ISerializableField
{
    [[nodiscard]] const char* GetName() const noexcept override { return "Name"; }
    [[nodiscard]] SerializableValue SerializeValue() const override { return L"test"; }
    void DeserializeValue(const SerializableValue& value) override { EXPECT_STREQ(L"test", value.c_str()); }
};

struct SerializableTypes
{
    std::shared_ptr<ISerializableInterface> sharedSerializableInterface = std::make_unique<SerializableInterfaceImpl>();
    std::unique_ptr<ISerializableInterface> uniqueSerializableInterface = std::make_unique<SerializableInterfaceImpl>();
    std::optional<SerializableInterfaceImpl> optionalSerializableInterface;
    SerializableInterfaceImpl serializableInterface;

    SerializableField serializableObjectField;

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

TEST(serialization_c_plus_plus_20_test, text_default)
{
    SerializableTypes testStruct;
    testStruct.sharedSerializableInterface = nullptr;
    testStruct.uniqueSerializableInterface = nullptr;

    std::wstring defaultText;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(defaultText), testStruct));
    test::samples::expect_equal_to_sample(defaultText, kSampleTextDefault);
}

TEST(serialization_c_plus_plus_20_test, text_modified)
{
    SerializableTypes testStruct;
    testStruct.SetFieldValues();

    std::wstring textAfterModification;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(textAfterModification), testStruct));
    test::samples::expect_equal_to_sample(textAfterModification, kSampleTextModified);
}

TEST(deserialization_c_plus_plus_20_test, text_default)
{
    const auto expectedText = std::widen(test::samples::load_sample_file(kSampleTextDefault));

    SerializableTypes testStruct;
    ASSERT_TRUE(DeserializeObject(Factory::TextDeserializer(expectedText), testStruct));

    std::wstring textAfterDeserialization;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(textAfterDeserialization), testStruct));

    EXPECT_STREQ(expectedText.c_str(), textAfterDeserialization.c_str());
}

TEST(deserialization_c_plus_plus_20_test, text_modified)
{
    const auto expectedText = std::widen(test::samples::load_sample_file(kSampleTextModified));

    SerializableTypes testStruct;
    ASSERT_TRUE(DeserializeObject(Factory::TextDeserializer(expectedText), testStruct));

    std::wstring textAfterDeserialization;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(textAfterDeserialization), testStruct));

    EXPECT_STREQ(expectedText.c_str(), textAfterDeserialization.c_str());
}

#endif // C++20