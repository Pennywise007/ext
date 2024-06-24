#pragma once

/*
    Serialization for objects

    Implementation of base classes to simplify serialization / deserialization of data
    Essentially - an attempt to create reflection in C++

    Usage example:

struct InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(long, value);
    DECLARE_SERIALIZABLE_FIELD(std::list<int>, valueList);
};

struct CustomField : ISerializableField
{
    [[nodiscard]] virtual const char* GetName() const noexcept { return "CustomFieldName"; }
    [[nodiscard]] virtual SerializableValue SerializeValue() const { return L"test"; }
    virtual void DeserializeValue(const SerializableValue& value) { EXT_EXPECT(value == L"test"); }
};

struct TestStruct : InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT(InternalStruct);

    DECLARE_SERIALIZABLE_FIELD(long, valueLong, 2);
    DECLARE_SERIALIZABLE_FIELD(int, valueInt);
    DECLARE_SERIALIZABLE_FIELD(std::vector<bool>, boolVector, { true, false });

    DECLARE_SERIALIZABLE_FIELD(CustomField, field);
    DECLARE_SERIALIZABLE_FIELD(InternalStruct, internalStruct);

    std::list<int> m_listOfParams;

    MyTestStruct()
    {
        REGISTER_SERIALIZABLE_FIELD(m_listOfParams); // or use DECLARE_SERIALIZABLE_FIELD macro
        using namespace ext::serializer;
        DeserializeObject(Factory::XMLDeserializer(L"C:\\Test.xml"), testStruct);
    }

    ~MyTestStruct()
    {
        using namespace ext::serializer;
        SerializeObject(Factory::XMLSerializer(L"C:\\Test.xml"), testStruct);
    }
};

You can also declare this functions in your REGISTER_SERIALIZABLE_OBJECT object to get notified when (de)serialization was called:
    // Called before collection serialization
    void OnSerializationStart() {}
    // Called after collection serialization
    void OnSerializationEnd() {};

    // Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allow to add upgrade for old stored settings
    // Also used to allocate collections elements
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& serializableTree) {}
    // Called after collection deserialization
    void OnDeserializationEnd() {};
*/

#include <algorithm>
#include <list>
#include <type_traits>
#include <map>
#include <memory>
#include <optional>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/singleton.h>

#include <ext/serialization/serializable_value.h>

#include <ext/std/type_traits.h>

#include <ext/utils/call_once.h>
#include <ext/utils/reflection.h>

// Register serializable object and it's based classes
#define REGISTER_SERIALIZABLE_OBJECT(/*Serializable base classes list*/...)                     \
    REGISTER_SERIALIZABLE_OBJECT_N(nullptr, __VA_ARGS__)

// Register serializable object, will register object name and based classes
#define REGISTER_SERIALIZABLE_OBJECT_N(SerializableName, /*Serializable base classes list*/...) \
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
                __info.SetName(SerializableName);                                               \
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

struct SerializableNode;

// Common internal interface for serializable objects, use ISerializableField or ISerializableCollection for your objects
struct ISerializable
{
    virtual ~ISerializable() = default;
    // Get name of serializable object
    [[nodiscard]] virtual const char* GetName() const noexcept = 0;

    // Called before collection serialization
    virtual void OnSerializationStart() {}
    // Called after collection serialization
    virtual void OnSerializationEnd() {};

    // Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allow to add upgrade for old stored settings
    // Also used to allocate collections elements
    virtual void OnDeserializationStart(std::shared_ptr<SerializableNode>& /*serializableTree*/) {}
    // Called after collection deserialization
    virtual void OnDeserializationEnd() {};
};

// Interface for serializable field, @see serialize_value/deserialize_value functions
// For serializing custom class you should overload std::wostringstream operator<< and operator>>
struct ISerializableField : ISerializable
{
    // Serialize field value to string
    [[nodiscard]] virtual SerializableValue SerializeValue() const = 0;
    // Deserialize field value from string
    virtual void DeserializeValue(const SerializableValue& value) = 0;
};

// Interface for collections of serializable objects
struct ISerializableCollection : ISerializable
{
    // Collection size
    [[nodiscard]] virtual size_t Size() const noexcept = 0;
    // Get collection element by index
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get(const size_t& index) const = 0;
};

// Interface for optional value like std::optional<T>
struct ISerializableOptional : ISerializable
{
    // if optional has value return serializable element, return SerializableValue with SerializableValue::ValueType::eNull otherwise
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get() const = 0;
};

namespace details {
// Field information, used to get field value from object
struct ISerializableFieldInfo
{
    virtual ~ISerializableFieldInfo() = default;
    [[nodiscard]] virtual std::shared_ptr<ISerializable> GetField(void* objectPointer) const = 0;
};

// Serializable object base information
struct ISerializableBaseInfo
{
    virtual ~ISerializableBaseInfo() = default;
    [[nodiscard]] virtual size_t CountFields() const = 0;
    [[nodiscard]] virtual std::shared_ptr<ISerializable> GetField(const size_t& index, void* objectPointer) const = 0;
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
    // Set custom serialization name for object
    void SetName(const char* name) { m_name = name; }
    // Register objects field for serialization
    template <class Field>
    void RegisterField(const char* name, Field Type::* field);
    // Register object base classes which will need to be serialized
    template <class... Classes>
    void RegisterSerializableBaseClasses();
    // Get object serializable fields count
    [[nodiscard]] size_t GetFieldsCount() const;
    // Get serializable collection with all registered fields
    [[nodiscard]] std::shared_ptr<ISerializableCollection> GetSerializable(Type& object, const char* customName = nullptr) const;
    // Conversion object function, allows to avoid private inheritance problems
    template <class ConvertedType>
    [[nodiscard]] ConvertedType* ConvertToType(Type* pointer) const;
    // Proxy function to call ISerializable methods on object, required if they declared as private 
    void CallOnSerializationStart(Type* pointer) const;
    void CallOnSerializationEnd(Type* pointer) const;
    void CallOnDeserializationStart(Type* pointer, std::shared_ptr<SerializableNode> &serializableTree) const;
    void CallOnDeserializationEnd(Type* pointer) const;

private:
    std::list<std::shared_ptr<details::ISerializableBaseInfo>> m_baseSerializableClasses;
    std::list<std::shared_ptr<details::ISerializableFieldInfo>> m_fields;
    const char* m_name = nullptr;
};

// Serialization tree, allow to iterate over serializable fields, @see also ext::serializer::Visitor
struct SerializableNode
{
    std::string Name;
    const std::weak_ptr<SerializableNode> Parent;

    std::optional<SerializableValue> Value;

    SerializableNode(std::string name, std::shared_ptr<SerializableNode> parentNode = nullptr) noexcept
        : Name(std::move(name)), Parent(parentNode)
    {}

    std::shared_ptr<SerializableNode> AddChild(const std::string& name, const std::shared_ptr<SerializableNode>&parentNode)
    {
        return m_childNodes.emplace_back(std::make_shared<SerializableNode>(name, parentNode));
    }
    [[nodiscard]] std::shared_ptr<SerializableNode> GetChild(const std::string& name, const size_t& indexAmongTheSameNames = 0) noexcept
    {
        if (m_childNodesByNames.size() != m_childNodes.size())
        {
            m_childNodesByNames.clear();
            for (const auto& child : m_childNodes)
            {
                m_childNodesByNames.emplace(std::pair(child->Name, child));
            }
        }
        auto range = m_childNodesByNames.equal_range(name);
        if (size_t(std::distance(range.first, range.second)) <= indexAmongTheSameNames)
        {
            EXT_ASSERT(false) << "Can`t find " << name.c_str() << " in child nodes " << Parent.lock() ? Parent.lock()->Name : "";
            return nullptr;
        }

        return std::next(range.first, indexAmongTheSameNames)->second;
    }
    [[nodiscard]] std::shared_ptr<SerializableNode> GetChild(size_t index) const
    {
        EXT_ASSERT(index < m_childNodes.size());
        return *std::next(m_childNodes.begin(), index);
    }
    [[nodiscard]] bool HasChilds() const
    {
        return !m_childNodes.empty();
    }
    [[nodiscard]] size_t CountChilds() const
    {
        return m_childNodes.size();
    }

private:
    std::list<std::shared_ptr<SerializableNode>> m_childNodes;
    std::multimap<std::string, std::shared_ptr<SerializableNode>> m_childNodesByNames;
};

} // namespace ext::serializable

#include <ext/details/serialization/serializable_details.h>
#include <ext/serialization/serializer.h>