#pragma once

/*
    Serialization for objects

    Implementation of base classes to simplify serialization / deserialization of data
    Essentially - an attempt to create reflection in C++

    Usage example:

struct InternalStruct : SerializableObject<InternalStruct, L"Pretty name">
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

struct TestStruct : SerializableObject<TestStruct>, InternalStruct
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
#include <vector>
#include <map>
#include <memory>
#include <set>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/noncopyable.h>

#include <ext/serialization/serializable_value.h>

#include <ext/utils/string.h>

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
* Calls SerializableObject:AddField function, return object with same type and constructed with __VA_ARGS__ parameters
*/
#define REGISTER_SERIALIZABLE_OBJECT_N(object, name) \
SerializableObject::RegisterField(this, &std::remove_pointer_t<decltype(this)>::object, name)

#define REGISTER_SERIALIZABLE_OBJECT(object) \
REGISTER_SERIALIZABLE_OBJECT_N(object, TEXT(STRINGINIZE(object)))

// Declaring a serializable property and registering it in a SerializableObject base class
// Use case DECLARE_SERIALIZABLE((long) m_propName); => long m_propName = long() |  SerializableObject::AddField(this, &MyTestStruct::m_propName);
// Use case DECLARE_SERIALIZABLE((long) m_propName, 105); => long m_propName = long(105) |  SerializableObject::AddField(this, &MyTestStruct::m_propName, 105);
// !!Some compilers require to add space after the parenthesis
// !!You cannot use a constructor for this variable, use REGISTER_SERIALIZABLE_OBJECT macro in case of necessity of constructor
#define DECLARE_SERIALIZABLE_N(Property, Name, ...)                         \
DECLARE_OBJECT(Property) = [this]() -> decltype(std::remove_pointer_t<decltype(this)>::GET_OBJECT(Property))       \
    {                                                                               \
        REGISTER_SERIALIZABLE_OBJECT_N(GET_OBJECT(Property), Name);                 \
        return { __VA_ARGS__ };                                                     \
    }();

#define DECLARE_SERIALIZABLE(Property, ...)                                 \
DECLARE_SERIALIZABLE_N(Property, GET_OBJECT_NAME(GET_OBJECT(Property)), __VA_ARGS__)

/*
* Declare operator ISerializable* to avoid ambiguous conversion to ISeralizable from current and base objects
* Register base class as base field to current class, you can call AddSerializableBase in constructor
*/
#define REGISTER_SERIALIZABLE_BASE(BaseClass)                                                                       \
operator ISerializable*() { return static_cast<SerializableObject<std::remove_pointer_t<decltype(this)>>*>(this); }  \
const bool ___BaseClassRegistrator = SerializableObject::RegisterBase(static_cast<BaseClass*>(this));

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

#pragma region SerializationInterfaces
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

#pragma endregion SerializationInterfaces

#pragma region SerializableClasses

namespace impl {

// Check is type a std::map/multimap container
template <class T>
struct is_map : std::false_type {};
template<class _Kty, class _Ty, class _Pr, class _Alloc>
struct is_map<std::map<_Kty, _Ty, _Pr, _Alloc>> : std::true_type {};
template<class _Kty, class _Ty, class _Pr, class _Alloc>
struct is_map<std::multimap<_Kty, _Ty, _Pr, _Alloc>> : std::true_type {};
template<class T>
_INLINE_VAR constexpr bool is_map_v = is_map<T>::value;

// Check is type a std::set/multiset container
template <class T>
struct is_set : std::false_type {};
template<class _Kty, class _Pr, class _Alloc>
struct is_set<std::set<_Kty, _Pr, _Alloc>> : std::true_type {};
template<class _Kty, class _Pr, class _Alloc>
struct is_set<std::multiset<_Kty, _Pr, _Alloc>> : std::true_type {};
template<class T>
_INLINE_VAR constexpr bool is_set_v = is_set<T>::value;

// Wrapper on fields, int,long, etc.
template <class Type, typename Field, class = std::void_t<>>
struct SerializableField;

// Wrapper on collections, std::list, std::vector
template <class Type, typename Field>
struct SerializableCollection;

// Wrapper on std::map/std::multimap
template <class Type, typename Field, class = std::void_t<>>
struct SerializableMap;

// Wrapper on std::set/std::multiset
template <class Type, typename Field, class = std::void_t<>>
struct SerializableSet;

// Wrapper on serialiablec objects, inherited from ISerializable
struct SerializableFieldProxy;
struct SerializableCollectionProxy;

struct SerializableCollectionImpl : ISerializableCollection
{
    SerializableCollectionImpl() = default;
    SerializableCollectionImpl(const std::list<std::shared_ptr<ISerializable>>& fields, const wchar_t* name) : m_fields(fields), m_name(name) {}

public:
// ISerializable
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { return m_name; }
// ISerializableCollection
    EXT_NODISCARD size_t Size() const EXT_NOEXCEPT override { return m_fields.size(); }
    EXT_NODISCARD std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        if (index >= m_fields.size()) { EXT_DUMP_IF(true); return nullptr; }
        return *std::next(m_fields.begin(), index);
    };
protected:
    const wchar_t* m_name = nullptr;
    std::list<std::shared_ptr<ISerializable>> m_fields;
};

} // namespace impl

/*
* Base class for class with serializable fields, register field by AddField function or DECLARE_SERIALIZABLE macros.
*/
template <class Type, const wchar_t* TypeName = nullptr>
struct SerializableObject : impl::SerializableCollectionImpl, ext::NonCopyable
{
protected:
    // Add field to serializable fields list, for serializing custom class you should overload std::wostringstream operator<< and operator>>
    template <class Field>
    void RegisterField(Type* type, Field Type::* field, const wchar_t* name)
    {
        std::wstring trimName = name;
        std::string_trim_all(trimName);
        EXT_ASSERT(!trimName.empty());
        if constexpr (impl::is_map_v<Field>)
            m_fields.emplace_back(std::make_shared<impl::SerializableMap<Type, Field>>(type, field, std::move(trimName)));
        else if constexpr (impl::is_set_v<Field>)
            m_fields.emplace_back(std::make_shared<impl::SerializableSet<Type, Field>>(type, field, std::move(trimName)));
        else
            m_fields.emplace_back(std::make_shared<impl::SerializableField<Type, Field>>(type, field, std::move(trimName)));
    }

    // Add vector of types to serializable fields list
    template <class Field>
    void RegisterField(Type* type, std::vector<Field> Type::* field, const wchar_t* name)
    {
        std::wstring trimName = name;
        std::string_trim_all(trimName);
        EXT_ASSERT(!trimName.empty());
        m_fields.emplace_back(std::make_shared<impl::SerializableCollection<Type, std::vector<Field>>>(type, field, std::move(trimName)));
    }

    // Add list of types to serializable fields list
    template <class Field>
    void RegisterField(Type* type, std::list<Field> Type::* field, const wchar_t* name)
    {
        std::wstring trimName = name;
        std::string_trim_all(trimName);
        EXT_ASSERT(!trimName.empty());
        m_fields.emplace_back(std::make_shared<impl::SerializableCollection<Type, std::list<Field>>>(type, field, std::move(trimName)));
    }

    bool RegisterBase(ISerializableCollection* baseCollection)
    {
        m_fields.emplace_front(std::make_shared<impl::SerializableCollectionProxy>(baseCollection));
        return true;
    }

    bool RegisterBase(ISerializableField* baseField)
    {
        m_fields.emplace_front(std::make_shared<impl::SerializableFieldProxy>(baseField));
        return true;
    }

// ISerializable
public:
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { return TypeName != nullptr ? TypeName : m_serializableClassName.c_str(); }

protected:
    const std::wstring m_serializableClassName = std::widen(typeid(Type).name());
};

namespace impl {


// Extract template type, ComPtr<ISerializable> => ISerializable, std::shared_ptr<int> => int
template<typename U>
struct extract_value_type { typedef U value_type; };
template<template<typename> class X, typename U>
struct extract_value_type<X<U>> { typedef U value_type; };
template<class U>
using extract_value_type_v = typename extract_value_type<U>::value_type;

// Check if type is base on Base
// 1. First, we isolate the type (if a template was passed) std::shared_ptr<int> => int
// 2. Remove the pointer if it is a pointer
// 3. Check what is inherited from Base
template<class Base, class T>
_INLINE_VAR constexpr bool is_based_on = std::is_base_of_v<Base, std::remove_pointer_t<extract_value_type_v<T>>>;

template<class T>
_INLINE_VAR constexpr bool is_serializable_v = is_based_on<ISerializable, T>;

template <typename ValueType>
ValueType create_default_value()
{
    if constexpr (std::is_same_v<std::shared_ptr<extract_value_type_v<ValueType>>, ValueType>)
        return std::make_shared<extract_value_type_v<ValueType>>();
    else if constexpr (std::is_same_v<std::unique_ptr<extract_value_type_v<ValueType>>, ValueType>)
        return std::make_unique<extract_value_type_v<ValueType>>();
    return {};
}

template <typename Type>
std::shared_ptr<ISerializable> get_serializable(const Type& value)
{
    static_assert(is_serializable_v<Type>, "Type is not serializable!");
    if constexpr (std::is_same_v<std::shared_ptr<extract_value_type_v<Type>>, Type>)
        return std::static_pointer_cast<ISerializable>(value);
    else
    {
        static_assert(!std::is_pointer_v<Type>, "Type pointer not allowed");
        if constexpr (is_based_on<ISerializableCollection, Type>)
            return std::make_shared<impl::SerializableCollectionProxy>(const_cast<ISerializableCollection*>(static_cast<const ISerializableCollection*>(&value)));
        else
        {
            static_assert(is_based_on<ISerializableField, Type>, "Type should be based on ISerializableField or ISerializableCollection");
            return std::make_shared<impl::SerializableFieldProxy>(const_cast<ISerializableField*>(static_cast<const ISerializableField*>(&value)));
        }
    }
}

// Base class for hold info about serializable field
template <typename ISerializableType, typename Type, typename Field>
struct SerializableBase : ISerializableType
{
    SerializableBase(Type* typePointer, Field Type::* field, std::wstring&& name) : m_typePointer(typePointer), m_field(field), m_name(std::move(name)) {}

// ISerializable
protected:
    EXT_NODISCARD virtual const wchar_t* GetName() const EXT_NOEXCEPT override { return m_name.c_str(); }

protected:
    EXT_NODISCARD Field& GetField() const { return (*m_typePointer).*m_field; }

private:
    Type* m_typePointer;
    Field Type::* m_field;
    const std::wstring m_name;
};

// Class with info about serializable type
template <class Type>
struct SerializableType : ISerializableField
{
    SerializableType(Type* typePointer, std::wstring name) : m_typePointer(typePointer), m_name(std::move(name)) {}

protected:
// ISerializable
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { return m_name.c_str(); }
// ISerializableField
    EXT_NODISCARD SerializableValue SerializeValue() const override { return serialize_value<Type>(*m_typePointer); }
    void DeserializeValue(const SerializableValue& value) override { *m_typePointer = deserialize_value<Type>(value); }

private:
    Type* m_typePointer;
    const std::wstring m_name;
};

// special class for set/map::key_type, all iterators inside is const, we will work with copy and deserialize them in PrepareToDeserialize
struct SerializableOnlyValueHolder : ISerializableField
{
    SerializableOnlyValueHolder(SerializableValue&& value, const wchar_t* name) : value(std::move(value)), m_name(name) {}

    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { return m_name; }
    EXT_NODISCARD SerializableValue SerializeValue() const override { return value; }
    void DeserializeValue(const SerializableValue& /*value*/) override { }

    SerializableValue value;
    const wchar_t* m_name;
};

// Special class for serializing/deserializing std::map/std::multimap types
template <class Type, typename Map>
struct SerializableMap<Type, Map, std::enable_if_t<is_map_v<Map>>> : SerializableBase<ISerializableCollection, Type, Map>
{
    using Base = SerializableBase<ISerializableCollection, Type, Map>;
    SerializableMap(Type* typePointer, Map Type::* field, std::wstring&& name) : Base(typePointer, field, std::move(name)) {}
protected:
// ISerializable
    void PrepareToDeserialize(_Inout_ std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override
    {
        Base::GetField().clear();
        for (const auto& child : deserializableTree->ChildNodes)
        {
            const auto& keyText = child->Name;
            EXT_EXPECT(keyText == L"Map pair");

            decltype(Base::GetField().emplace(typename Map::value_type())) emplaceResult;
            if constexpr (is_serializable_v<typename Map::key_type>)
                emplaceResult = Base::GetField().emplace(create_default_value<typename Map::key_type>(), create_default_value<typename Map::mapped_type>());
            else
            {
                const auto& keyChild = child->GetChild(L"Key");
                EXT_EXPECT(keyChild);
                EXT_EXPECT(keyChild->Value.has_value());
                emplaceResult = Base::GetField().emplace(deserialize_value<typename Map::key_type>(*keyChild->Value), create_default_value<typename Map::mapped_type>());
            }

            typename Map::iterator insertedIter;
            if constexpr (std::is_same_v<decltype(emplaceResult), typename Map::iterator>) // std:::map and multimap emplace got different result type
                insertedIter = std::move(emplaceResult);
            else
            {
                if (!emplaceResult.second)
                {
                    EXT_ASSERT(false) << "Failed to emplace, Type changed?";
                    return;
                }
                insertedIter = std::move(emplaceResult.first);
            }

            if constexpr (!is_serializable_v<typename Map::mapped_type>)
            {
                const auto& valueChild = child->GetChild(L"Value");
                EXT_EXPECT(valueChild);
                EXT_EXPECT(valueChild->Value.has_value());
                insertedIter->second = deserialize_value<typename Map::mapped_type>(*valueChild->Value);
            }
        }
    }

// ISerializableCollection
    EXT_NODISCARD size_t Size() const EXT_NOEXCEPT override { return Base::GetField().size(); }
    EXT_NODISCARD std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        Map& container = Base::GetField();
        if (index >= container.size()) { EXT_DUMP_IF(true); return nullptr; }

        auto it = std::next(container.begin(), index);

        std::list<std::shared_ptr<ISerializable>> pair;
        if constexpr (is_serializable_v<typename Map::key_type>)
            pair.push_back(get_serializable(it->first));
        else
            pair.push_back(std::make_shared<SerializableOnlyValueHolder>(serialize_value(it->first), L"Key"));

        if constexpr (is_serializable_v<typename Map::mapped_type>)
            pair.push_back(get_serializable(it->second));
        else
            pair.push_back(std::make_shared<SerializableOnlyValueHolder>(serialize_value(it->second), L"Value"));

        return std::make_shared<SerializableCollectionImpl>(std::move(pair), L"Map pair");
    }
};

// Special class for serializing/deserializing std::set/std::multiset types
template <class Type, typename Set>
struct SerializableSet<Type, Set, std::enable_if_t<is_set_v<Set>>> : SerializableBase<ISerializableCollection, Type, Set>
{
    using Base = SerializableBase<ISerializableCollection, Type, Set>;
    SerializableSet(Type* typePointer, Set Type::* field, std::wstring&& name) : Base(typePointer, field, std::move(name)) {}
protected:
// ISerializable
    void PrepareToDeserialize(_Inout_ std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override
    {
        Base::GetField().clear();
        for (const auto& child : deserializableTree->ChildNodes)
        {
            if constexpr (is_serializable_v<typename Set::value_type>)
                Base::GetField().emplace(create_default_value<typename Set::value_type>());
            else
            {
                EXT_EXPECT(child->Name == L"Value") << "Expect set node name Value";
                EXT_EXPECT(child->Value.has_value()) << "Expect set node with value not empty";
                Base::GetField().emplace(deserialize_value<typename Set::key_type>(*child->Value));
            }
        }
    }
// ISerializableCollection
    EXT_NODISCARD size_t Size() const EXT_NOEXCEPT override { return Base::GetField().size(); }
    EXT_NODISCARD std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        Set& container = Base::GetField();
        if (index >= container.size()) { EXT_DUMP_IF(true); return nullptr; }

        auto it = std::next(container.begin(), index);
        if constexpr (is_serializable_v<typename Set::value_type>)
            return get_serializable(*it);
        else
            // if set types if not serializable - store string value presentation as index name with empty value
            return std::make_shared<SerializableOnlyValueHolder>(serialize_value(*it), L"Value");
    }
};

// Base class for collection of objects std::vector/list etc.
template <typename Type, typename ArrayFields>
struct SerializableCollection : SerializableBase<ISerializableCollection, Type, ArrayFields>
{
    using Base = SerializableBase<ISerializableCollection, Type, ArrayFields>;
    SerializableCollection(Type* typePointer, ArrayFields Type::* field, std::wstring&& name) : Base(typePointer, field, std::move(name)) {}
protected:
// ISerializable
    void PrepareToDeserialize(_Inout_ std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override
    {
        Base::GetField().resize(deserializableTree->ChildNodes.size());
        for (auto& field : Base::GetField())
        {
            field = create_default_value<typename ArrayFields::value_type>();
        }
    }
// ISerializableCollection
    EXT_NODISCARD size_t Size() const EXT_NOEXCEPT override { return Base::GetField().size(); }
    EXT_NODISCARD std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        auto& list = Base::GetField();
        if (index >= list.size()) { EXT_DUMP_IF(true); return nullptr; }

        auto it = std::next(list.begin(), index);
        if constexpr (is_serializable_v<typename ArrayFields::value_type>)
            return get_serializable(*it);
        else
            return std::make_shared<SerializableType<typename ArrayFields::value_type>>(&*it, serialize_value(index));
    }
};

// Base class for serializable fields
template <class Type, typename Field>
struct SerializableField<Type, Field, std::enable_if_t<!is_serializable_v<Field>>> : SerializableBase<ISerializableField, Type, Field>
{
    using Base = SerializableBase<ISerializableField, Type, Field>;
    SerializableField(Type* typePointer, Field Type::* field, std::wstring&& name) : Base(typePointer, field, std::move(name)) {}

protected:
// ISerializableField
    EXT_NODISCARD SerializableValue SerializeValue() const override { return serialize_value<Field>(Base::GetField()); }
    void DeserializeValue(const SerializableValue& value) override { Base::GetField() = deserialize_value<Field>(value); }
};

// Field bases on ISerializableField
template <class Type, typename Field>
struct SerializableField<Type, Field, std::enable_if_t<is_based_on<ISerializableField, Field>>> : SerializableBase<ISerializableField, Type, Field>
{
    using Base = SerializableBase<ISerializableField, Type, Field>;
    SerializableField(Type* typePointer, Field Type::* field, std::wstring&& /*name*/) : Base(typePointer, field, L"") {}

protected:
// ISerializable
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { return Base::GetField().GetName(); }
// ISerializableField
    EXT_NODISCARD SerializableValue SerializeValue() const override { return Base::GetField().SerializeValue(); }
    void DeserializeValue(const SerializableValue& value) override { Base::GetField().DeserializeValue(value); }
};

// Field bases on ISerializableCollection
template <class Type, typename Field>
struct SerializableField<Type, Field, std::enable_if_t<is_based_on<ISerializableCollection, Field>>> : SerializableBase<ISerializableCollection, Type, Field>
{
    using Base = SerializableBase<ISerializableCollection, Type, Field>;
    SerializableField(Type* typePointer, Field Type::* field, std::wstring&& /*name*/) : Base(typePointer, field, L"") {}

protected:
// ISerializable
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { return Base::GetField().GetName(); }
    void PrepareToDeserialize(_Inout_ std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override { return Base::GetField().PrepareToDeserialize(deserializableTree); }
// ISerializableCollection
    EXT_NODISCARD size_t Size() const EXT_NOEXCEPT override { return Base::GetField().Size(); }
    EXT_NODISCARD std::shared_ptr<ISerializable> Get(const size_t& index) const override { return Base::GetField().Get(index); }
};

// Proxy class for ISerializableField, used for base classes
struct SerializableFieldProxy : ISerializableField
{
    SerializableFieldProxy(ISerializableField* fieldPointer) : m_field(fieldPointer) {}

protected:
// ISerializable
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { EXT_DUMP_IF(!m_field); return m_field ? m_field->GetName() : nullptr; }
    void PrepareToDeserialize(_Inout_ std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override { EXT_EXPECT(m_field); m_field->PrepareToDeserialize(deserializableTree); }
// ISerializableField
    EXT_NODISCARD SerializableValue SerializeValue() const override { EXT_EXPECT(m_field); return m_field->SerializeValue(); }
    void DeserializeValue(const SerializableValue& value) override { EXT_EXPECT(m_field); m_field->DeserializeValue(value); }

    ISerializableField* m_field;
};

// Proxy class for ISerializableField, used for base classes
struct SerializableCollectionProxy : ISerializableCollection
{
    SerializableCollectionProxy(ISerializableCollection* collectionPointer) : m_collection(collectionPointer) {}

protected:
// ISerializable
    EXT_NODISCARD const wchar_t* GetName() const EXT_NOEXCEPT override { EXT_DUMP_IF(!m_collection); return m_collection ? m_collection->GetName() : nullptr; }
    void PrepareToDeserialize(_Inout_ std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override { EXT_EXPECT(m_collection); m_collection->PrepareToDeserialize(deserializableTree); }
// ISerializableCollection
    EXT_NODISCARD size_t Size() const EXT_NOEXCEPT override { EXT_DUMP_IF(!m_collection); return m_collection ? m_collection->Size() : 0; }
    EXT_NODISCARD std::shared_ptr<ISerializable> Get(const size_t& index) const override { EXT_EXPECT(m_collection); return m_collection->Get(index); }

    ISerializableCollection* m_collection;
};

} // namespace impl

#pragma endregion SerializableClasses

} // namespace ext::serializable

#include <ext/serialization/serializer.h>