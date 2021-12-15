#pragma once

/*
    Serialization for objects

    Implementation of base classes to simplify serialization / deserialization of data
    Essentially - an attempt to create reflection in C++

    Usage example:

struct InternalStruct : ext::serializable::SerializableObject<InternalStruct, L"Pretty name">
{
    DECLARE_SERIALIZABLE((long) value);
    DECLARE_SERIALIZABLE((std::list<int>) valueList);
};

struct CustomField: ISerializableField
{
    EXT_NODISCARD virtual const wchar_t* GetName() const EXT_NOEXCEPT { return L"CustomFieldName"; }
    EXT_NODISCARD virtual SerializableValue SerializeValue() const { return L"test"; }
    virtual void DeserializeValue(const SerializableValue& value) { EXT_EXPECT(value == L"test"); }
};

struct TestStruct :  ext::serializable::SerializableObject<TestStruct>, InternalStruct
{
    REGISTER_SERIALIZABLE_BASE(InternalStruct);

    DECLARE_SERIALIZABLE((long) valueLong, 2);
    DECLARE_SERIALIZABLE((int) valueInt);
    DECLARE_SERIALIZABLE((std::vector<bool>) boolVector, { true, false });

    DECLARE_SERIALIZABLE((CustomField) field);
    DECLARE_SERIALIZABLE((InternalStruct) internalStruct);

    std::list<int> m_listOfParams;

    MyTestStruct()
    {
        REGISTER_SERIALIZABLE_OBJECT(m_listOfParams); // or use DECLARE_SERIALIZABLE macro

        Executor::DeserializeObject(Fabric::XMLDeserializer(L"C:\\Test.xml"), testStruct);
    }

    ~MyTestStruct()
    {
        Executor::SerializeObject(Fabric::XMLSerializer(L"C:\\Test.xml"), testStruct);
    }
};

*/

#include <algorithm>
#include <list>
#include <type_traits>
#include <memory>

#include <ext/core/defines.h>
#include <ext/core/check.h>

#include <ext/serialization/serializable_value.h>

// Help macros
#define REM(...) __VA_ARGS__
#define TYPE_AND_OBJECT(x) REM x
// Declare property  DECLARE_OBJECT((int) x) => int x
#define DECLARE_OBJECT(object) TYPE_AND_OBJECT(object)

#define EAT(...)
#define REMOVE_TYPE(x) EAT##x
// Get object name.  GET_OBJECT((int) x) => x
#define GET_OBJECT(object) REMOVE_TYPE(object)

#define GET_OBJECT_NAME(object) TEXT(STRINGINIZE(object))

/*
* Register serializable field of current class, current class must be inherited from SerializableObject
* Calls SerializableObject:RegisterField function, return object with same type and constructed with __VA_ARGS__ parameters
*/
#define REGISTER_SERIALIZABLE_OBJECT_N(name, object) \
SerializableObject<std::remove_pointer_t<decltype(this)>>::RegisterField(name, &std::remove_pointer_t<decltype(this)>::object)

#define REGISTER_SERIALIZABLE_OBJECT(object) \
REGISTER_SERIALIZABLE_OBJECT_N(TEXT(STRINGINIZE(object)), object)

// Declaring a serializable property and registering it in a SerializableObject base class
// Use case DECLARE_SERIALIZABLE((long) m_propName); => long m_propName = long();  SerializableObject::RegisterField(this, &MyTestStruct::m_propName);
// Use case DECLARE_SERIALIZABLE((long) m_propName, 105); => long m_propName = long(105);  SerializableObject::RegisterField(this, &MyTestStruct::m_propName, 105);
// !!Some compilers require to add space after the parenthesis
// !!You cannot use a constructor for this variable, use REGISTER_SERIALIZABLE_OBJECT macro in case of necessity of constructor
#define DECLARE_SERIALIZABLE_N(Name, Property, ...)                                 \
DECLARE_OBJECT(Property) = [this]()                                                 \
    {                                                                               \
        REGISTER_SERIALIZABLE_OBJECT_N(Name, GET_OBJECT(Property));                 \
        return decltype(std::remove_pointer_t<decltype(this)>::GET_OBJECT(Property)){ __VA_ARGS__ };   \
    }()

#define DECLARE_SERIALIZABLE(Property, ...)                                         \
DECLARE_SERIALIZABLE_N(GET_OBJECT_NAME(GET_OBJECT(Property)), Property, __VA_ARGS__)

/*
* Declare operator ISerializable* to avoid ambiguous conversion to ISeralizable from current and base objects
* Register base class as base field to current class, you can call AddSerializableBase in constructor
*/
#define REGISTER_SERIALIZABLE_BASE(...)                                                                                 \
operator ISerializable*() { return static_cast<SerializableObject<std::remove_pointer_t<decltype(this)>>*>(this); }     \
const bool ___BaseClassRegistrator = [this]()                                                                           \
    {                                                                                                                   \
        SerializableObject<std::remove_pointer_t<decltype(this)>>::RegisterSerializableBaseClasses<__VA_ARGS__>();      \
        return true;                                                                                                    \
    }();

namespace ext::serializable {

// Serialization tree, allow to iterate over serializable fields, @see also ext::serializable::serializer::Visitor
struct SerializableNode
{
    std::wstring Name;
    std::weak_ptr<SerializableNode> Parent;

    std::optional<SerializableValue> Value;
    std::list<std::shared_ptr<SerializableNode>> ChildNodes;

    SerializableNode(std::wstring name, std::shared_ptr<SerializableNode> parentNode = nullptr) EXT_NOEXCEPT
        : Name(std::move(name)), Parent(parentNode)
    {}
    EXT_NODISCARD std::shared_ptr<SerializableNode> GetChild(const wchar_t* name, const size_t& indexAmongTheSameNames = 0) const EXT_NOEXCEPT
    {
        auto searchIt = ChildNodes.begin(), end = ChildNodes.end();
        for (size_t i = 0; i <= indexAmongTheSameNames && searchIt != end; ++i)
        {
            searchIt = std::find_if(i == 0 ? searchIt : std::next(searchIt), end, [&name](const auto& node) { return node->Name == name; });
        }
        EXT_ASSERT(searchIt != end) << "Can`t find " << name << " in child nodes " << Parent.lock() ? Parent.lock()->Name : L"";
        return searchIt != end ? *searchIt : nullptr;
    }
};

// Common internal interface for serializable objects, use ISerializableField or ISerializableCollection for your objects
struct ISerializable
{
    virtual ~ISerializable() = default;
    // Get name of serializable object
    EXT_NODISCARD virtual const wchar_t* GetName() const EXT_NOEXCEPT = 0;
    // Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allow to add upgrade for old stored settings
    // Also used to allocate collections elements
    virtual void PrepareToDeserialize(_Inout_ std::shared_ptr<SerializableNode>& /*serializableTree*/) EXT_THROWS() {}
};

// Interface for serializable field, @see serialize_value/deserialize_value functions
// For serializing custom class you should overload std::wostringstream operator<< and operator>>
struct ISerializableField : ISerializable
{
    // Serialize field value to string
    EXT_NODISCARD virtual SerializableValue SerializeValue() const = 0;
    // Deserialize field value from string
    virtual void DeserializeValue(const SerializableValue& value) = 0;
};

// Interface for collections of serializable objects, to prepare collection to deserialization see ISerializable::PrepareToDeserialize
struct ISerializableCollection : ISerializable
{
    // Collection size
    EXT_NODISCARD virtual size_t Size() const EXT_NOEXCEPT = 0;
    // Get collection element by index
    EXT_NODISCARD virtual std::shared_ptr<ISerializable> Get(const size_t& index) const = 0;
};

// Interface for optional value like std::optional<T>
struct ISerializableOptional : ISerializable
{
    // if optional has value return serializable element, return SerializableValue with SerializableValue::ValueType::eNull otherwise
    EXT_NODISCARD virtual std::shared_ptr<ISerializable> Get() const = 0;
};

namespace impl {

struct ISerializableFieldInfo
{
    EXT_NODISCARD virtual std::shared_ptr<ISerializable> GetField(const ISerializable* object) const = 0;
};
} // namespace impl

/*
* Base class for class with serializable fields, register field by RegisterField function or DECLARE_SERIALIZABLE macros.
*/
template <class Type, const wchar_t* TypeName = nullptr>
struct SerializableObject : ISerializableCollection
{
protected:
    template <class Field>
    void RegisterField(const wchar_t* name, Field Type::* field);

    template <class... Classes>
    void RegisterSerializableBaseClasses();

private:
// ISerializable
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { return m_name.c_str(); }
// ISerializableCollection
    EXT_NODISCARD size_t Size() const EXT_NOEXCEPT override { return m_baseSerializableClasses.size() + m_fields.size(); }
    EXT_NODISCARD std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        if (index < m_baseSerializableClasses.size())
            return *std::next(m_baseSerializableClasses.begin(), index);
        if (index - m_baseSerializableClasses.size() >= m_fields.size())
            return nullptr;
        return std::next(m_fields.begin(), index - m_baseSerializableClasses.size())->get()->GetField(this);
    }

private:
    std::list<std::shared_ptr<ISerializable>> m_baseSerializableClasses;
    std::list<std::shared_ptr<impl::ISerializableFieldInfo>> m_fields;

    const std::wstring m_name = TypeName != nullptr ? TypeName : std::widen(typeid(Type).name());
};

} // namespace ext::serializable

#include <ext/serialization/serializable_impl.h>
#include <ext/serialization/serializer.h>