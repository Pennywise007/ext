#pragma once

#include <memory>
#include <string>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/mpl.h>

#include <ext/reflection/object.h>

#include <ext/serialization/iserializable.h>

#include <ext/std/type_traits.h>

#include <ext/types/utils.h>

namespace ext::serializable {
namespace details {

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
void call_on_deserialization_start(Type *pointer, SerializableNode& serializableTree)
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

// Wrapper around Type object to convert it to the ISerializableObject to create ability to iterate over fields
template <class Type>
struct SerializableObjectDescription : ISerializableObject
{
    SerializableObjectDescription(Type& object,
        const std::list<std::shared_ptr<details::ISerializableFieldInfo>>& fields,
        const std::list<std::shared_ptr<details::ISerializableBaseInfo>>& baseSerializableClasses)
    : m_object(object)
    , m_fields(fields)
    , m_baseSerializableClasses(baseSerializableClasses)
    {}

private:
// ISerializable
    void OnSerializationStart() override { call_on_serialization_start(&m_object); }
    void OnSerializationEnd() override { call_on_serialization_end(&m_object); }
    void OnDeserializationStart(SerializableNode& serializableTree) override { 
        call_on_deserialization_start(&m_object, serializableTree);
    }
    void OnDeserializationEnd() override { call_on_deserialization_end(&m_object); }

// ISerializableObject
    [[nodiscard]] virtual const char* ObjectName() const noexcept override { return ext::type_name<Type>(); }
    [[nodiscard]] size_t Size() const noexcept override
    {
        size_t result = 0;
        for (const auto& base : m_baseSerializableClasses)
        {
            result += base->CountFields(&m_object);
        } 
        return result + m_fields.size(); 
    }
    [[nodiscard]] std::shared_ptr<ISerializableField> Get(const size_t& requestedIndex) override
    {
        const auto getField = [&]() -> std::shared_ptr<ISerializableFieldInfo> {
            size_t index = requestedIndex;
            for (const auto& base : m_baseSerializableClasses)
            {
                auto baseClassFields = base->CountFields(&m_object);
                if (index < baseClassFields)
                    return base->GetField(index, &m_object);
                index -= baseClassFields;
            }

            EXT_ASSERT(index < m_fields.size());
            return *std::next(m_fields.begin(), index);
        };

        auto field = getField();
        EXT_EXPECT(field) << "Failed to get field at index " << requestedIndex
            << " for object " << ext::type_name<Type>();

        return std::make_shared<details::SerializableFieldImpl>(field->GetName(), field->GetField(&m_object));
    }

private:
    Type& m_object;
    const std::list<std::shared_ptr<details::ISerializableFieldInfo>> m_fields;
    const std::list<std::shared_ptr<details::ISerializableBaseInfo>> m_baseSerializableClasses;
};

template <class Type, typename Field>
struct SerializableFieldInfo : ISerializableFieldInfo
{
    SerializableFieldInfo(const char* name, Field Type::* field) : m_name(name), m_field(field) {}

protected:
// ISerializableFieldInfo
    [[nodiscard]] const char* GetName() const { return m_name; }
    [[nodiscard]] std::shared_ptr<ISerializable> GetField(void* objectPointer) const override
    {
        Type* typePointer = reinterpret_cast<Type*>(objectPointer);
        EXT_EXPECT(typePointer) << "Can`t get type " << ext::type_name<Type>();
        return get_as_serializable<Field>(&((*typePointer).*m_field));
    }

protected:
    const char* m_name;
    Field Type::* m_field;
};

struct SerializableFieldInfoWrapper : ISerializableFieldInfo
{
    SerializableFieldInfoWrapper(const std::shared_ptr<ext::serializable::ISerializableField>& field)
        : m_name(std::narrow(field->GetName())), m_field(field->GetField()) {}

protected:
// ISerializableFieldInfo
    [[nodiscard]] const char* GetName() const { return m_name.c_str(); }
    [[nodiscard]] std::shared_ptr<ISerializable> GetField(void*) const override { return m_field; }

protected:
    const std::string m_name;
    const std::shared_ptr<ISerializable> m_field;
};

template <class Type, class BaseType>
struct SerializableBaseInfo : ISerializableBaseInfo
{
    static_assert(is_based_on<BaseType, Type>);
    static_assert(is_serializable_v<BaseType>);

protected:
// ISerializableBaseInfo
    [[nodiscard]] size_t CountFields(void* objectPointer) const override
    {
        if constexpr (is_registered_serializable_object_v<BaseType>)
            return ext::get_singleton<SerializableObjectDescriptor<BaseType>>().GetFieldsCount(objectPointer);
        else if constexpr (is_iserializable_v<BaseType>)
        {
            static_assert(is_based_on<ISerializableField, BaseType> || is_based_on<ISerializableObject, BaseType>,
                "Only serializable field or object can be base classes");
            if constexpr (is_based_on<ISerializableField, BaseType>)
                return 1;
            else
            {
                BaseType* object = ext::get_singleton<SerializableObjectDescriptor<Type>>().
                    template ConvertToType<BaseType>(reinterpret_cast<Type*>(objectPointer));
                const SerializableObjectDescriptor<BaseType>& baseDescriptor = ext::get_singleton<SerializableObjectDescriptor<BaseType>>();
                return baseDescriptor.template ConvertToType<ISerializableObject>(object)->Size();
            }
        }
        else
        {
            static_assert(is_serializable_object<BaseType>, "Object can be serializable in C++20");
            return ext::get_singleton<SerializableObjectDescriptor<BaseType>>().GetFieldsCount(objectPointer);
        }
    }

    [[nodiscard]] std::shared_ptr<ISerializableFieldInfo> GetField(const size_t& index, void* objectPointer) const override
    {
        EXT_ASSERT(index < CountFields(objectPointer));
        auto getSerializableField = [&]() -> std::shared_ptr<ext::serializable::ISerializableField> {
            BaseType* object = ext::get_singleton<SerializableObjectDescriptor<Type>>().
                template ConvertToType<BaseType>(reinterpret_cast<Type*>(objectPointer));

            const SerializableObjectDescriptor<BaseType>& baseDescriptor = ext::get_singleton<SerializableObjectDescriptor<BaseType>>();
            if constexpr (is_registered_serializable_object_v<BaseType>)
                return baseDescriptor.GetSerializable(*object)->Get(index);
            else if constexpr (is_iserializable_v<BaseType>)
            {
                static_assert(is_based_on<ISerializableField, BaseType> || is_based_on<ISerializableObject, BaseType>,
                    "Only serializable field or object can be base classes");

                if constexpr (is_based_on<ISerializableField, BaseType>)
                {
                    EXT_EXPECT(index == 0) << "Unexpected field index";
                    ext::serializable::ISerializableField* field = baseDescriptor.template ConvertToType<ext::serializable::ISerializableField>(object);
                    return std::make_shared<ext::serializable::details::SerializableProxy<BaseType>>(field);
                }
                else
                {
                    BaseType* object = ext::get_singleton<SerializableObjectDescriptor<Type>>().
                        template ConvertToType<BaseType>(reinterpret_cast<Type*>(objectPointer));
                    const SerializableObjectDescriptor<BaseType>& baseDescriptor = ext::get_singleton<SerializableObjectDescriptor<BaseType>>();
                    return baseDescriptor.template ConvertToType<ISerializableObject>(object)->Get(index);
                }
            }
            else
            {
                static_assert(is_serializable_object<BaseType>, "Object can be serializable in C++20");
                return baseDescriptor.GetSerializable(*object)->Get(index);
            }
        };

        return std::make_shared<SerializableFieldInfoWrapper>(getSerializableField());
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
        static_assert(is_serializable_object<BaseType> ||
            details::is_based_on<ISerializableField, BaseType> ||
            details::is_based_on<ISerializableObject, BaseType>,
            "Only serializable field or object can be base classes");

        m_baseSerializableClasses.emplace_back(std::make_shared<details::SerializableBaseInfo<Type, BaseType>>());
    });
}

template<class Type>
[[nodiscard]] size_t SerializableObjectDescriptor<Type>::GetFieldsCount(void* objectPointer) const
{
    static const size_t fieldsCount = [&]() {
        size_t result = 0;
        for (const auto& base : m_baseSerializableClasses)
        {
            result += base->CountFields(objectPointer);
        }

        if constexpr (!is_registered_serializable_object_v<Type>)
            return result + ext::reflection::fields_count<Type>;
        else
            return result + m_fields.size();
    }();
    return fieldsCount;
}

template<class Type>
[[nodiscard]] std::shared_ptr<ISerializableObject> SerializableObjectDescriptor<Type>::GetSerializable(Type& object) const
{
#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20
    if constexpr (!is_registered_serializable_object_v<Type>)
    {
        static_assert(ext::reflection::fields_count<Type> > 0, "No fields in object for serialization");

        auto fields = ext::reflection::get_object_fields(object);
        
        auto fieldsCollection = std::make_shared<details::SerializableObjectImpl>(ext::type_name<Type>());
        const auto registerField = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (([&] {
                constexpr auto fieldName = ext::reflection::field_name<Type, Is>;
                fieldsCollection->AddField(std::string(fieldName.data(), fieldName.size()), &std::get<Is>(fields));
            }()), ...);
        };
        registerField(std::make_index_sequence<std::tuple_size_v<decltype(fields)>>());
        return fieldsCollection;
    }
    else
#endif // C++20
    {
        EXT_ASSERT(!m_fields.empty()) << "Object " << ext::type_name<Type>() << " doesn't have any registered fields." 
            << " Did you forget to register them?";
        return std::make_shared<details::SerializableObjectDescription<Type>>(
            object, m_fields, m_baseSerializableClasses);
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
inline void SerializableObjectDescriptor<Type>::CallOnDeserializationStart(Type* pointer, SerializableNode& serializableTree) const
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
