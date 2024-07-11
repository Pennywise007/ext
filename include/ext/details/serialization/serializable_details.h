#pragma once

#include <algorithm>
#include <list>
#include <type_traits>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <optional>
#include <string>
#include <set>
#include <unordered_set>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/mpl.h>

#include <ext/reflection/object.h>

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
struct is_map<std::unordered_map<_Kty, _Ty, _Pr, _Alloc>> : std::true_type {};
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
template<class _Kty, class _Pr, class _Alloc>
struct is_set<std::unordered_set<_Kty, _Pr, _Alloc>> : std::true_type {};
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
inline constexpr bool is_iserializable_v = is_based_on<ISerializable, T>;
// Check if class inherited from ISerializable or registered in SerializableObjectDescriptor
template<class T>
inline constexpr bool is_serializable_v = is_iserializable_v<T> || is_serializable_object<T>;

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

template <class Type>
void call_on_serialization_start(Type* pointer)
{
    if (pointer == nullptr)
        return;

    using ExtractedType = std::extract_value_type_v<Type>;
    if constexpr (details::is_iserializable_v<ExtractedType>)
    {
        if constexpr (HAS_FUNCTION(ExtractedType, OnSerializationStart))
            pointer->OnSerializationStart();
    }
    else
    {
        static_assert(is_serializable_object<ExtractedType>, "Unknown object to call serialization start");
        ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>().CallOnSerializationStart(pointer);
    }
}

template <class Type>
void call_on_serialization_end(Type *pointer)
{
    if (pointer == nullptr)
        return;

    using ExtractedType = std::extract_value_type_v<Type>;
    if constexpr (details::is_iserializable_v<ExtractedType>)
    {
        if constexpr (HAS_FUNCTION(ExtractedType, OnSerializationEnd))
            pointer->OnSerializationEnd();
    }
    else
    {
        static_assert(is_serializable_object<ExtractedType>, "Unknown object to call serialization end");
        ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>().CallOnSerializationEnd(pointer);
    }
}

template <class Type>
void call_on_deserialization_start(Type *pointer, std::shared_ptr<SerializableNode>& serializableTree)
{
    if (pointer == nullptr)
        return;

    using ExtractedType = std::extract_value_type_v<Type>;
    if constexpr (details::is_iserializable_v<ExtractedType>)
    {
        if constexpr (HAS_FUNCTION(ExtractedType, OnDeserializationStart))
            pointer->OnDeserializationStart(serializableTree);
    }
    else
    {
        static_assert(is_serializable_object<ExtractedType>, "Unknown object to call deserialization start");
        ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>().CallOnDeserializationStart(pointer, serializableTree);
    }
}

template <class Type>
void call_on_deserialization_end(Type *pointer)
{
    if (pointer == nullptr)
        return;

    using ExtractedType = std::extract_value_type_v<Type>;
    if constexpr (details::is_iserializable_v<ExtractedType>)
    {
        if constexpr (HAS_FUNCTION(ExtractedType, OnDeserializationEnd))
            pointer->OnDeserializationEnd();
    }
    else
    {
        static_assert(is_serializable_object<ExtractedType>, "Unknown object to call deserialization end");
        ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>().CallOnDeserializationEnd(pointer);
    }
}

// Wrapper around Type object to convert it to the ISerializableCollection to create ability to iterate over fields
template <class Type>
struct SerializableObject : ISerializableCollection
{
    SerializableObject(Type& object,
                       const std::string& name,
                       const std::list<std::shared_ptr<details::ISerializableFieldInfo>>& fields,
                       const std::list<std::shared_ptr<details::ISerializableBaseInfo>>& baseSerializableClasses)
    : m_object(object)
    , m_name(name)
    , m_fields(fields)
    , m_baseSerializableClasses(baseSerializableClasses)
    {}

private:
// ISerializable
    [[nodiscard]] const char* GetName() const noexcept override { return m_name.c_str(); }
    void OnSerializationStart() override { call_on_serialization_start(&m_object); }
    void OnSerializationEnd() override { call_on_serialization_end(&m_object); }
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& serializableTree) override {
        call_on_deserialization_start(&m_object, serializableTree);
    }
    void OnDeserializationEnd() override { call_on_deserialization_end(&m_object); }

// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override
    {
        size_t result = 0;
        for (const auto& base : m_baseSerializableClasses)
        {
            result += base->CountFields();
        } 
        return result + m_fields.size(); 
    }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& requestedIndex) const override
    {
        size_t index = requestedIndex;
        for (const auto& base : m_baseSerializableClasses)
        {
            auto baseClassFields = base->CountFields();
            if (index < baseClassFields)
                return base->GetField(index, &m_object);
            index -= baseClassFields;
        }

        EXT_ASSERT(index < m_fields.size());
        return std::next(m_fields.begin(), index)->get()->GetField(&m_object);
    }

private:
    Type& m_object;
    const std::string m_name;
    const std::list<std::shared_ptr<details::ISerializableFieldInfo>> m_fields;
    const std::list<std::shared_ptr<details::ISerializableBaseInfo>> m_baseSerializableClasses;
};

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
    void OnSerializationStart() EXT_THROWS() override { EXT_EXPECT(m_field); m_field->OnSerializationStart(); }
    void OnSerializationEnd() EXT_THROWS() override { EXT_EXPECT(m_field); m_field->OnSerializationEnd(); }
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& deserializableTree) override { EXT_EXPECT(m_field); m_field->OnDeserializationStart(deserializableTree); }
    void OnDeserializationEnd() EXT_THROWS() override { EXT_EXPECT(m_field); m_field->OnDeserializationEnd(); }
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
    
    void OnSerializationStart() EXT_THROWS() override { EXT_EXPECT(m_collection); m_collection->OnSerializationStart(); }
    void OnSerializationEnd() EXT_THROWS() override { EXT_EXPECT(m_collection); m_collection->OnSerializationEnd(); }
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& deserializableTree) override { EXT_EXPECT(m_collection); m_collection->OnDeserializationStart(deserializableTree); }
    void OnDeserializationEnd() EXT_THROWS() override { EXT_EXPECT(m_collection); m_collection->OnDeserializationEnd(); }
// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { EXT_DUMP_IF(!m_collection); return m_collection ? m_collection->Size() : 0; }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override { EXT_EXPECT(m_collection); return m_collection->Get(index); }

    const std::optional<std::string> m_name;
    ISerializableCollection* m_collection;
    const std::shared_ptr<ISerializableCollection> m_collectionHolder;
};

template <class Type>
struct SerializableOptional : SerializableBase<ISerializableOptional, Type>
{
    using Base = SerializableBase<ISerializableOptional, Type>;
    SerializableOptional(const std::string& name, Type* typePointer) : Base(name, typePointer) {}
protected:
// ISerializable
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& serializableTree) override
    {
        using ExtractedType = std::extract_value_type_v<Type>;
        Base::GetType()->reset();
        if (serializableTree->HasChilds())
        {
            if constexpr (is_serializable_object<ExtractedType> ||
                is_based_on<ISerializableCollection, ExtractedType>)
                Base::GetType()->emplace(create_default_value<ExtractedType>());
            else if (std::dynamic_pointer_cast<ISerializableCollection>(
                get_as_serializable<ExtractedType>(Base::GetName(), nullptr)))
                Base::GetType()->emplace(create_default_value<ExtractedType>());
            else
                EXT_ASSERT(false);
        }
        else if (serializableTree->Value.value_or(SerializableValue::CreateNull()).Type != SerializableValue::ValueType::eNull)
            Base::GetType()->emplace(create_default_value<ExtractedType>());
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
    void OnSerializationStart() override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
            call_on_serialization_start(Base::GetType()->get());
        else
            call_on_serialization_start(Base::GetType());
    }
    void OnSerializationEnd() override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
            call_on_serialization_end(Base::GetType()->get());
        else
            call_on_serialization_end(Base::GetType());
    }

    void OnDeserializationStart(std::shared_ptr<SerializableNode>& serializableTree) override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
        {
            bool tryCreateDefault = false;
            if constexpr (is_based_on<ISerializableCollection, RealType> || is_serializable_object<RealType>)
                tryCreateDefault = serializableTree->HasChilds();
            else
                tryCreateDefault = serializableTree->Value.value_or(SerializableValue::CreateNull()).Type != SerializableValue::ValueType::eNull;

            if (tryCreateDefault)
            {
                if constexpr (std::is_abstract_v<RealType>)
                    EXT_EXPECT(*Base::GetType()) << "Object " << Base::GetName() << " should be created in constructor!";
                else
                    *Base::GetType() = create_default_value<Type>();

                call_on_deserialization_start(Base::GetType()->get(), serializableTree);
            }
            else
                *Base::GetType() = nullptr;
        }
        else
            call_on_deserialization_start(Base::GetType(), serializableTree);
    }
    void OnDeserializationEnd() override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
            call_on_deserialization_end(Base::GetType()->get());
        else
            call_on_deserialization_end(Base::GetType());
    }

// ISerializableOptional
    [[nodiscard]] std::shared_ptr<ISerializable> Get() const override
    {
        using ExtractedType = std::extract_value_type_v<Type>;
        ExtractedType* pointer = nullptr;
        if constexpr (std::is_same_v<std::unique_ptr<ExtractedType>, Type> ||
                      std::is_same_v<std::shared_ptr<ExtractedType>, Type>)
            pointer = Base::GetType()->get();
        else
            pointer = Base::GetType();
        if (pointer == nullptr)
            return std::make_shared<SerializableValueHolder>(Base::GetName(), SerializableValue::CreateNull());

        if constexpr (is_registered_serializable_object_v<ExtractedType>)
        {
            auto& descriptor = ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>();
            return descriptor.GetSerializable(*pointer, Base::GetName());
        }
        /* Use reinterpret_cast in case of private inheritance, example:
        struct Settings
        {
            struct SubSettings : private ISerializableCollection <--
            {};
            DECLARE_SERIALIZABLE_FIELD(SubSettings, field);
        }
        */
        else if constexpr (std::is_base_of_v<ISerializableCollection, ExtractedType>) 
            return std::make_shared<SerializableProxy<ExtractedType>>(reinterpret_cast<ISerializableCollection*>(pointer), Base::GetName());
        else if constexpr (std::is_base_of_v<ISerializableField, ExtractedType>) 
            return std::make_shared<SerializableProxy<ExtractedType>>(reinterpret_cast<ISerializableField*>(pointer), Base::GetName());
        else
        {
            static_assert(is_serializable_object<ExtractedType>, "Object can be serializable in C++20 but not registered");
            auto& descriptor = ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>();
            return descriptor.GetSerializable(*pointer, Base::GetName());
        }
    }
};

template <class Type>
std::shared_ptr<ISerializable> get_serializable(const std::string& name, const Type* value)
{
    static_assert(is_serializable_v<Type>, "Type is not serializable");

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
    // Called before collection deserialization
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& deserializableTree) override
    {
        MapType* container = Base::GetType();
        container->clear();

        m_serializableValues.clear();
        if constexpr (is_serializable_v<typename MapType::key_type>)
        {
            m_serializableValues.resize(deserializableTree->CountChilds());
        }
        else
        {
            for (size_t i = 0, size = deserializableTree->CountChilds(); i < size; ++i)
            {
                auto child = deserializableTree->GetChild(i);
                EXT_EXPECT(child->Name == "Map pair");

                const auto& keyChild = child->GetChild("Key");
                EXT_ASSERT(keyChild);
                auto emplaceResult = container->emplace(deserialize_value<typename MapType::key_type>(*keyChild->Value), create_default_value<typename MapType::mapped_type>());

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
    }
    // Called after collection deserialization
    void OnDeserializationEnd() override
    {
        if constexpr (is_serializable_v<typename MapType::key_type>)
        {
            for (auto&& [key, value] : m_serializableValues)
            {
                Base::GetType()->emplace(std::move(key), std::move(value));
            }
            m_serializableValues.clear();
        }
    }

// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { return m_serializableValues.size() + Base::GetType()->size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        if constexpr (is_serializable_v<typename MapType::key_type>)
        {
            if (!m_serializableValues.empty())
            {
                if (index >= m_serializableValues.size()) {
                    EXT_DUMP_IF(true) << "Trying to get value during deserialization which is not allocated";
                    return nullptr; 
                }
                
                auto& values = m_serializableValues[index];

                auto collection = std::make_shared<SerializableCollectionImpl>("Map pair");
                collection->AddField(get_serializable("Key", &values.first));
                collection->AddField(get_as_serializable("Value", &const_cast<typename MapType::mapped_type&>(values.second)));

                return collection;
            }
        }

        MapType* container = Base::GetType();
        if (index >= container->size()) { EXT_DUMP_IF(true); return nullptr; }

        auto collection = std::make_shared<SerializableCollectionImpl>("Map pair");

        auto it = std::next(container->begin(), index);
        if constexpr (is_serializable_v<typename MapType::key_type>)
            collection->AddField(get_serializable("Key", &it->first));
        else
            collection->AddField(std::make_shared<SerializableValueHolder>("Key", serialize_value(it->first)));

        collection->AddField(get_as_serializable("Value", &it->second));
        return collection;
    }

private:
    // Array where we store all objects during deserialization of the serializable MapType::key_type
    // We store them here because we can't resize map with default values  
    std::vector<std::pair<typename MapType::key_type, typename MapType::mapped_type>> m_serializableValues;
};

// Special class for serializing/deserializing std::set/std::multiset types
template <class SetType>
struct SerializableSet : SerializableBase<ISerializableCollection, SetType>
{
    using Base = SerializableBase<ISerializableCollection, SetType>;
    SerializableSet(const std::string& name, SetType* typePointer) : Base(name, typePointer) {}
protected:
// ISerializable
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& deserializableTree) override
    {
        SetType* container = Base::GetType();
        container->clear();

        m_serializableValues.clear();
        if constexpr (is_serializable_v<typename SetType::key_type>)
        {
            m_serializableValues.resize(deserializableTree->CountChilds());
        }
        else
        {
            for (size_t i = 0, size = deserializableTree->CountChilds(); i < size; ++i)
            {
                auto child = deserializableTree->GetChild(i);
                EXT_EXPECT(child->Name == "Value") << "Expect set node name Value";
                EXT_EXPECT(child->Value.has_value()) << "Expect set node with value not empty";
                container->emplace(deserialize_value<typename SetType::key_type>(*child->Value));
            }
        }
    }
    // Called after collection deserialization
    void OnDeserializationEnd() override
    {
        if constexpr (is_serializable_v<typename SetType::key_type>)
        {
            for (const auto& deserializedValue : m_serializableValues)
            {
                Base::GetType()->emplace(std::move(deserializedValue));
            }
            m_serializableValues.clear();
        }
    }
// ISerializableCollection
    [[nodiscard]] size_t Size() const noexcept override { return m_serializableValues.size() + Base::GetType()->size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) const override
    {
        if constexpr (is_serializable_v<typename SetType::key_type>)
        {
            if (!m_serializableValues.empty())
            {
                if (index >= m_serializableValues.size()) {
                    EXT_DUMP_IF(true) << "Trying to get value during deserialization which is not allocated";
                    return nullptr; 
                }
                return get_serializable("Value", &m_serializableValues[index]);
            }
        }

        SetType* container = Base::GetType();
        if (index >= container->size()) { EXT_DUMP_IF(true); return nullptr; }

        auto it = std::next(container->begin(), index);
        if constexpr (is_serializable_v<typename SetType::key_type>)
            return get_serializable("Value", &*it);
        else
            // if set types if not serializable - store string value presentation as index name with empty value
            return std::make_shared<SerializableValueHolder>("Value", serialize_value(*it));
    }
private:
    // Array where we store all objects during deserialization of the serializable SetType::key_type
    // We store them here because we can't resize set with default values
    std::vector<typename SetType::key_type> m_serializableValues;
};

// Base class for collection of objects std::vector/list etc.
template <typename ArrayType>
struct SerializableArray : SerializableBase<ISerializableCollection, ArrayType>
{
    using Base = SerializableBase<ISerializableCollection, ArrayType>;
    SerializableArray(const std::string& name, ArrayType* typePointer) : Base(name, typePointer) {}
protected:
// ISerializable
    void OnDeserializationStart(std::shared_ptr<SerializableNode>& deserializableTree) override
    {
        ArrayType* array = Base::GetType();
        array->clear();
        if constexpr (HAS_FUNCTION(ArrayType, reserve))
            array->reserve(deserializableTree->CountChilds());
        for (auto count = deserializableTree->CountChilds(); count != 0; --count)
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
        return collection;
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
    [[nodiscard]] std::shared_ptr<ISerializable> GetField(void* objectPointer) const override
    {
        Type* typePointer = reinterpret_cast<Type*>(objectPointer);
        EXT_EXPECT(typePointer) << "Can`t get type " << ext::type_name<Type>();

        std::string trimName = m_name;
        std::string_trim_all(trimName);
        EXT_ASSERT(!trimName.empty());

        return get_as_serializable<Field>(trimName, &((*typePointer).*m_field));
    }

protected:
    const char* m_name;
    Field Type::* m_field;
};

template <class Type, class BaseType>
struct SerializableBaseInfo : ISerializableBaseInfo
{
    static_assert(is_based_on<BaseType, Type>);
    static_assert(is_serializable_v<BaseType>);

protected:
// ISerializableBaseInfo
    [[nodiscard]] size_t CountFields() const override
    {
        if constexpr (is_registered_serializable_object_v<BaseType>)
            return ext::get_singleton<SerializableObjectDescriptor<BaseType>>().GetFieldsCount();
        else if constexpr (is_iserializable_v<BaseType>)
            return 1;
        else
        {
            static_assert(is_serializable_object<BaseType>, "Object can be serializable in C++20");
            return ext::get_singleton<SerializableObjectDescriptor<BaseType>>().GetFieldsCount();
        }
    }

    [[nodiscard]] std::shared_ptr<ISerializable> GetField(const size_t& index, void* objectPointer) const override
    {
        EXT_ASSERT(index < CountFields());
        BaseType* object = ext::get_singleton<SerializableObjectDescriptor<Type>>().
            template ConvertToType<BaseType>(reinterpret_cast<Type*>(objectPointer));

        const SerializableObjectDescriptor<BaseType>& baseDescriptor = ext::get_singleton<SerializableObjectDescriptor<BaseType>>();
        if constexpr (is_registered_serializable_object_v<BaseType>)
            return baseDescriptor.GetSerializable(*object)->Get(index);
        else if constexpr (is_iserializable_v<BaseType>)
        {
            auto* ser = baseDescriptor.template ConvertToType<ext::serializable::ISerializable>(object);
            return get_as_serializable<BaseType>(ser->GetName(), object);
        }
        else
        {
            static_assert(is_serializable_object<BaseType>, "Object can be serializable in C++20");
            return baseDescriptor.GetSerializable(*object)->Get(index);
        }
    }
};

} // namespace details

template<class Type>
template <class Field>
void SerializableObjectDescriptor<Type>::RegisterField(const char* name, Field Type::* field)
{
    m_fields.emplace_back(std::make_shared<details::SerializableFieldInfo<Type, Field>>(name, field));
}

template<class Type>
template<class ...Classes>
void SerializableObjectDescriptor<Type>::RegisterSerializableBaseClasses()
{
    ext::mpl::list<Classes...>::ForEachType([&](auto* type)
    {
        using BaseType = std::remove_pointer_t<decltype(type)>;

        static_assert(!std::is_same_v<BaseType, Type>, "Trying to register itself");
        static_assert(details::is_based_on<BaseType, Type>, "Trying to register not a base class");
        static_assert(details::is_serializable_v<BaseType>, "Non serializable base type, did you register it with REGISTER_SERIALIZABLE_OBJECT?");

        m_baseSerializableClasses.emplace_back(std::make_shared<details::SerializableBaseInfo<Type, BaseType>>());
    });
}

template<class Type>
[[nodiscard]] size_t SerializableObjectDescriptor<Type>::GetFieldsCount() const
{
    static const size_t fieldsCount = [&]() {
        size_t result = 0;
        for (const auto& base : m_baseSerializableClasses)
        {
            result += base->CountFields();
        }

        if constexpr (!is_registered_serializable_object_v<Type>)
            return result + ext::reflection::fields_count<Type>;
        else
            return result + m_fields.size();
    }();
    return fieldsCount;
}

template<class Type>
[[nodiscard]] std::shared_ptr<ISerializableCollection> SerializableObjectDescriptor<Type>::GetSerializable(Type& object, const char* customName) const
{
    if (customName == nullptr)
        customName = m_name;

    const char* collectionName = customName != nullptr ? customName : ext::type_name<Type>();

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20
    if constexpr (!is_registered_serializable_object_v<Type>)
    {
        static_assert(ext::reflection::fields_count<Type> > 0, "No fields in object for serialization");

        auto fields = ext::reflection::get_object_fields(object);
        
        auto fieldsCollection = std::make_shared<details::SerializableCollectionImpl>(collectionName);
        const auto registerField = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (([&] {
                fieldsCollection->AddField(details::get_as_serializable(
                    std::string(ext::reflection::field_name<Type, Is>),
                    &std::get<Is>(fields)));
            }()), ...);
        };
        registerField(std::make_index_sequence<std::tuple_size_v<decltype(fields)>>());

        return fieldsCollection;
    }
    else
#endif // C++20
    {
        EXT_ASSERT(!m_fields.empty()) << "Object " << ext::type_name<Type>() << "doesn't have any registered fields. Did you forget to register them?";
        return std::make_shared<details::SerializableObject<Type>>(
            object, collectionName, m_fields, m_baseSerializableClasses);
    }
}

template<class Type>
template<class ConvertedType>
[[nodiscard]] ConvertedType* SerializableObjectDescriptor<Type>::ConvertToType(Type* object) const
{
    return static_cast<ConvertedType*>(object);
}

template <class Type>
inline void SerializableObjectDescriptor<Type>::CallOnSerializationStart(Type* pointer) const
{
    if constexpr (HAS_FUNCTION(Type, OnSerializationStart))
        pointer->OnSerializationStart();
}

template <class Type>
inline void SerializableObjectDescriptor<Type>::CallOnSerializationEnd(Type* pointer) const
{
    if constexpr (HAS_FUNCTION(Type, OnSerializationEnd))
        pointer->OnSerializationEnd();
}

template <class Type>
inline void SerializableObjectDescriptor<Type>::CallOnDeserializationStart(Type* pointer, 
    std::shared_ptr<SerializableNode>& serializableTree) const
{
    if constexpr (HAS_FUNCTION(Type, OnDeserializationStart))
        pointer->OnDeserializationStart(serializableTree);
}

template <class Type>
inline void SerializableObjectDescriptor<Type>::CallOnDeserializationEnd(Type* pointer) const
{
    if constexpr (HAS_FUNCTION(Type, OnDeserializationEnd))
        pointer->OnDeserializationEnd();
}

} // namespace ext::serializable
