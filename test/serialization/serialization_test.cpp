#include "gtest/gtest.h"

#include <set>
#include <map>

#include <fstream>

#include "samples_helper.h"

#define USE_PUGI_XML
#include <ext/serialization/iserializable.h>

#include <ext/std/filesystem.h>
#include <ext/std/string.h>

using namespace ext::serializable;
using namespace ext::serializable::serializer;

namespace {
constexpr char BaseTypeName[] = "BaseTypes";
constexpr char SerializableTypesName[] = "SerializableTypes";

const auto kTestXmlFilePath = std::filesystem::temp_directory_path() / L"test.xml";

constexpr auto kSampleTextDefault = "serialization/text_default.txt";
constexpr auto kSampleTextModified = "serialization/text_modification.txt";
constexpr auto kSampleXMLDefault = "serialization/xml_default.xml";
constexpr auto kSampleXMLModified = "serialization/xml_modification.xml";
} // namespace

struct CleanUp
{
    ~CleanUp() { std::filesystem::remove(kTestXmlFilePath); }
} __cleanUp;

struct ISerializableInterface : ISerializableCollection
{
    virtual void changeValue() = 0;
};

struct SerializableInterfaceImpl : SerializableObject<SerializableInterfaceImpl, nullptr, ISerializableInterface>
{
    void changeValue() override final
    {
        flagTest = false;
    }

    DECLARE_SERIALIZABLE_FIELD(bool, flagTest, true);
};

struct BaseTypes : SerializableObject<BaseTypes, BaseTypeName>
{
    DECLARE_SERIALIZABLE_FIELD(long, value, 0);
    DECLARE_SERIALIZABLE_FIELD(std::string, text);

    DECLARE_SERIALIZABLE_FIELD((std::map<int, long>), valueMap);
    DECLARE_SERIALIZABLE_FIELD((std::multimap<unsigned, long>), valueMultimap);
    DECLARE_SERIALIZABLE_FIELD(std::list<int>, valueList);
    DECLARE_SERIALIZABLE_FIELD(std::vector<float>, valueVector);
    DECLARE_SERIALIZABLE_FIELD(std::set<double>, valueSet);
    DECLARE_SERIALIZABLE_FIELD(std::multiset<int>, valueMultiSet);

    DECLARE_SERIALIZABLE_FIELD(std::filesystem::path, path);

    DECLARE_SERIALIZABLE_FIELD(std::optional<bool>, optional);
    DECLARE_SERIALIZABLE_FIELD((std::optional<std::pair<int, bool>>), optionalPair);
    DECLARE_SERIALIZABLE_FIELD(std::vector<std::optional<bool>>, vectorOptional);
    DECLARE_SERIALIZABLE_FIELD((std::pair<int, bool>), pair);
    DECLARE_SERIALIZABLE_FIELD((std::list<std::pair<int, bool>>), listPair);

    bool testField;
    BaseTypes()
        : testField(true)
    {
        REGISTER_SERIALIZABLE_FIELD(testField);
    }

    BaseTypes(bool /*SetValue*/)
        : BaseTypes()
    {
        SetFieldValues();
    }

    BaseTypes(long val, const std::list<int>& _list) : value(val), valueList(_list)
    {
        REGISTER_SERIALIZABLE_FIELD(value);
        REGISTER_SERIALIZABLE_FIELD(valueList);
        REGISTER_SERIALIZABLE_FIELD(testField);
    }

    virtual void SetFieldValues()
    {
        value = 213;
        text = "text,\\s2\\\"\\d";

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
    EXT_NODISCARD const char* GetName() const EXT_NOEXCEPT override { return "Name"; }
    EXT_NODISCARD SerializableValue SerializeValue() const override { return L"test"; }
    void DeserializeValue(const SerializableValue& value) override { EXT_EXPECT(value == L"test"); }
};

struct SerializableTypes : SerializableObject<SerializableTypes, SerializableTypesName>, virtual BaseTypes, virtual SerializableField
{
    typedef SerializableObject<SerializableTypes, SerializableTypesName> SerializableObjectType;

    REGISTER_SERIALIZABLE_BASE(BaseTypes, SerializableField);

    DECLARE_SERIALIZABLE_FIELD(std::shared_ptr<ISerializableInterface>, sharedSerializableInterface, std::make_shared<SerializableInterfaceImpl>());
    DECLARE_SERIALIZABLE_FIELD(std::unique_ptr<ISerializableInterface>, uniqueSerializableInterface, std::make_unique<SerializableInterfaceImpl>());
    DECLARE_SERIALIZABLE_FIELD(SerializableField, serializableObjectField);

    DECLARE_SERIALIZABLE_FIELD(BaseTypes, baseTypesField);
    DECLARE_SERIALIZABLE_FIELD(std::list<BaseTypes>, serializableList);

    DECLARE_SERIALIZABLE_FIELD(std::shared_ptr<BaseTypes>, serializableSharedPtr);
    DECLARE_SERIALIZABLE_FIELD(std::unique_ptr<BaseTypes>, serializableUniquePtr);
    DECLARE_SERIALIZABLE_FIELD(std::vector<std::shared_ptr<BaseTypes>>, serializableUniquePtrList);

    DECLARE_SERIALIZABLE_FIELD(std::optional<BaseTypes>, serializableOptional);

    DECLARE_SERIALIZABLE_FIELD((std::map<int, std::shared_ptr<BaseTypes>>), serializableStructsMap);
    DECLARE_SERIALIZABLE_FIELD((std::map<std::shared_ptr<BaseTypes>, int>), serializableStructsMapShared);
    DECLARE_SERIALIZABLE_FIELD((std::multimap<int, std::shared_ptr<BaseTypes>>), serializableStructsMultiMap);
    DECLARE_SERIALIZABLE_FIELD((std::multimap<std::shared_ptr<BaseTypes>, unsigned>), serializableStructsMultiMapShared);
    DECLARE_SERIALIZABLE_FIELD(std::set<std::shared_ptr<BaseTypes>>, serializableStructsSet);
    DECLARE_SERIALIZABLE_FIELD(std::multiset<std::shared_ptr<BaseTypes>>, serializableStructsMultiSet);

    DECLARE_SERIALIZABLE_FIELD(std::vector<std::string>, strings);
    DECLARE_SERIALIZABLE_FIELD(std::list<std::wstring>, wstrings);
    DECLARE_SERIALIZABLE_FIELD_N("My flag name", bool, oneFlag, true);

    std::map<int, long> flags2 = [this]()
    {
        SerializableObjectType::RegisterField("flags name", &SerializableTypes::flags2);
        return std::map<int, long>{};
    }();

    void SetFieldValues() override
    {
        BaseTypes::SetFieldValues();
        sharedSerializableInterface->changeValue();
        uniqueSerializableInterface->changeValue();

        baseTypesField.SetFieldValues();

        serializableList.emplace_back(true);
        serializableList.emplace_back(true);

        serializableSharedPtr = std::make_shared<BaseTypes>(true);
        serializableUniquePtr = std::make_unique<BaseTypes>(true);
        serializableUniquePtrList = { std::make_shared<BaseTypes>(true) };

        serializableOptional.emplace(BaseTypes(true));

        serializableStructsMap = { {1,std::make_shared<BaseTypes>(true) }};
        serializableStructsMapShared = { {std::make_shared<BaseTypes>(true), 534 }};
        serializableStructsMultiMap = { {1,std::make_shared<BaseTypes>(true) }};
        serializableStructsMultiMapShared = { {std::make_shared<BaseTypes>(true), 534 }};
        serializableStructsSet = { {std::make_shared<BaseTypes>(true) }};
        serializableStructsMultiSet = { {std::make_shared<BaseTypes>(true)}};

        strings = { "123", "Test1" };
        wstrings = { L"123", L"Test2" };

        oneFlag = false;
        flags2 = { { 0, 1 },  { 1, 15245 } };
    }
};

TEST(serialization_test, text_default)
{
    SerializableTypes testStruct;

    std::wstring defaultText;
    ASSERT_TRUE(Executor::SerializeObject(Factory::TextSerializer(defaultText), testStruct));
    test::samples::expect_equal_to_sample(defaultText, kSampleTextDefault);
}

TEST(serialization_test, text_modified)
{
    SerializableTypes testStruct;
    testStruct.SetFieldValues();

    std::wstring textAfterModification;
    ASSERT_TRUE(Executor::SerializeObject(Factory::TextSerializer(textAfterModification), testStruct));
    test::samples::expect_equal_to_sample(textAfterModification, kSampleTextModified);
}

TEST(deserialization_test, text_default)
{
    const auto defaultStructText = std::widen(test::samples::load_sample_file(kSampleTextDefault));

    SerializableTypes testStruct;
    ASSERT_TRUE(Executor::DeserializeObject(Factory::TextDeserializer(defaultStructText), testStruct));

    std::wstring textAfterDeserialization;
    ASSERT_TRUE(Executor::SerializeObject(Factory::TextSerializer(textAfterDeserialization), testStruct));

    EXPECT_STREQ(textAfterDeserialization.c_str(), defaultStructText.c_str());
}

TEST(deserialization_test, text_modified)
{
    const auto modifiedStructText = std::widen(test::samples::load_sample_file(kSampleTextModified));
    
    SerializableTypes testStruct;
    ASSERT_TRUE(Executor::DeserializeObject(Factory::TextDeserializer(modifiedStructText), testStruct));
   
    std::wstring textAfterDeserialization;
    ASSERT_TRUE(Executor::SerializeObject(Factory::TextSerializer(textAfterDeserialization), testStruct));

    EXPECT_STREQ(textAfterDeserialization.c_str(), modifiedStructText.c_str());
}

TEST(serialization_test, xml_default)
{
    SerializableTypes testStruct;

    ASSERT_TRUE(Executor::SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));
    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLDefault);
}

TEST(serialization_test, xml_modified)
{
    SerializableTypes testStruct;
    testStruct.SetFieldValues();

    ASSERT_TRUE(Executor::SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));
    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLModified);
}

TEST(deserialization_test, xml_default)
{
    const auto defaultStructFile = test::samples::sample_file_path(kSampleXMLDefault);

    SerializableTypes testStruct;
    ASSERT_TRUE(Executor::DeserializeObject(Factory::XMLDeserializer(defaultStructFile), testStruct));
    ASSERT_TRUE(Executor::SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));

    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLDefault);
}

TEST(deserialization_test, xml_modified)
{
    const auto modifiedStructText = test::samples::sample_file_path(kSampleXMLModified);
    
    SerializableTypes testStruct;
    ASSERT_TRUE(Executor::DeserializeObject(Factory::XMLDeserializer(modifiedStructText), testStruct));
    ASSERT_TRUE(Executor::SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));

    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLModified);
}

struct Settings : ext::serializable::SerializableObject<Settings>
{
    struct User : ext::serializable::SerializableObject<User>
    {
        DECLARE_SERIALIZABLE_FIELD(std::int64_t, id);
        DECLARE_SERIALIZABLE_FIELD(std::string, firstName);
        DECLARE_SERIALIZABLE_FIELD(std::string, userName);

        bool operator==(const User& other) const
        {
            return id == other.id && firstName == other.firstName && userName == other.userName;
        }
    };
    
    DECLARE_SERIALIZABLE_FIELD(std::wstring, token);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, password);
    DECLARE_SERIALIZABLE_FIELD(std::list<User>, registeredUsers);
};

TEST(serialization_test, serialization_custom)
{
    using namespace ext::serializable::serializer;

    std::wstring text;

    Settings initialSettings;
    initialSettings.token = L"123";
    initialSettings.password = L"password";
    initialSettings.registeredUsers.resize(1);
    initialSettings.registeredUsers.back().id = 123;

    ASSERT_TRUE(Executor::SerializeObject(Factory::TextSerializer(text), reinterpret_cast<const ISerializable*>(&initialSettings)));

    Settings result; 
    ASSERT_TRUE(Executor::DeserializeObject(Factory::TextDeserializer(text), reinterpret_cast<ISerializable*>(&result)));
    EXPECT_EQ(initialSettings.token, result.token);
    EXPECT_EQ(initialSettings.password, result.password);
    EXPECT_EQ(initialSettings.registeredUsers, result.registeredUsers);
}

