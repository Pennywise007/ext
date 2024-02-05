#pragma once

/*
    Serialization for objects

    Implementation of base classes to simplify serialization / deserialization of data
    Essentially - an attempt to create reflection in C++

    Usage example:

struct InternalStruct : ext::serializable::SerializableObject<InternalStruct, "Pretty name">
{
    DECLARE_SERIALIZABLE_FIELD(long, value);
    DECLARE_SERIALIZABLE_FIELD(std::list<int>, valueList);
};

struct CustomField : ISerializableField
{
    [[nodiscard]] virtual const char* GetName() const noexcept { return "CustomFieldName"; }
    [[nodiscard]] virtual SerializableValue SerializeValue() const { return L"test"; }
    virtual void DeserializeValue(const SerializableValue& value) { EXT_EXPECT(value == L"test"); }
};

struct TestStruct :  ext::serializable::SerializableObject<TestStruct>, InternalStruct
{
    REGISTER_SERIALIZABLE_BASE(InternalStruct);

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
        Executor::DeserializeObject(Factory::XMLDeserializer(L"C:\\Test.xml"), testStruct);
    }

    ~MyTestStruct()
    {
        using namespace ext::serializer;
        Executor::SerializeObject(Factory::XMLSerializer(L"C:\\Test.xml"), testStruct);
    }
};

*/

#include <algorithm>
#include <list>
#include <type_traits>
#include <memory>
#include <optional>

#include <ext/core/defines.h>
#include <ext/core/check.h>

#include <ext/serialization/serializable_value.h>

#include <ext/types/utils.h>

/*
* Register serializable field of current class, current class must be inherited from SerializableObject
* Calls SerializableObject:RegisterField function, return object with same type and constructed with __VA_ARGS__ parameters
*/
#define REGISTER_SERIALIZABLE_FIELD_N(name, object)                         \
    using CurrentType = std::remove_pointer_t<decltype(this)>;              \
    SerializableObjectType::RegisterField(name, &CurrentType::object)

#define REGISTER_SERIALIZABLE_FIELD(object)                                 \
    REGISTER_SERIALIZABLE_FIELD_N(STRINGINIZE(object), object)

// Declaring a serializable field and registering it in a SerializableObject base class
// Use case DECLARE_SERIALIZABLE_FIELD(long, m_propName); => long m_propName = long();  SerializableObject::RegisterField(this, &MyTestStruct::m_propName);
// Use case DECLARE_SERIALIZABLE_FIELD(long, m_propName, 105); => long m_propName = long(105);  SerializableObject::RegisterField(this, &MyTestStruct::m_propName, 105);
// !! You cannot use a constructor for this variable, use REGISTER_SERIALIZABLE_FIELD macro in case of necessity of constructor
#define DECLARE_SERIALIZABLE_FIELD(Type, Name, ...)                         \
    DECLARE_SERIALIZABLE_FIELD_N(STRINGINIZE(Name), Type, Name, __VA_ARGS__)

// Declaring a serializable field with a special serializable name and registering it in a SerializableObject base class
// Use case DECLARE_SERIALIZABLE_FIELD_N(long, m_propName, "Pretty name"); => long m_propName = long();  SerializableObject::RegisterField(this, &MyTestStruct::m_propName);
// Use case DECLARE_SERIALIZABLE_FIELD_N(long, m_propName, "Pretty name", 105); => long m_propName = long(105);  SerializableObject::RegisterField(this, &MyTestStruct::m_propName, 105);
// !! You cannot use a constructor for this variable, use REGISTER_SERIALIZABLE_FIELD macro in case of necessity of constructor
#define DECLARE_SERIALIZABLE_FIELD_N(SerializableName, Type, Name, ...)     \
    REMOVE_PARENTHESES(Type) Name = [this]()                                \
        {                                                                   \
            REGISTER_SERIALIZABLE_FIELD_N(SerializableName, Name);          \
            return REMOVE_PARENTHESES(Type) { __VA_ARGS__ };                \
        }()

/*
* Declare operator ISerializable* to avoid ambiguous conversion to ISerializable from current and base objects
* Register base class as base field to current class, you can call AddSerializableBase in constructor
*/
#define REGISTER_SERIALIZABLE_BASE(...)                                                                                     \
    operator ISerializable*() { return static_cast<SerializableObjectType*>(this); }     \
    const bool ___BaseClassRegistration = [this]()                                                                          \
        {                                                                                                                   \
            SerializableObjectType::RegisterSerializableBaseClasses<__VA_ARGS__>();                                         \
            return true;                                                                                                    \
        }()

namespace ext::serializable {

// Serialization tree, allow to iterate over serializable fields, @see also ext::serializer::Visitor
struct SerializableNode
{
    std::string Name;
    const std::weak_ptr<SerializableNode> Parent;

    std::optional<SerializableValue> Value;
    std::list<std::shared_ptr<SerializableNode>> ChildNodes;

    SerializableNode(std::string name, std::shared_ptr<SerializableNode> parentNode = nullptr) noexcept
        : Name(std::move(name)), Parent(parentNode)
    {}
    [[nodiscard]] std::shared_ptr<SerializableNode> GetChild(const std::string& name, const size_t& indexAmongTheSameNames = 0) const noexcept
    {
        auto searchIt = ChildNodes.begin(), end = ChildNodes.end();
        for (size_t i = 0; i <= indexAmongTheSameNames && searchIt != end; ++i)
        {
            searchIt = std::find_if(i == 0 ? searchIt : std::next(searchIt), end, [&name](const auto& node) { return node->Name == name; });
        }
        EXT_ASSERT(searchIt != end) << "Can`t find " << name.c_str() << " in child nodes " << Parent.lock() ? Parent.lock()->Name : "";
        return searchIt != end ? *searchIt : nullptr;
    }
};

// Common internal interface for serializable objects, use ISerializableField or ISerializableCollection for your objects
struct ISerializable
{
    virtual ~ISerializable() = default;
    // Get name of serializable object
    [[nodiscard]] virtual const char* GetName() const noexcept = 0;
    // Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allow to add upgrade for old stored settings
    // Also used to allocate collections elements
    virtual void PrepareToDeserialize(std::shared_ptr<SerializableNode>& /*serializableTree*/) EXT_THROWS() {}
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

// Interface for collections of serializable objects, to prepare collection to deserialization see ISerializable::PrepareToDeserialize
struct ISerializableCollection : ISerializable
{
    // Collection size
    [[nodiscard]] virtual size_t Size() const noexcept = 0;
    // Get collection element by index
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get(const size_t& index) const = 0;

    // Called before collection serialization
    virtual void OnSerializationStart() {}
    // Called after collection serialization
    virtual void OnSerializationEnd() {};

    // Called before collection deserialization
    virtual void OnDeserializationStart() {}
    // Called after collection deserialization
    virtual void OnDeserializationEnd() {};
};

// Interface for optional value like std::optional<T>
struct ISerializableOptional : ISerializable
{
    // if optional has value return serializable element, return SerializableValue with SerializableValue::ValueType::eNull otherwise
    [[nodiscard]] virtual std::shared_ptr<ISerializable> Get() const = 0;
};

namespace details {

struct ISerializableFieldInfo
{
    virtual ~ISerializableFieldInfo() = default;
    [[nodiscard]] virtual std::shared_ptr<ISerializable> GetField(const ISerializable* object) const = 0;
};
} // namespace details

/*
* Base class for class with serializable fields, register field by RegisterField function or DECLARE_SERIALIZABLE_FIELD macros.
*/
template <class Type, const char* TypeName = nullptr, class ICollectionInterface = ISerializableCollection>
struct SerializableObject : ICollectionInterface
{
    static_assert(std::is_base_of_v<ISerializableCollection, ICollectionInterface>, "Collection should be derived from ISerializableCollection");
protected:
    typedef SerializableObject<Type, TypeName, ICollectionInterface> SerializableObjectType;

    template <class... Classes>
    void RegisterSerializableBaseClasses();

#if (_MSC_FULL_VER < 192027508) // fix problem with on 141 toolset with inheritance
public:
#endif
    template <class Field>
    void RegisterField(const char* name, Field Type::* field);

private:
// ISerializable
    [[nodiscard]] const char* GetName() const noexcept override { return m_name; }
// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { return m_baseSerializableClasses.size() + m_fields.size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        if (index < m_baseSerializableClasses.size())
            return *std::next(m_baseSerializableClasses.begin(), index);
        if (index - m_baseSerializableClasses.size() >= m_fields.size())
            return nullptr;
        return std::next(m_fields.begin(), index - m_baseSerializableClasses.size())->get()->GetField(this);
    }

private:
    std::list<std::shared_ptr<ISerializable>> m_baseSerializableClasses;
    std::list<std::shared_ptr<details::ISerializableFieldInfo>> m_fields;

    const char* m_name = TypeName != nullptr ? TypeName : ext::type_name<Type>();
};

} // namespace ext::serializable

#include <ext/details/serialization/serializable_details.h>
#include <ext/serialization/serializer.h>