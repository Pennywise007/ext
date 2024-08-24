#pragma once

/*
    Serialization of objects

    Implementation of base classes to simplify serialization / deserialization of data

    Usage example:

#if C++20 // we use reflection to get fields info, no macro needed, to use base classes you need to use REGISTER_SERIALIZABLE_OBJECT
struct Settings
{
    struct User
    {
        std::int64_t id;
        std::string firstName;
        std::string userName;
    };
    
    std::wstring password;
    std::list<User> registeredUsers;
};

Settings settings;

std::wstring json;
try {
    SerializeToJson(settings, json);
}
catch (...) {
    ext::ManageException(EXT_TRACE_FUNCTION);
}
...
try {
    DeserializeFromJson(settings, json);
}
catch (...) {
    ext::ManageException(EXT_TRACE_FUNCTION);
}

#endif // C++20

struct InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(long, value);
    DECLARE_SERIALIZABLE_FIELD(std::list<int>, valueList);
};

struct CustomValue : ISerializableValue {
// ISerializableValue
    [[nodiscard]] SerializableValue SerializeValue() const override { return std::to_wstring(val); }
    [[nodiscard]] void DeserializeValue(const SerializableValue& value) override { val = std::wtoi(value); }
    int val = 10;
};

struct TestStruct : InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT(InternalStruct);

    DECLARE_SERIALIZABLE_FIELD(long, valueLong, 2);
    DECLARE_SERIALIZABLE_FIELD(int, valueInt);
    DECLARE_SERIALIZABLE_FIELD(std::vector<bool>, boolVector, { true, false });

    DECLARE_SERIALIZABLE_FIELD(CustomValue, value);
    DECLARE_SERIALIZABLE_FIELD(InternalStruct, internalStruct);

    // Instead of using macroses - use REGISTER_SERIALIZABLE_FIELD in constructor
    std::list<int> m_listOfParams;

    MyTestStruct()
    {
        REGISTER_SERIALIZABLE_FIELD(m_listOfParams); // or use DECLARE_SERIALIZABLE_FIELD macro

        if (!std::filesystem::exists(kFilePath))
            return;
        try
        {
            std::wifstream file(kFilePath);
            EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
            EXT_DEFER(file.close());

            const std::wstring json{ std::istreambuf_iterator<wchar_t>(file),
                                        std::istreambuf_iterator<wchar_t>() };

            ext::serializer::DeserializeFromJson(settings, json);
        }
        catch (const std::exception&)
        {
            ext::ManageException("Failed to load settings");
        }
    }

    ~MyTestStruct()
    {
        try
        {
            std::wstring json;
            ext::serializer::SerializeToJson(*this, json);

            std::wofstream file(kFullFileName);
            EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
            EXT_DEFER(file.close());
            file << json;
        }
        catch (const std::exception&)
        {
            ext::ManageException("Failed to save settings");
        }
    }
};

You can also declare this functions in your REGISTER_SERIALIZABLE_OBJECT object to get notified when (de)serialization was called:
    // Called before object serialization
    void OnSerializationStart() {}
    // Called after object serialization
    void OnSerializationEnd() {};

    // Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allow to add upgrade for old stored settings
    // Also used to allocate object elements
    void OnDeserializationStart(SerializableNode& serializableTree) {}
    // Called after object deserialization
    void OnDeserializationEnd() {};
*/

#include <algorithm>
#include <chrono>
#include <list>
#include <type_traits>
#include <map>
#include <memory>
#include <optional>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/singleton.h>

#include <ext/reflection/object.h>

#include <ext/serialization/serializable_value.h>

#include <ext/std/type_traits.h>

#include <ext/utils/call_once.h>

// Register serializable object and it's based classes
#define REGISTER_SERIALIZABLE_OBJECT(/*Serializable base classes list*/...)                     \
    /* Make helper function our class friend to allow serialization of the private fields */    \
    template <class __SerializableClass__>                                                      \
    friend struct ::ext::serializable::has___serializable_object_registration_field;            \
    /* To avoid problems with private fields/inheritance we will use SerializableObjectDescriptor::ConvertToType */  \
    template <class __SerializableClass__>                                                      \
    friend class ext::serializable::SerializableObjectDescriptor;                               \
    /* Special flag to mark that this struct can be serialized, see is_registered_serializable_object_v */  \
    bool __serializable_object_registration = [this]() -> bool                                  \
        {                                                                                       \
            CALL_ONCE({                                                                         \
                using CurrentType = std::remove_reference_t<decltype(*this)>;                   \
                auto& __info = ext::get_singleton<ext::serializable::SerializableObjectDescriptor<CurrentType>>(); \
                __info.RegisterSerializableBaseClasses<__VA_ARGS__>();                          \
            })                                                                                  \
            return true;                                                                        \
        }()

// Declaring a serializable field and registering it in global serializable objects register
// Use case DECLARE_SERIALIZABLE_FIELD(long, m_propName); => long m_propName = long(); REGISTER_SERIALIZABLE_FIELD(m_propName);
// Use case DECLARE_SERIALIZABLE_FIELD(long, m_propName, 105); => long m_propName = long(105); REGISTER_SERIALIZABLE_FIELD(m_propName);
// !! You mustn't use a constructor for this variable, use REGISTER_SERIALIZABLE_FIELD macro in case of necessity of constructor
#define DECLARE_SERIALIZABLE_FIELD(Type, Name, ...)                                             \
    DECLARE_SERIALIZABLE_FIELD_N(Type, Name, STRINGINIZE(Name), __VA_ARGS__)

// Declaring a serializable field with a special serializable name and registering it in global serializable objects register
// Use case DECLARE_SERIALIZABLE_FIELD(long, m_propName, "Pretty name"); => long m_propName = long(); REGISTER_SERIALIZABLE_FIELD_N("Pretty name", m_propName);
// Use case DECLARE_SERIALIZABLE_FIELD(long, m_propName, "Pretty name", 105); => long m_propName = long(105); REGISTER_SERIALIZABLE_FIELD_N("Pretty name", m_propName);
// !! You mustn't use a constructor for this variable, use REGISTER_SERIALIZABLE_FIELD macro in case of necessity of constructor
#define DECLARE_SERIALIZABLE_FIELD_N(Type, Name, SerializableName, ...)                         \
    REMOVE_PARENTHESES(Type) Name = [this]() -> REMOVE_PARENTHESES(Type)                        \
        {                                                                                       \
            REGISTER_SERIALIZABLE_FIELD_N(SerializableName, Name);                              \
            return { __VA_ARGS__ };                                                             \
        }()

// Register serializable field of current class in global serializable objects register
#define REGISTER_SERIALIZABLE_FIELD(object)                                                     \
    REGISTER_SERIALIZABLE_FIELD_N(STRINGINIZE(object), object)

// Register serializable field of current class in global serializable objects register with pretty name
#define REGISTER_SERIALIZABLE_FIELD_N(SerializableName, object)                                 \
    CALL_ONCE({                                                                                 \
            using CurrentType = std::remove_reference_t<decltype(*this)>;                       \
            static_assert(ext::serializable::is_registered_serializable_object_v<CurrentType>,  \
                "Trying to register field on non registered object, maybe you forget to add REGISTER_SERIALIZABLE_OBJECT?");    \
            auto& __info = ext::get_singleton<ext::serializable::SerializableObjectDescriptor<CurrentType>>();                  \
            __info.RegisterField(SerializableName, &CurrentType::object);                       \
        })

namespace ext::serializable {

// Helper to check if class has __serializable_object_registration field 
DECLARE_CHECK_FIELD_EXISTS(__serializable_object_registration);
// Check if class has __serializable_object_registration field
template<class T>
inline constexpr bool is_registered_serializable_object_v = has___serializable_object_registration_field_v<std::remove_pointer_t<std::extract_value_type_v<T>>>;
// Check if object is serializable
template<class T>
inline constexpr bool is_serializable_object = is_registered_serializable_object_v<T>
#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20
    || (std::is_class_v<T>
        // Exceptions list
        && !std::is_same_v<T, std::string>
        && !std::is_same_v<T, std::wstring>
        && !is_duration_v<T>
        && !std::is_same_v<T, std::chrono::system_clock::time_point>
        && !std::is_same_v<T, std::filesystem::path>)
#endif // C++20
    ;

struct SerializableNode;

// Common internal interface for serializable objects, use ISerializableField or ISerializableObject for your objects
struct ISerializable
{
    virtual ~ISerializable() = default;

    // Called before collection serialization
    virtual void OnSerializationStart() {}
    // Called after collection serialization
    virtual void OnSerializationEnd() {}

    // Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allow to add upgrade for old stored settings
    // Also used to allocate collections elements
    virtual void OnDeserializationStart(SerializableNode& /*serializableTree*/) {}
    // Called after collection deserialization
    virtual void OnDeserializationEnd() {}
};

// Interface for serializable value, @see serialize_value/deserialize_value functions
// For serializing custom class you should overload std::wostringstream operator<< and operator>>
struct ISerializableValue : ISerializable
{
    // Serialize field value to string
    [[nodiscard]] virtual SerializableValue SerializeValue() const = 0;
    // Deserialize field value from string
    virtual void DeserializeValue(const SerializableValue& value) = 0;
};

// Interface for serializable field, used in serializable objects
struct ISerializableField : ISerializable
{
    // Field name/map key
    [[nodiscard]] virtual SerializableValue GetName() const = 0;
    // Get field value
    [[nodiscard]] virtual std::shared_ptr<ISerializable> GetField() = 0;
};

// Interface for serializable objects(object which contain multiple fields or a dictionaries)
struct ISerializableObject : ISerializable
{
    // Object name, used mostly for debugging
    [[nodiscard]] virtual const char* ObjectName() const noexcept = 0;
    // Fields/objects count
    [[nodiscard]] virtual size_t Size() const noexcept = 0;
    // Get field by index
    [[nodiscard]] virtual std::shared_ptr<ISerializableField> Get(const size_t& index) = 0;
};

// Interface for array of serializable objects
struct ISerializableArray : ISerializable
{
    // Array size
    [[nodiscard]] virtual size_t Size() const noexcept = 0;
    // Get array element by index
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get(const size_t& index) = 0;
};

// Interface for optional value like std::optional<T>
struct ISerializableOptional : ISerializable
{
    // if optional has value return serializable element, return SerializableValue with SerializableValue::ValueType::eNull otherwise
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get() const = 0;
};

namespace details {
// Object fields information, used to get field or base classes from the object 

// Field information, used to get field value from object
struct ISerializableFieldInfo
{
    virtual ~ISerializableFieldInfo() = default;
    [[nodiscard]] virtual const char* GetName() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ISerializable> GetField(void* objectPointer) const = 0;
};

// Serializable object base information
struct ISerializableBaseInfo
{
    virtual ~ISerializableBaseInfo() = default;
    [[nodiscard]] virtual size_t CountFields(void* objectPointer) const = 0;
    [[nodiscard]] virtual std::shared_ptr<ISerializableFieldInfo> GetField(const size_t& index, void* objectPointer) const = 0;
};
} // namespace details

/*
* Global register for serializable objects fields with descriptors, use ext::get_singleton<Object>() to get it.
*/
template <class Type>
class SerializableObjectDescriptor
{
    friend ext::Singleton<SerializableObjectDescriptor<Type>>;

public:
    // Register objects field for serialization
    template <class Field>
    void RegisterField(const char* name, Field Type::* field);
    // Register object base classes which will need to be serialized
    template <class... Classes>
    void RegisterSerializableBaseClasses();
    // Get object serializable fields count
    [[nodiscard]] size_t GetFieldsCount(void* objectPointer) const;
    // Get serializable collection with all registered fields
    [[nodiscard]] std::shared_ptr<ISerializableObject> GetSerializable(Type& object) const;
    // Conversion object function, allows to avoid private inheritance problems
    template <class ConvertedType>
    [[nodiscard]] ConvertedType* ConvertToType(Type* pointer) const;
    // Proxy function to call ISerializable methods on object, required if they declared as private 
    void CallOnSerializationStart(Type* pointer) const;
    void CallOnSerializationEnd(Type* pointer) const;
    void CallOnDeserializationStart(Type* pointer, SerializableNode &serializableTree) const;
    void CallOnDeserializationEnd(Type* pointer) const;

private:
    std::list<std::shared_ptr<details::ISerializableBaseInfo>> m_baseSerializableClasses;
    std::list<std::shared_ptr<details::ISerializableFieldInfo>> m_fields;
};

// Serialization tree, allow to iterate over serializable objects, @see also ext::serializer::Visitor
struct SerializableNode
{
    [[nodiscard]] static SerializableNode ArrayNode() noexcept { return SerializableNode(NodeType::eArray); }
    [[nodiscard]] static SerializableNode ObjectNode(const char* objectName = nullptr) noexcept
    {
        SerializableNode node(NodeType::eObject);
        if (objectName)
            node.Name.emplace(std::widen(objectName));
        return node;
    }
    [[nodiscard]] static SerializableNode ValueNode(SerializableValue fieldValue = SerializableValue::CreateNull()) noexcept
    {
        SerializableNode node(NodeType::eValue);
        node.Value.emplace(std::move(fieldValue));
        return node;
    }
    [[nodiscard]] static SerializableNode FieldNode(SerializableValue name) noexcept
    {
        SerializableNode node(NodeType::eField);
        node.Name.emplace(std::move(name));
        return node;
    }

    SerializableNode* AddChild(SerializableNode&& node)
    {
        EXT_ASSERT(Type == NodeType::eArray || Type == NodeType::eObject || Type == NodeType::eField);
        auto childNode = m_childNodes.emplace_back(std::make_shared<SerializableNode>(std::move(node)));
        childNode->Parent = this;
        return childNode.get();
    }

    void CacheChildNames() noexcept
    {
        EXT_ASSERT(Type == NodeType::eObject);
        EXT_ASSERT(m_childNodesByNames.empty());

        for (const auto& child : m_childNodes)
        {
            m_childNodesByNames.emplace(child->Name.value(), child);
        }
    }

    [[nodiscard]] SerializableNode* GetChild(const std::wstring& name, size_t indexAmongTheSameNames = 0) const noexcept
    {
        EXT_ASSERT(Type == NodeType::eObject);
        auto range = m_childNodesByNames.equal_range(name);
        if (size_t(std::distance(range.first, range.second)) <= indexAmongTheSameNames)
            return nullptr;

        return std::next(range.first, indexAmongTheSameNames)->second.get();
    }

    [[nodiscard]] SerializableNode* GetChild(size_t index) const
    {
        EXT_ASSERT(Type == NodeType::eArray || Type == NodeType::eObject || Type == NodeType::eField);
        EXT_EXPECT(index < m_childNodes.size()) << "No child with index " << index;
        return std::next(m_childNodes.begin(), index)->get();
    }

    [[nodiscard]] size_t CountChilds() const { return m_childNodes.size(); }

public:
    enum class NodeType
    {
        eArray,
        eObject,
        eField,
        eValue,
    } Type;

    SerializableNode* Parent;
    // Set only for fields and objects
    std::optional<SerializableValue> Name;
    // Set only for values
    std::optional<SerializableValue> Value;

private:
    explicit SerializableNode(NodeType type) noexcept : Parent(nullptr), Type(type) {}
    // Collection child nodes, for field will contain 1 node
    std::list<std::shared_ptr<SerializableNode>> m_childNodes;
    // Cache for child nodes by names, used for fast access to child nodes by name
    std::multimap<std::wstring, std::shared_ptr<SerializableNode>> m_childNodesByNames;
};

} // namespace ext::serializable

#include <ext/details/serialization/serializable_details.h>

#include <ext/serialization/serializer.h>
