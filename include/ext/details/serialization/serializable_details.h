#pragma once
// TODO rework type pointers to references

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

namespace ext::serializer {

template <class Type>
void SerializeKey(const Type& object, std::wstring& text) EXT_THROWS();

// Serialize object to the json string
template <class Type>
void DeserializeKey(Type& object, std::wstring& text) EXT_THROWS();

} // namespace ext::serializer

namespace ext::serializable::details {

template <class Type>
void call_on_serialization_start(Type* pointer);
template <class Type>
void call_on_serialization_end(Type *pointer);
template <class Type>
void call_on_deserialization_start(Type *pointer, SerializableNode& serializableTree);
template <class Type>
void call_on_deserialization_end(Type *pointer);

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
// Check if class inherited from ISerializableArray or ISerializableObject
template<class T>
inline constexpr bool is_iserializable_collection_v = is_based_on<ISerializableArray, T> || is_based_on<ISerializableObject, T>;
// Check if class inherited from ISerializable or registered in SerializableObjectDescriptor
template<class T>
inline constexpr bool is_serializable_v = is_iserializable_v<T> || is_serializable_object<T>;

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
std::shared_ptr<ISerializable> get_serializable(const Type* value);

template <class Type>
std::shared_ptr<ISerializable> get_as_serializable(Type* type);

// special class for holding SerializableValue
struct SerializableValueHolder : ISerializableValue
{
    SerializableValueHolder(SerializableValue&& _value) : value(std::move(_value)) {}
// ISerializableValue
    [[nodiscard]] SerializableValue SerializeValue() const override { return value; }
    void DeserializeValue(const SerializableValue& /*value*/) override { }

    const SerializableValue value;
};

// Base class for serializable fields
struct SerializableFieldImpl : ISerializableField
{
    SerializableFieldImpl(std::string name, std::shared_ptr<ISerializable> field) : m_name(std::move(name)), m_field(std::move(field)) {}

protected:
// ISerializableField
    [[nodiscard]] SerializableValue GetName() const override { return serialize_value(m_name); }
    [[nodiscard]] std::shared_ptr<ISerializable> GetField() override { return m_field; }

private:
    const std::string m_name;
    const std::shared_ptr<ISerializable> m_field;
};

struct SerializableObjectImpl : ISerializableObject
{
    SerializableObjectImpl(const char* name) : m_name(name) {}
    template <class Type>
    void AddField(std::string name, Type* field)
    {
        m_fields.emplace_back(std::make_shared<SerializableFieldImpl>(std::move(name), get_as_serializable<Type>(field)));
    }
public:
// ISerializableObject
    [[nodiscard]] virtual const char* ObjectName() const noexcept override { return m_name; }
    [[nodiscard]] size_t Size() const noexcept override { return m_fields.size(); }
    [[nodiscard]] std::shared_ptr<ISerializableField> Get(const size_t& index) override
    {
        EXT_EXPECT(index < m_fields.size());
        return *std::next(m_fields.begin(), index);
    }
protected:
    const char* m_name;
    std::list<std::shared_ptr<ISerializableField>> m_fields;
};

// Base class for serializable values
template <class FieldType>
struct SerializableValueImpl : ISerializableValue
{
    SerializableValueImpl(FieldType* typePointer) : m_typePointer(typePointer) {}

protected:
// ISerializableValue
    [[nodiscard]] SerializableValue SerializeValue() const override { return serialize_value<FieldType>(*m_typePointer); }
    void DeserializeValue(const SerializableValue& value) override { *m_typePointer = deserialize_value<FieldType>(value); }

private:
    FieldType* m_typePointer;
};

// Base class for collection of objects std::vector/list etc.
template <typename ArrayType>
struct SerializableArrayImpl : ISerializableArray
{
    SerializableArrayImpl(ArrayType* typePointer) : m_typePointer(typePointer) {}
protected:
// ISerializable
    void OnDeserializationStart(SerializableNode& deserializableTree) override
    {
        ArrayType* array = m_typePointer;
        array->clear();
        if constexpr (HAS_FUNCTION(ArrayType, reserve))
            array->reserve(deserializableTree.CountChilds());
        for (auto count = deserializableTree.CountChilds(); count != 0; --count)
            array->emplace_back(create_default_value<typename ArrayType::value_type>());
    }
// ISerializableArray
    [[nodiscard]] size_t Size() const noexcept override { return m_typePointer->size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) override
    {
        ArrayType* array = m_typePointer;
        if (index >= array->size()) { EXT_DUMP_IF(true); return nullptr; }
        return get_as_serializable<typename ArrayType::value_type>(&*std::next(array->begin(), index));
    }
    ArrayType* m_typePointer;
};

template <class Type>
struct SerializableOptionalImpl : ISerializableOptional
{
    SerializableOptionalImpl(Type* typePointer) : m_typePointer(typePointer) {}
protected:
// ISerializable
    void OnDeserializationStart(SerializableNode& serializableTree) override
    {
        using ExtractedType = std::extract_value_type_v<Type>;
        m_typePointer->reset();
        if (serializableTree.CountChilds() != 0)
        {
            if constexpr (is_serializable_object<ExtractedType> || is_iserializable_collection_v<ExtractedType>)
                m_typePointer->emplace(create_default_value<ExtractedType>());
            else
            {
                // It might be a std::vector or sth like that which we will wrap in ISerializable object
                auto serializableWrappedObject = get_as_serializable<ExtractedType>(nullptr);
                if (std::dynamic_pointer_cast<ISerializableArray>(serializableWrappedObject) ||
                    std::dynamic_pointer_cast<ISerializableObject>(serializableWrappedObject))
                    m_typePointer->emplace(create_default_value<ExtractedType>());
                else
                    EXT_ASSERT(false) << "Type " << ext::type_name<Type>() << " should be an array/object";
            }
        }
        else if (serializableTree.Value.value_or(SerializableValue::CreateNull()).Type != SerializableValue::ValueType::eNull)
            m_typePointer->emplace(create_default_value<ExtractedType>());
    }

// ISerializableOptional
    [[nodiscard]] std::shared_ptr<ISerializable> Get() const override
    {
        if (!m_typePointer->has_value())
            return std::make_shared<SerializableValueHolder>(SerializableValue::CreateNull());
        return get_as_serializable(&m_typePointer->value());
    }

private:
    Type* m_typePointer;
};

// Serializable key value pair, used during map serialization
template <class KeyType, class ValueType>
struct SerializableKeyValuePair : ISerializableField
{
    SerializableKeyValuePair(const KeyType& key, const ValueType& value) : m_key(key), m_value(value) {}

public:
    // ISerializableField
    [[nodiscard]] SerializableValue GetName() const override
    {
#ifdef DEBUG
        EXT_ASSERT(!m_nameRequested) << "Expected to receive GetName call only once";
        m_nameRequested = true;
#endif // DEBUG

        if constexpr (is_serializable_v<KeyType>)
        {
            // serialize serializable object to the text
            using RealKeyType = std::extract_value_type_v<KeyType>;
            const RealKeyType* keyPointer = nullptr;

            if constexpr (std::is_same_v<std::unique_ptr<RealKeyType>, KeyType> || std::is_same_v<std::shared_ptr<RealKeyType>, KeyType>)
            {
                if (!!m_key)
                    keyPointer = m_key.get();
            }
            else
                keyPointer = &m_key;

            if (keyPointer)
            {
                try
                {
                    std::wstring json;
                    ext::serializer::SerializeKey(*keyPointer, json);
                    return json;
                }
                catch (...)
                {
                    std::throw_with_nested(ext::exception(std::source_location::current(),
                        (std::string("Can't serialize key with type ") + ext::type_name<KeyType>()).c_str()));
                }
            }
            else
                return SerializableValue::CreateNull();
        }
        else
            return serialize_value(m_key);
    }
    [[nodiscard]] std::shared_ptr<ISerializable> GetField() override { return get_as_serializable<ValueType>(const_cast<ValueType*>(&m_value)); }

    // ISerializable
    void OnDeserializationStart(SerializableNode& /*serializableTree*/) override { EXT_EXPECT(false) << "Unexpected call"; }
    void OnDeserializationEnd() override { EXT_EXPECT(false) << "Unexpected call"; }

public:
#ifdef DEBUG
    bool m_nameRequested = false;
#endif // DEBUG
    const KeyType& m_key;
    const ValueType& m_value;
};

// Deserializable key value pair, used during map deserialization
template <class KeyType, class ValueType>
struct DeserializableKeyValuePair : ISerializableField
{
    DeserializableKeyValuePair(SerializableNode& deserializableTree)
        : m_name(deserializableTree.Name.value())
    {
        EXT_EXPECT(deserializableTree.Name.has_value()) << "No value for map key " << ext::type_name<KeyType>();

        if constexpr (is_serializable_v<KeyType>)
        {
            using RealType = std::extract_value_type_v<KeyType>;
            RealType* keyPointer = nullptr;
            if constexpr (std::is_same_v<std::unique_ptr<RealType>, KeyType> || std::is_same_v<std::shared_ptr<RealType>, KeyType>)
            {
                m_key = create_default_value<KeyType>();
                keyPointer = m_key.get();
            }
            else
                keyPointer = &m_key;

            // deserialize serializable object from the text
            try
            {
                ext::serializer::DeserializeKey(*keyPointer, m_name);
            }
            catch (...)
            {
                std::throw_with_nested(ext::exception(std::source_location::current(),
                    (std::string("Can't deserialize key with type ") + ext::type_name<KeyType>()).c_str()));
            }
        }
        else
            m_key = deserialize_value<KeyType>(m_name);
    }

private:
    // ISerializableField
    [[nodiscard]] SerializableValue GetName() const override { return m_name; }
    [[nodiscard]] std::shared_ptr<ISerializable> GetField() override { return get_as_serializable<ValueType>(&m_value); }
    // ISerializable
    void OnSerializationStart() override { EXT_EXPECT(false) << "Unexpected call"; }
    void OnSerializationEnd() override { EXT_EXPECT(false) << "Unexpected call"; }

public:
    // We store the name to return it, because we could remove fields and after deserialization object will change
    SerializableValue m_name;
    KeyType m_key;
    ValueType m_value;
};

// Special class for serializing/deserializing std::map/std::multimap types
template <class MapType>
struct SerializableMap : ISerializableObject
{
    using SerializablePair = SerializableKeyValuePair<typename MapType::key_type, typename MapType::mapped_type>;
    using DeserializablePair = DeserializableKeyValuePair<typename MapType::key_type, typename MapType::mapped_type>;

    SerializableMap(MapType* typePointer) : m_mapPointer(typePointer) {}
protected:
// ISerializable
    // Called before collection deserialization
    void OnDeserializationStart(SerializableNode& deserializableTree) override
    {
        m_mapPointer->clear();
        EXT_EXPECT(m_deserializableValuesPairs.empty());
        m_deserializableValuesPairs.resize(deserializableTree.CountChilds());
        for (size_t i = 0, size = deserializableTree.CountChilds(); i < size; ++i)
        {
            m_deserializableValuesPairs[i] = std::make_shared<DeserializablePair>(*deserializableTree.GetChild(i));
        }
    }
    // Called after collection deserialization
    void OnDeserializationEnd() override
    {
        for (auto&& values : m_deserializableValuesPairs)
        {
            m_mapPointer->emplace(std::move(values->m_key), std::move(values->m_value));
        }
        m_deserializableValuesPairs.clear();
    }

// ISerializableObject
    [[nodiscard]] virtual const char* ObjectName() const noexcept override { return ext::type_name<MapType>(); }
    [[nodiscard]] size_t Size() const noexcept override { return m_deserializableValuesPairs.size() + m_mapPointer->size(); }
    [[nodiscard]] std::shared_ptr<ISerializableField> Get(const size_t& index) override
    {
        if (!m_deserializableValuesPairs.empty())
        {
            EXT_EXPECT(index < m_deserializableValuesPairs.size());
            return m_deserializableValuesPairs[index];
        }

        EXT_EXPECT(index < m_mapPointer->size());
        auto it = std::next(m_mapPointer->begin(), index);
        return std::make_shared<SerializablePair>(it->first, it->second);
    }

private:
    MapType* m_mapPointer;
    // Array where we store all objects during deserialization
    // We store them here because we can't resize map
    std::vector<std::shared_ptr<DeserializablePair>> m_deserializableValuesPairs;
};

// Special class for serializing/deserializing std::set/std::multiset types
template <class SetType>
struct SerializableSet : ISerializableArray
{
    SerializableSet(SetType* typePointer) : m_setPointer(typePointer) {}
protected:
// ISerializable
    void OnDeserializationStart(SerializableNode& deserializableTree) override
    {
        m_setPointer->clear();
        m_serializableValues.clear();
        m_serializableValues.resize(deserializableTree.CountChilds());
    }
    // Called after collection deserialization
    void OnDeserializationEnd() override
    {
        for (const auto& deserializedValue : m_serializableValues)
        {
            m_setPointer->emplace(std::move(deserializedValue));
        }
        m_serializableValues.clear();
    }
// ISerializableArray
    [[nodiscard]] size_t Size() const noexcept override { return m_serializableValues.size() + m_setPointer->size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) override
    {
        // if we in process of the deserialization - return values from m_serializableValues
        if (!m_serializableValues.empty())
        {
            EXT_EXPECT(index < m_serializableValues.size());
            return get_as_serializable(&m_serializableValues[index]);
        }

        EXT_EXPECT(index < m_setPointer->size());
        auto it = std::next(m_setPointer->begin(), index);
        if constexpr (is_serializable_v<typename SetType::key_type>)
            return get_serializable(&*it);
        else
            // if set types if not serializable - store string value presentation
            return std::make_shared<SerializableValueHolder>(serialize_value(*it));
    }
private:
    // Array where we store all objects during deserialization
    // We store them here because we can't resize set with default values
    std::vector<typename SetType::key_type> m_serializableValues;
    SetType* m_setPointer;
};

template <class Type, class = std::void_t<>>
struct SerializableProxy;

template <class Type>
struct SerializableValueProxy : ISerializableOptional
{
    SerializableValueProxy(Type* typePointer) : m_typePointer(typePointer) {}

protected:
// ISerializable
    void OnSerializationStart() override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
            call_on_serialization_start(m_typePointer->get());
        else
            call_on_serialization_start(m_typePointer);
    }
    void OnSerializationEnd() override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
            call_on_serialization_end(m_typePointer->get());
        else
            call_on_serialization_end(m_typePointer);
    }

    void OnDeserializationStart(SerializableNode& serializableTree) override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
        {
            bool tryCreateDefault = false;
            if constexpr (is_iserializable_collection_v<RealType> || is_serializable_object<RealType>)
                tryCreateDefault = serializableTree.CountChilds() > 0;
            else
                tryCreateDefault = serializableTree.Value.value_or(SerializableValue::CreateNull()).Type != SerializableValue::ValueType::eNull;

            if (tryCreateDefault)
            {
                if constexpr (std::is_abstract_v<RealType>)
                    EXT_EXPECT(*m_typePointer) << "Object " << ext::type_name<Type>() << " should be created in constructor!";
                else
                    *m_typePointer = create_default_value<Type>();

                call_on_deserialization_start(m_typePointer->get(), serializableTree);
            }
            else
                *m_typePointer = nullptr;
        }
        else
            call_on_deserialization_start(m_typePointer, serializableTree);
    }
    void OnDeserializationEnd() override
    {
        using RealType = std::extract_value_type_v<Type>;
        if constexpr (std::is_same_v<std::unique_ptr<RealType>, Type> || std::is_same_v<std::shared_ptr<RealType>, Type>)
            call_on_deserialization_end(m_typePointer->get());
        else
            call_on_deserialization_end(m_typePointer);
    }

// ISerializableOptional
    [[nodiscard]] std::shared_ptr<ISerializable> Get() const override
    {
        using ExtractedType = std::extract_value_type_v<Type>;
        ExtractedType* pointer = nullptr;
        if constexpr (std::is_same_v<std::unique_ptr<ExtractedType>, Type> ||
                      std::is_same_v<std::shared_ptr<ExtractedType>, Type>)
            pointer = m_typePointer->get();
        else
            pointer = m_typePointer;
        if (pointer == nullptr)
            return std::make_shared<SerializableValueHolder>(SerializableValue::CreateNull());

        if constexpr (is_registered_serializable_object_v<ExtractedType>)
        {
            auto& descriptor = ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>();
            return descriptor.GetSerializable(*pointer);
        }
        /* Use reinterpret_cast in case of private inheritance, example:
        struct Settings
        {
            struct SubSettings : private ISerializableArray <--
            {};
            DECLARE_SERIALIZABLE_FIELD(SubSettings, field);
        }
        */
        else if constexpr (std::is_base_of_v<ISerializableArray, ExtractedType>) 
            return std::make_shared<SerializableProxy<ExtractedType>>(reinterpret_cast<ISerializableArray*>(pointer));
        else if constexpr (std::is_base_of_v<ISerializableObject, ExtractedType>) 
            return std::make_shared<SerializableProxy<ExtractedType>>(reinterpret_cast<ISerializableObject*>(pointer));
        else if constexpr (std::is_base_of_v<ISerializableField, ExtractedType>) 
            return std::make_shared<SerializableProxy<ExtractedType>>(reinterpret_cast<ISerializableField*>(pointer));
        else if constexpr (std::is_base_of_v<ISerializableValue, ExtractedType>) 
            return std::make_shared<SerializableProxy<ExtractedType>>(reinterpret_cast<ISerializableValue*>(pointer));
        else
        {
            static_assert(is_serializable_object<ExtractedType>, "Object can be serializable in C++20 but not registered");
            auto& descriptor = ext::get_singleton<SerializableObjectDescriptor<ExtractedType>>();
            return descriptor.GetSerializable(*pointer);
        }
    }

private:
    Type* m_typePointer;
};

// Proxy class for ISerializableValue
template <class Type>
struct SerializableProxy<Type, std::enable_if_t<is_based_on<ISerializableValue, Type>>> : ISerializableValue
{
    SerializableProxy(ISerializableValue* valuePointer) : m_value(valuePointer) { EXT_EXPECT(m_value); }

protected:
// ISerializable
    void OnSerializationStart() EXT_THROWS() override { m_value->OnSerializationStart(); }
    void OnSerializationEnd() EXT_THROWS() override { m_value->OnSerializationEnd(); }
    void OnDeserializationStart(SerializableNode& deserializableTree) override { m_value->OnDeserializationStart(deserializableTree); }
    void OnDeserializationEnd() EXT_THROWS() override { m_value->OnDeserializationEnd(); }
// ISerializableValue
    [[nodiscard]] SerializableValue SerializeValue() const override { return m_value->SerializeValue(); }
    void DeserializeValue(const SerializableValue& value) override { m_value->DeserializeValue(value); }

    ISerializableValue* m_value;
};

// Proxy class for ISerializableField
template <class Type>
struct SerializableProxy<Type, std::enable_if_t<is_based_on<ISerializableField, Type>>> : ISerializableField
{
    SerializableProxy(ISerializableField* fieldPointer) : m_field(fieldPointer) { EXT_EXPECT(m_field); }

protected:
// ISerializable
    void OnSerializationStart() EXT_THROWS() override { m_field->OnSerializationStart(); }
    void OnSerializationEnd() EXT_THROWS() override { m_field->OnSerializationEnd(); }
    void OnDeserializationStart(SerializableNode& deserializableTree) override { m_field->OnDeserializationStart(deserializableTree); }
    void OnDeserializationEnd() EXT_THROWS() override { m_field->OnDeserializationEnd(); }
// ISerializableField
    [[nodiscard]] SerializableValue GetName() const override { return m_field->GetName(); }
    [[nodiscard]] std::shared_ptr<ISerializable> GetField() override { return m_field->GetField(); }

    ISerializableField* m_field;
};

// Proxy class for ISerializableArray
template <class Type>
struct SerializableProxy<Type, std::enable_if_t<is_based_on<ISerializableArray, Type>>> : ISerializableArray
{
    SerializableProxy(ISerializableArray* objectPointer) : m_array(objectPointer) { EXT_EXPECT(m_array); }

protected:
// ISerializable
    void OnSerializationStart() EXT_THROWS() override { m_array->OnSerializationStart(); }
    void OnSerializationEnd() EXT_THROWS() override { m_array->OnSerializationEnd(); }
    void OnDeserializationStart(SerializableNode& deserializableTree) override {  m_array->OnDeserializationStart(deserializableTree); }
    void OnDeserializationEnd() EXT_THROWS() override { m_array->OnDeserializationEnd(); }
// ISerializableArray
    [[nodiscard]] size_t Size() const noexcept override { return m_array->Size(); }
    [[nodiscard]] std::shared_ptr<ISerializable> Get(const size_t& index) override { return m_array->Get(index); }

    ISerializableArray* m_array;
};

// Proxy class for ISerializableObject, used for base classes
template <class Type>
struct SerializableProxy<Type, std::enable_if_t<is_based_on<ISerializableObject, Type>>> : ISerializableObject
{
    SerializableProxy(ISerializableObject* objectPointer) : m_object(objectPointer) { EXT_EXPECT(m_object); }

protected:
// ISerializable
    void OnSerializationStart() EXT_THROWS() override { m_object->OnSerializationStart(); }
    void OnSerializationEnd() EXT_THROWS() override { m_object->OnSerializationEnd(); }
    void OnDeserializationStart(SerializableNode& deserializableTree) override { m_object->OnDeserializationStart(deserializableTree); }
    void OnDeserializationEnd() EXT_THROWS() override { m_object->OnDeserializationEnd(); }
// ISerializableObject
    [[nodiscard]] const char* ObjectName() const noexcept override { return m_object->ObjectName(); }
    [[nodiscard]] size_t Size() const noexcept override { return m_object->Size(); }
    [[nodiscard]] std::shared_ptr<ISerializableField> Get(const size_t& index) override { return m_object->Get(index); }

    ISerializableObject* m_object;
};

template <class Type>
std::shared_ptr<ISerializable> get_serializable(const Type* value)
{
    static_assert(is_serializable_v<Type>, "Type is not serializable");
    return std::make_shared<SerializableValueProxy<Type>>(const_cast<Type*>(value));
}

template <class Type>
std::shared_ptr<ISerializable> get_as_serializable(Type* type)
{
    static_assert(!std::is_const_v<Type>, "Type should not be const");

    if constexpr (std::is_same_v<std::optional<std::extract_value_type_v<Type>>, Type>)
        return std::make_shared<details::SerializableOptionalImpl<Type>>(type);
    else if constexpr (details::is_map_v<Type>)
        return std::make_shared<details::SerializableMap<Type>>(type);
    else if constexpr (details::is_set_v<Type>)
        return std::make_shared<details::SerializableSet<Type>>(type);
    else if constexpr (details::is_pair_v<Type>)
    {
        auto collection = std::make_shared<SerializableObjectImpl>("pair");
        collection->AddField("first", &type->first);
        collection->AddField("second", &type->second);
        return collection;
    }
    else if constexpr (details::is_array_v<Type>)
        return std::make_shared<details::SerializableArrayImpl<Type>>(type);
    else if constexpr (is_serializable_v<Type>)
        return get_serializable(type);
    else
        return std::make_shared<details::SerializableValueImpl<Type>>(type);
}

} // namespace ext::serializable::details

#include <ext/details/serialization/serializable_descriptor_details.h>
