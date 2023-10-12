#pragma once

#include <algorithm>
#include <list>
#include <type_traits>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <set>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/mpl.h>

#include <ext/serialization/iserializable.h>

#include <ext/std/type_traits.h>
#include <ext/std/string.h>

#include <ext/types/utils.h>

namespace ext::serializable {
namespace details {

// Check is type a std::map/multimap container
template <class T>
struct is_map : std::false_type {};
template<class _Kty, class _Ty, class _Pr, class _Alloc>
struct is_map<std::map<_Kty, _Ty, _Pr, _Alloc>> : std::true_type {};
template<class _Kty, class _Ty, class _Pr, class _Alloc>
struct is_map<std::multimap<_Kty, _Ty, _Pr, _Alloc>> : std::true_type {};
template<class T>
inline constexpr bool is_map_v = is_map<T>::value;

// Check is type a std::set/multiset container
template <class T>
struct is_set : std::false_type {};
template<class _Kty, class _Pr, class _Alloc>
struct is_set<std::set<_Kty, _Pr, _Alloc>> : std::true_type {};
template<class _Kty, class _Pr, class _Alloc>
struct is_set<std::multiset<_Kty, _Pr, _Alloc>> : std::true_type {};
template<class T>
inline constexpr bool is_set_v = is_set<T>::value;

// Check is type a std::set/multiset container
template <class T>
struct is_pair : std::false_type {};
template<class First, class Second>
struct is_pair<std::pair<First, Second>> : std::true_type {};
template<class T>
inline constexpr bool is_pair_v = is_pair<T>::value;

// Check is type a std::vector/list container
template <class T>
struct is_array : std::false_type {};
template<class Type>
struct is_array<std::vector<Type>> : std::true_type {};
template<class Type>
struct is_array<std::list<Type>> : std::true_type {};
template<class T>
inline constexpr bool is_array_v = is_array<T>::value;

// 1. First, we isolate the type (if a template was passed) std::shared_ptr<int> => int
// 2. Remove the pointer if it is a pointer
// 3. Check what is inherited from Base
template<class Base, class T>
inline constexpr bool is_based_on = std::is_base_of_v<Base, std::remove_pointer_t<std::extract_value_type_v<T>>>;
template<class T>
inline constexpr bool is_serializable_v = is_based_on<ISerializable, T>;

// declare check reserve and get member function
DECLARE_CHECK_FUNCTION(reserve);

template <class Type>
std::shared_ptr<ISerializable> get_as_serializable(const std::string& name, Type* type);

template <typename ValueType>
ValueType create_default_value()
{
    if constexpr (std::is_same_v<std::shared_ptr<std::extract_value_type_v<ValueType>>, ValueType>)
        return std::make_shared<std::extract_value_type_v<ValueType>>();
    else if constexpr (std::is_same_v<std::unique_ptr<std::extract_value_type_v<ValueType>>, ValueType>)
        return std::make_unique<std::extract_value_type_v<ValueType>>();
    else
    {
        static_assert(!std::is_pointer_v<ValueType>, "Can`t create default value for ordinary pointer!");
        return {};
    }
}

// Base class for hold info about serializable field
template <typename ISerializableType, typename Type>
struct SerializableBase : ISerializableType
{
    SerializableBase(const std::string& name, Type* typePointer) : m_name(name), m_typePointer(typePointer) {}
    [[nodiscard]] Type* GetType() const { return m_typePointer; }

// ISerializable
protected:
    [[nodiscard]] virtual const char* GetName() const noexcept override { return m_name.c_str(); }

private:
    const std::string m_name;
    Type* m_typePointer;
};

// special class for holding SerializableValue
struct SerializableValueHolder : ISerializableField
{
    SerializableValueHolder(const std::string& name, SerializableValue&& _value) : m_name(name), value(std::move(_value)) {}
// ISerializable
    [[nodiscard]] const char* GetName() const noexcept override { return m_name.c_str(); }
// ISerializableField
    [[nodiscard]] SerializableValue SerializeValue() const override { return value; }
    void DeserializeValue(const SerializableValue& /*value*/) override { }

    const std::string m_name;
    const SerializableValue value;
};

template <class Type, class = std::void_t<>>
struct SerializableProxy;

// Proxy class for ISerializableField, used for base classes
template <class Type>
struct SerializableProxy<Type, std::enable_if_t<is_based_on<ISerializableField, Type>>> : ISerializableField
{
    SerializableProxy(ISerializableField* fieldPointer, std::optional<std::string> name = std::nullopt)
        : m_name(std::move(name)), m_field(fieldPointer) {}

protected:
// ISerializable
    [[nodiscard]] const char* GetName() const noexcept override
    {
        if (m_name.has_value()) return m_name->c_str();
        EXT_DUMP_IF(!m_field);
        return m_field ? m_field->GetName() : nullptr;
    }
    void PrepareToDeserialize(std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override { EXT_EXPECT(m_field); m_field->PrepareToDeserialize(deserializableTree); }
// ISerializableField
    [[nodiscard]] SerializableValue SerializeValue() const override { EXT_EXPECT(m_field); return m_field->SerializeValue(); }
    void DeserializeValue(const SerializableValue& value) override { EXT_EXPECT(m_field); m_field->DeserializeValue(value); }

    const std::optional<std::string> m_name;
    ISerializableField* m_field;
};

// Proxy class for ISerializableCollection, used for base classes
template <class Type>
struct SerializableProxy<Type, std::enable_if_t<is_based_on<ISerializableCollection, Type>>> : ISerializableCollection
{
    SerializableProxy(ISerializableCollection* collectionPointer, std::optional<std::string> name = std::nullopt)
       : m_name(std::move(name)), m_collection(collectionPointer) {}

protected:
// ISerializable
    [[nodiscard]] const char* GetName() const noexcept override
    {
        if (m_name.has_value()) return m_name->c_str();
        EXT_DUMP_IF(!m_collection);
        return m_collection ? m_collection->GetName() : nullptr;
    }
    void PrepareToDeserialize(std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override { EXT_EXPECT(m_collection); m_collection->PrepareToDeserialize(deserializableTree); }
// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { EXT_DUMP_IF(!m_collection); return m_collection ? m_collection->Size() : 0; }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override { EXT_EXPECT(m_collection); return m_collection->Get(index); }

    const std::optional<std::string> m_name;
    ISerializableCollection* m_collection;
};

template <class Type>
struct SerializableOptional : SerializableBase<ISerializableOptional, Type>
{
    using Base = SerializableBase<ISerializableOptional, Type>;
    SerializableOptional(const std::string& name, Type* typePointer) : Base(name, typePointer) {}
protected:
// ISerializable
    void PrepareToDeserialize(std::shared_ptr<SerializableNode>& serializableTree) EXT_THROWS() override
    {
        Base::GetType()->reset();
        if (!serializableTree->ChildNodes.empty())
        {
            if (is_based_on<ISerializableCollection, std::extract_value_type_v<Type>> ||
                std::dynamic_pointer_cast<ISerializableCollection>(get_as_serializable<std::extract_value_type_v<Type>>(Base::GetName(), nullptr)))
                Base::GetType()->emplace(create_default_value<std::extract_value_type_v<Type>>());
            else
                EXT_ASSERT(false);
        }
        else if (serializableTree->Value.value_or(SerializableValue::CreateNull()).Type != SerializableValue::ValueType::eNull)
            Base::GetType()->emplace(create_default_value<std::extract_value_type_v<Type>>());
    }
// ISerializableOptional
    [[nodiscard]] std::shared_ptr<ISerializable> Get() const override
    {
        if (!Base::GetType()->has_value())
            return std::make_shared<SerializableValueHolder>(Base::GetName(), SerializableValue::CreateNull());
        return get_as_serializable(Base::GetName(), &Base::GetType()->value());
    }
};

template <class Type>
struct SerializableValueProxy : SerializableBase<ISerializableOptional, Type>
{
    using Base = SerializableBase<ISerializableOptional, Type>;
    SerializableValueProxy(const std::string& name, Type* typePointer) : Base(name, typePointer) {}

protected:
// ISerializable
    void PrepareToDeserialize(std::shared_ptr<SerializableNode>& serializableTree) EXT_THROWS() override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> ||
                      std::is_same_v<std::shared_ptr<RealType>, Type>)
        {
            if constexpr (is_based_on<ISerializableCollection, Type>)
            {
                if (!serializableTree->ChildNodes.empty())
                {
                    if constexpr (std::is_abstract_v<RealType>)
                        EXT_EXPECT(*Base::GetType()) << "Object should be created in constructor!";
                    else
                        *Base::GetType() = create_default_value<Type>();
                    Base::GetType()->get()->PrepareToDeserialize(serializableTree);
                }
                else
                    *Base::GetType() = nullptr;

            }
            else if (serializableTree->Value.value_or(SerializableValue::CreateNull()).Type != SerializableValue::ValueType::eNull)
            {
                if constexpr (std::is_abstract_v<RealType>)
                    EXT_EXPECT(*Base::GetType()) << "Object should be created in constructor!";
                else
                    *Base::GetType() = create_default_value<Type>();
                Base::GetType()->get()->PrepareToDeserialize(serializableTree);
            }
            else
                *Base::GetType() = nullptr;
        }
    }
// ISerializableOptional
    [[nodiscard]] std::shared_ptr<ISerializable> Get() const override
    {
        std::extract_value_type_v<Type>* pointer = nullptr;
        if constexpr (std::is_same_v<std::unique_ptr<std::extract_value_type_v<Type>>, Type> ||
                      std::is_same_v<std::shared_ptr<std::extract_value_type_v<Type>>, Type>)
            pointer = Base::GetType()->get();
        else
            pointer = Base::GetType();
        if (pointer == nullptr)
            return std::make_shared<SerializableValueHolder>(Base::GetName(), SerializableValue::CreateNull());
        return std::make_shared<SerializableProxy<std::extract_value_type_v<Type>>>(pointer, Base::GetName());
    }
};

template <class Type>
std::shared_ptr<ISerializable> get_serializable(const std::string& name, const Type* value)
{
    static_assert(is_serializable_v<Type>, "Type is not serializable!");
    return std::make_shared<SerializableValueProxy<Type>>(name, const_cast<Type*>(value));
}

struct SerializableCollectionImpl : ISerializableCollection
{
    SerializableCollectionImpl(std::string name) : m_name(std::move(name)) {}

    void AddField(std::shared_ptr<ISerializable>&& serializable) { m_fields.emplace_back(std::move(serializable)); }
public:
// ISerializable
    [[nodiscard]] const char* GetName() const noexcept override { return m_name.c_str(); }
// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { return m_fields.size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        if (index >= m_fields.size()) { EXT_DUMP_IF(true); return nullptr; }
        return *std::next(m_fields.begin(), index);
    }
protected:
    const std::string m_name;
    std::list<std::shared_ptr<ISerializable>> m_fields;
};

// Special class for serializing/deserializing std::map/std::multimap types
template <class MapType>
struct SerializableMap : SerializableBase<ISerializableCollection, MapType>
{
    using Base = SerializableBase<ISerializableCollection, MapType>;
    SerializableMap(const std::string& name, MapType* typePointer) : Base(name, typePointer) {}
protected:
// ISerializable
    void PrepareToDeserialize(std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override
    {
        MapType* container = Base::GetType();
        container->clear();
        for (const auto& child : deserializableTree->ChildNodes)
        {
            EXT_EXPECT(child->Name == "Map pair");

            decltype(Base::GetType()->emplace(typename MapType::value_type())) emplaceResult;
            if constexpr (is_serializable_v<typename MapType::key_type>)
                emplaceResult = container->emplace(create_default_value<typename MapType::key_type>(), create_default_value<typename MapType::mapped_type>());
            else
            {
                const auto& keyChild = child->GetChild("Key");
                EXT_ASSERT(keyChild);
                emplaceResult = container->emplace(deserialize_value<typename MapType::key_type>(*keyChild->Value), create_default_value<typename MapType::mapped_type>());
            }

            typename MapType::iterator insertedIter;
            if constexpr (std::is_same_v<decltype(emplaceResult), typename MapType::iterator>) // std:::map and multimap emplace got different result type
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

            if constexpr (!is_serializable_v<typename MapType::mapped_type>)
            {
                const auto& valueChild = child->GetChild("Value");
                EXT_EXPECT(valueChild);
                EXT_EXPECT(valueChild->Value.has_value());
                insertedIter->second = deserialize_value<typename MapType::mapped_type>(*valueChild->Value);
            }
        }
    }

// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { return Base::GetType()->size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        MapType* container = Base::GetType();
        if (index >= container->size()) { EXT_DUMP_IF(true); return nullptr; }

        auto collection = std::make_shared<SerializableCollectionImpl>("Map pair");

        auto it = std::next(container->begin(), index);
        if constexpr (is_serializable_v<typename MapType::key_type>)
            collection->AddField(get_serializable("Key", &it->first));
        else
            collection->AddField(std::make_shared<SerializableValueHolder>("Key", serialize_value(it->first)));

        collection->AddField(get_as_serializable("Value", &it->second));
        return std::move(collection);
    }
};

// Special class for serializing/deserializing std::set/std::multiset types
template <class SetType>
struct SerializableSet : SerializableBase<ISerializableCollection, SetType>
{
    using Base = SerializableBase<ISerializableCollection, SetType>;
    SerializableSet(const std::string& name, SetType* typePointer) : Base(name, typePointer) {}
protected:
// ISerializable
    void PrepareToDeserialize(std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override
    {
        SetType* container = Base::GetType();
        container->clear();
        for (const auto& child : deserializableTree->ChildNodes)
        {
            if constexpr (is_serializable_v<typename SetType::value_type>)
                container->emplace(create_default_value<typename SetType::value_type>());
            else
            {
                EXT_EXPECT(child->Name == "Value") << "Expect set node name Value";
                EXT_EXPECT(child->Value.has_value()) << "Expect set node with value not empty";
                container->emplace(deserialize_value<typename SetType::key_type>(*child->Value));
            }
        }
    }
// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { return Base::GetType()->size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        SetType* container = Base::GetType();
        if (index >= container->size()) { EXT_DUMP_IF(true); return nullptr; }

        auto it = std::next(container->begin(), index);
        if constexpr (is_serializable_v<typename SetType::value_type>)
            return get_serializable("Value", &*it);
        else
            // if set types if not serializable - store string value presentation as index name with empty value
            return std::make_shared<SerializableValueHolder>("Value", serialize_value(*it));
    }
};

// Base class for collection of objects std::vector/list etc.
template <typename ArrayType>
struct SerializableArray : SerializableBase<ISerializableCollection, ArrayType>
{
    using Base = SerializableBase<ISerializableCollection, ArrayType>;
    SerializableArray(const std::string& name, ArrayType* typePointer) : Base(name, typePointer) {}
protected:
// ISerializable
    void PrepareToDeserialize(std::shared_ptr<SerializableNode>& deserializableTree) EXT_THROWS() override
    {
        ArrayType* array = Base::GetType();
        array->clear();
        if constexpr (has_reserve_function_v<ArrayType>)
            array->reserve(deserializableTree->ChildNodes.size());
        for (auto count = deserializableTree->ChildNodes.size(); count != 0; --count)
            array->emplace_back(create_default_value<typename ArrayType::value_type>());
    }
// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { return Base::GetType()->size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        ArrayType* array = Base::GetType();
        if (index >= array->size()) { EXT_DUMP_IF(true); return nullptr; }
        return get_as_serializable<typename ArrayType::value_type>(std::to_string(index), &*std::next(array->begin(), index));
    }
};

// Base class for serializable fields
template <class FieldType>
struct SerializableField : SerializableBase<ISerializableField, FieldType>
{
    using Base = SerializableBase<ISerializableField, FieldType>;
    SerializableField(const std::string& name, FieldType* typePointer) : Base(name, typePointer) {}

protected:
// ISerializableField
    [[nodiscard]] SerializableValue SerializeValue() const override { return serialize_value<FieldType>(*Base::GetType()); }
    void DeserializeValue(const SerializableValue& value) override { *Base::GetType() = deserialize_value<FieldType>(value); }
};

template <class Type>
std::shared_ptr<ISerializable> get_as_serializable(const std::string& name, Type* type)
{
    if constexpr (std::is_same_v<std::optional<std::extract_value_type_v<Type>>, Type>)
        return std::make_shared<details::SerializableOptional<Type>>(name, type);
    else if constexpr (details::is_map_v<Type>)
        return std::make_shared<details::SerializableMap<Type>>(name, type);
    else if constexpr (details::is_set_v<Type>)
        return std::make_shared<details::SerializableSet<Type>>(name, type);
    else if constexpr (details::is_pair_v<Type>)
    {
        auto collection = std::make_shared<SerializableCollectionImpl>(name);
        collection->AddField(get_as_serializable<decltype(Type::first)>("First", &type->first));
        collection->AddField(get_as_serializable<decltype(Type::second)>("Second", &type->second));
        return std::move(collection);
    }
    else if constexpr (details::is_array_v<Type>)
        return std::make_shared<details::SerializableArray<Type>>(name, type);
    else if constexpr (is_serializable_v<Type>)
        return get_serializable(name, type);
    else
        return std::make_shared<details::SerializableField<Type>>(name, type);
}

template <class Type, typename Field>
struct SerializableFieldInfo : ISerializableFieldInfo
{
    SerializableFieldInfo(const char* name, Field Type::* field) : m_name(name), m_field(field) {}

protected:
// ISerializableFieldInfo
    [[nodiscard]] std::shared_ptr<ISerializable> GetField(const ISerializable* object) const override
    {
        Type* typePointer = const_cast<Type*>(reinterpret_cast<const Type*>(object));
        EXT_ASSERT(typePointer) << "Cant get type " << ext::type_name<Type>() << " from object, maybe virtual table is missing, remove ATL_NO_VTABLE." 
            << " Or private inheritance problem.";

        std::string trimName = m_name;
        std::string_trim_all(trimName);
        EXT_ASSERT(!trimName.empty());

        return get_as_serializable<Field>(trimName, &((*typePointer).*m_field));
    }

protected:
    const char* m_name;
    Field Type::* m_field;
};

} // namespace details

template<class Type, const char* TypeName, class ICollectionInterface>
template <class Field>
void SerializableObject<Type, TypeName, ICollectionInterface>::RegisterField(const char* name, Field Type::* field)
{
    m_fields.emplace_back(std::make_shared<details::SerializableFieldInfo<Type, Field>>(name, field));
}

template<class Type, const char* TypeName, class ICollectionInterface>
template<class ...Classes>
void SerializableObject<Type, TypeName, ICollectionInterface>::RegisterSerializableBaseClasses()
{
    Type* currentClass = dynamic_cast<Type*>(this);
    EXT_ASSERT(currentClass);

    ext::mpl::list<Classes...>::ForEachType([&](auto* type)
    {
        using BaseType = std::remove_pointer_t<decltype(type)>;

        auto* base = static_cast<BaseType*>(currentClass);
        m_baseSerializableClasses.emplace_back(std::make_shared<details::SerializableProxy<BaseType>>(base));
    });
}

} // namespace ext::serializable
