#include "gtest/gtest.h"

#include <set>
#include <map>

#include <fstream>

#include "samples_helper.h"

#define USE_PUGI_XML
#include <ext/serialization/iserializable.h>

#include <ext/std/filesystem.h>
#include <ext/std/string.h>

#include <ext/types/uuid.h>

using namespace ext::serializable;
using namespace ext::serializer;

namespace {
constexpr char BaseTypeName[] = "BaseTypes";
constexpr char SerializableTypesName[] = "SerializableTypes";

const auto kTestXmlFilePath = std::filesystem::temp_directory_path() / 
    std::string_sprintf("test_%s.xml", ext::uuid().ToString().c_str());

constexpr auto kSampleTextDefault = "serialization/text_default.txt";
constexpr auto kSampleTextModified = "serialization/text_modification.txt";
constexpr auto kSampleXMLDefault = "serialization/xml_default.xml";
constexpr auto kSampleXMLModified = "serialization/xml_modification.xml";

struct CleanUp
{
    ~CleanUp() { std::filesystem::remove(kTestXmlFilePath); }
} __cleanUp;

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
    [[nodiscard]] virtual size_t Size() const noexcept override { return 1;}
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
    REGISTER_SERIALIZABLE_OBJECT_N(BaseTypeName);

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

    BaseTypes(bool)
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
    [[nodiscard]] const char* GetName() const noexcept override { return "Name"; }
    [[nodiscard]] SerializableValue SerializeValue() const override { return L"test"; }
    void DeserializeValue(const SerializableValue& value) override { EXPECT_STREQ(L"test", value.c_str()); }
};

struct SerializableTypes : BaseTypes, SerializableInterfaceImpl
{
    REGISTER_SERIALIZABLE_OBJECT_N(SerializableTypesName, BaseTypes, SerializableInterfaceImpl);

    DECLARE_SERIALIZABLE_FIELD(std::shared_ptr<ISerializableInterface>, sharedSerializableInterface, std::make_unique<SerializableInterfaceImpl>());
    DECLARE_SERIALIZABLE_FIELD(std::unique_ptr<ISerializableInterface>, uniqueSerializableInterface, std::make_unique<SerializableInterfaceImpl>());
    DECLARE_SERIALIZABLE_FIELD(std::optional<SerializableInterfaceImpl>, optionalSerializableInterface);
    DECLARE_SERIALIZABLE_FIELD(SerializableInterfaceImpl, serializableInterface);

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
    DECLARE_SERIALIZABLE_FIELD_N(bool, oneFlag, "My flag name", true);

    std::map<int, long> flags2 = [this]()
    {
        REGISTER_SERIALIZABLE_FIELD_N("flags name", flags2);
        return std::map<int, long>{};
    }();

    void SetFieldValues() override
    {
        BaseTypes::SetFieldValues();
        SerializableInterfaceImpl::changeValue();

        sharedSerializableInterface->changeValue();
        uniqueSerializableInterface->changeValue();
        serializableInterface.changeValue();
        optionalSerializableInterface.emplace(serializableInterface);

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
    testStruct.sharedSerializableInterface = nullptr;
    testStruct.uniqueSerializableInterface = nullptr;

    std::wstring defaultText;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(defaultText), testStruct));
    test::samples::expect_equal_to_sample(defaultText, kSampleTextDefault);
}

TEST(serialization_test, text_modified)
{
    SerializableTypes testStruct;
    testStruct.SetFieldValues();

    std::wstring textAfterModification;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(textAfterModification), testStruct));
    test::samples::expect_equal_to_sample(textAfterModification, kSampleTextModified);
}

TEST(deserialization_test, text_default)
{
    const auto expectedText = std::widen(test::samples::load_sample_file(kSampleTextDefault));

    SerializableTypes testStruct;
    ASSERT_TRUE(DeserializeObject(Factory::TextDeserializer(expectedText), testStruct));

    std::wstring textAfterDeserialization;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(textAfterDeserialization), testStruct));

    EXPECT_STREQ(expectedText.c_str(), textAfterDeserialization.c_str());
}

TEST(deserialization_test, text_modified)
{
    const auto expectedText = std::widen(test::samples::load_sample_file(kSampleTextModified));
    
    SerializableTypes testStruct;
    ASSERT_TRUE(DeserializeObject(Factory::TextDeserializer(expectedText), testStruct));
   
    std::wstring textAfterDeserialization;
    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(textAfterDeserialization), testStruct));

    EXPECT_STREQ(expectedText.c_str(), textAfterDeserialization.c_str());
}

TEST(serialization_test, xml_default)
{
    SerializableTypes testStruct;
    testStruct.sharedSerializableInterface = nullptr;
    testStruct.uniqueSerializableInterface = nullptr;

    ASSERT_TRUE(SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));
    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLDefault);
}

TEST(serialization_test, xml_modified)
{
    SerializableTypes testStruct;
    testStruct.SetFieldValues();

    ASSERT_TRUE(SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));
    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLModified);
}

TEST(deserialization_test, xml_default)
{
    const auto defaultStructFile = test::samples::sample_file_path(kSampleXMLDefault);

    SerializableTypes testStruct;
    ASSERT_TRUE(DeserializeObject(Factory::XMLDeserializer(defaultStructFile), testStruct));
    ASSERT_TRUE(SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));

    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLDefault);
}

TEST(deserialization_test, xml_modified)
{
    const auto modifiedStructText = test::samples::sample_file_path(kSampleXMLModified);
    
    SerializableTypes testStruct;
    ASSERT_TRUE(DeserializeObject(Factory::XMLDeserializer(modifiedStructText), testStruct));
    ASSERT_TRUE(SerializeObject(Factory::XMLSerializer(kTestXmlFilePath), testStruct));

    test::samples::expect_equal_to_sample(kTestXmlFilePath, kSampleXMLModified);
}

#define DECLARE_SERIALIZABLE_CALLBACKS()                                                                    \
    void OnSerializationStart() { serializationStarted = true; }                                            \
    void OnSerializationEnd() { EXPECT_TRUE(serializationStarted); serializationEnded = true; };            \
    void OnDeserializationStart(std::shared_ptr<SerializableNode>&)                    \
    { deserializationStarted = true; }                                                                      \
    void OnDeserializationEnd() { EXPECT_TRUE(deserializationStarted); deserializationEnded = true; };      \
    bool serializationStarted = false;                                                                      \
    bool serializationEnded = false;                                                                        \
    bool deserializationStarted = false;                                                                    \
    bool deserializationEnded = false

struct Settings
{
    struct User
    {
        REGISTER_SERIALIZABLE_OBJECT();
        DECLARE_SERIALIZABLE_FIELD(std::int64_t, id);
        DECLARE_SERIALIZABLE_FIELD(std::string, firstName);
        DECLARE_SERIALIZABLE_FIELD(std::string, userName);

        bool operator==(const User& other) const
        {
            return id == other.id && firstName == other.firstName && userName == other.userName;
        }
        DECLARE_SERIALIZABLE_CALLBACKS();
    };

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(std::wstring, token);
    DECLARE_SERIALIZABLE_FIELD(std::wstring, password);
    DECLARE_SERIALIZABLE_FIELD(std::list<User>, registeredUsers);

    DECLARE_SERIALIZABLE_CALLBACKS();
};

TEST(serialization_test, serialization_custom)
{
    std::wstring text;

    Settings initialSettings;
    initialSettings.token = L"123";
    initialSettings.password = L"password";
    initialSettings.registeredUsers.resize(1);
    initialSettings.registeredUsers.back().id = 123;

    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(text), initialSettings));

    EXPECT_TRUE(initialSettings.serializationStarted);
    EXPECT_TRUE(initialSettings.serializationEnded);
    EXPECT_FALSE(initialSettings.deserializationStarted);
    EXPECT_FALSE(initialSettings.deserializationEnded);
    for (const auto& item : initialSettings.registeredUsers)
    {
        EXPECT_TRUE(item.serializationStarted);
        EXPECT_TRUE(item.serializationEnded);
        EXPECT_FALSE(item.deserializationStarted);
        EXPECT_FALSE(item.deserializationEnded);
    }

    Settings result; 
    ASSERT_TRUE(DeserializeObject(Factory::TextDeserializer(text), result));
    EXPECT_EQ(initialSettings.token, result.token);
    EXPECT_EQ(initialSettings.password, result.password);
    EXPECT_EQ(initialSettings.registeredUsers, result.registeredUsers);
    
    EXPECT_FALSE(result.serializationStarted);
    EXPECT_FALSE(result.serializationEnded);
    EXPECT_TRUE(result.deserializationStarted);
    EXPECT_TRUE(result.deserializationEnded);
    for (const auto& item : result.registeredUsers)
    {
        EXPECT_FALSE(item.serializationStarted);
        EXPECT_FALSE(item.serializationEnded);
        EXPECT_TRUE(item.deserializationStarted);
        EXPECT_TRUE(item.deserializationEnded);
    }
}

class GlobalObject : SerializableInterfaceImpl
{
    class PrivateBaseObject
    {
    protected:
        REGISTER_SERIALIZABLE_OBJECT();
        DECLARE_SERIALIZABLE_FIELD(int, baseObjectVal, 0);
    };
    class PrivateObject : PrivateBaseObject
    {
        REGISTER_SERIALIZABLE_OBJECT(PrivateBaseObject);
        DECLARE_SERIALIZABLE_FIELD(int, privateObjectVal, 0);

    public:
        bool operator==(const PrivateObject& other) const { return privateObjectVal == other.privateObjectVal && baseObjectVal == other.baseObjectVal; }
        void Change() { privateObjectVal = -100; baseObjectVal = -1; }
    };

    REGISTER_SERIALIZABLE_OBJECT(SerializableInterfaceImpl);
    DECLARE_SERIALIZABLE_FIELD(PrivateObject, collection);

public:
    bool operator==(const GlobalObject& other) const { return collection == other.collection && flagTest == other.flagTest; }
    void Change()
    {
        collection.Change();
        SerializableInterfaceImpl::changeValue();
    }
};

TEST(serialization_test, collection_as_a_field_with_private_inheritance)
{
    GlobalObject object;

    std::wstring data;
    SerializeObject(Factory::TextSerializer(data), object);
    EXPECT_STREQ(L"\"GlobalObject\": {\n    \"SerializableInterfaceImpl\": {\n        \"flagTest\": true\n    },\n    \"collection\": {\n        \"baseObjectVal\": 0,\n        \"privateObjectVal\": 0\n    }\n}\n", data.c_str());

    object.Change();
    SerializeObject(Factory::TextSerializer(data), object);
    EXPECT_STREQ(L"\"GlobalObject\": {\n    \"SerializableInterfaceImpl\": {\n        \"flagTest\": false\n    },\n    \"collection\": {\n        \"baseObjectVal\": -1,\n        \"privateObjectVal\": -100\n    }\n}\n", data.c_str());

    GlobalObject deserializationResult;
    DeserializeObject(Factory::TextDeserializer(data), deserializationResult);
    EXPECT_EQ(object, deserializationResult);
}

struct MappedSerializableType
{
    MappedSerializableType() = default;
    MappedSerializableType(int val) { value = val; }
    int operator<(const MappedSerializableType& other) const { return std::to_wstring(value) < std::to_wstring(other.value); }
    bool operator==(const MappedSerializableType& other) const { return value == other.value; }

    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(long, value, 0);
};

template <>
struct std::hash<MappedSerializableType>
{
    std::size_t operator()(const MappedSerializableType& k) const
    {
        return std::hash<string>()(std::to_string(k.value));
    }
};

struct MappedSerializableTypeChecker
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(std::set<MappedSerializableType>, set, { 1, 2, 3});
    DECLARE_SERIALIZABLE_FIELD(std::unordered_set<MappedSerializableType>, unordered_set, { 4, 5, 6});
    DECLARE_SERIALIZABLE_FIELD(std::multiset<MappedSerializableType>, multiset, { 7, 8, 9});
    DECLARE_SERIALIZABLE_FIELD(std::set<std::shared_ptr<MappedSerializableType>>, set_shared_ptr, { std::make_shared<MappedSerializableType>(10), std::make_shared<MappedSerializableType>(11), std::make_shared<MappedSerializableType>(12)});
    DECLARE_SERIALIZABLE_FIELD((std::map<MappedSerializableType, int>), map, { {1, 4}, {2, 5}, {3, 6}});
    DECLARE_SERIALIZABLE_FIELD((std::unordered_map<MappedSerializableType, int>), unordered_map, { {4, 7}, {5, 8}, {6, 9}});
    DECLARE_SERIALIZABLE_FIELD((std::multimap<MappedSerializableType, int>), multimap, { {7, 10}, {8, 11}, {9, 12}});
    DECLARE_SERIALIZABLE_FIELD((std::map<std::shared_ptr<MappedSerializableType>, int>), map_shared_ptr, { {std::make_shared<MappedSerializableType>(10), 13}, {std::make_shared<MappedSerializableType>(11), 14}, {std::make_shared<MappedSerializableType>(12), 15}});
};

TEST(serialization_deserialization_test, mapped_serializable_checker)
{
    MappedSerializableTypeChecker object;
    std::wstring text;

    ASSERT_TRUE(SerializeObject(Factory::TextSerializer(text), object));

    MappedSerializableTypeChecker result;
    result.set.clear();
    result.unordered_set.clear();
    result.multiset.clear();
    result.set_shared_ptr.clear();
    result.map.clear();
    result.unordered_map.clear();
    result.multimap.clear();
    result.map_shared_ptr.clear();
    ASSERT_TRUE(DeserializeObject(Factory::TextDeserializer(text), result));
    EXPECT_EQ(object.set, result.set);
    EXPECT_EQ(object.unordered_set, result.unordered_set);
    EXPECT_EQ(object.multiset, result.multiset);
    EXPECT_EQ(object.map, result.map);
    EXPECT_EQ(object.unordered_map, result.unordered_map);
    EXPECT_EQ(object.multimap, result.multimap);

    ASSERT_EQ(object.set_shared_ptr.size(), result.set_shared_ptr.size());
    {
        std::set<MappedSerializableType> expected;
        std::set<MappedSerializableType> resultValues;

        auto itL = object.set_shared_ptr.begin(), itR = result.set_shared_ptr.begin();
        for (auto endL = object.set_shared_ptr.end(); itL != endL; ++itL, ++itR)
        {
            expected.emplace(*itR->get());
            resultValues.emplace(*itL->get());
        }
        EXPECT_EQ(expected, resultValues);
    }

    EXPECT_EQ(object.map_shared_ptr.size(), result.map_shared_ptr.size());
    {
        std::map<MappedSerializableType, int> expected;
        std::map<MappedSerializableType, int> resultValues;

        auto itL = object.map_shared_ptr.begin(), itR = result.map_shared_ptr.begin();
        for (auto endL = object.map_shared_ptr.end(); itL != endL; ++itL, ++itR)
        {
            expected.emplace(*itR->first, itR->second);
            resultValues.emplace(*itL->first, itL->second);
        }
        EXPECT_EQ(expected, resultValues);
    }
}
