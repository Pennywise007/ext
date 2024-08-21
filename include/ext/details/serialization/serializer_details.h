#pragma once

/*
* Implementation for ext/serialization/serializer.h
*/

#include <string.h>
#include <map>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/reflection/enum.h>

#include <ext/serialization/serializer.h>

#include <ext/serialization/serializers/json.h>

namespace ext::serializer {

template <class Type>
inline void SerializeToJson(const Type& object, std::wstring& outputJson, std::wstring indentText, bool addNewLines) EXT_THROWS()
{
    SerializeObject(std::make_unique<SerializerJson>(outputJson, indentText, addNewLines), object);
}

template <class Type>
inline void DeserializeFromJson(Type& object, const std::wstring& json) EXT_THROWS()
{
    DeserializeObject(std::make_unique<DeserializerJson>(json), object);
}

template <class Type>
inline void SerializeObject(const std::unique_ptr<ISerializer>& serializer, const Type& object) EXT_THROWS()
{
    static_assert(ext::serializable::details::is_serializable_v<Type>,
        "Type not serializable, maybe you forgot to add REGISTER_SERIALIZABLE_OBJECT?");

    EXT_REQUIRE(serializer) << "No serializer passed";

    std::shared_ptr<ISerializableObject> objectCollection;
    const ISerializable* serializableObject;
    if constexpr (ext::serializable::is_registered_serializable_object_v<Type>)
    {
        objectCollection = ext::get_singleton<ext::serializable::SerializableObjectDescriptor<Type>>().GetSerializable(const_cast<Type&>(object));
        serializableObject = objectCollection.get();
    }
    else if constexpr (std::is_base_of_v<ISerializable, Type>)
    {
        // To avoid problems with private inheritance will use C++ magic
        serializableObject = reinterpret_cast<const ISerializable*>(&object);
    }
    else
    {
        static_assert(ext::serializable::is_serializable_object<Type>, "Object can be serializable in C++20");

        objectCollection = ext::get_singleton<ext::serializable::SerializableObjectDescriptor<Type>>().GetSerializable(const_cast<Type&>(object));
        serializableObject = objectCollection.get();
    }
    EXT_EXPECT(serializableObject) << "Can't find serializable description for " << ext::type_name<Type>();

    SerializableNode serializationTreeRoot = SerializableNode::ValueNode();
    SerializableNode* currentNode = nullptr;

    Visitor objectsVisitor(const_cast<ISerializable*>(serializableObject));

    try
    {
        do
        {
            switch (objectsVisitor.GetCurrentObjectType())
            {
            case Visitor::ObjectType::eObjectStart:
            {
                auto* objectPointer = dynamic_cast<ISerializableObject*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(objectPointer);
                objectPointer->OnSerializationStart();
            
                if (!currentNode)
                {
                    serializationTreeRoot = SerializableNode::ObjectNode(objectPointer->ObjectName());
                    currentNode = &serializationTreeRoot;
                }
                else
                    currentNode = currentNode->AddChild(SerializableNode::ObjectNode(objectPointer->ObjectName()));
            }
            break;
            case Visitor::ObjectType::eArrayStart:
            {
                auto* arrayPointer = dynamic_cast<ISerializableArray*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(arrayPointer);
                arrayPointer->OnSerializationStart();
            
                if (!currentNode)
                {
                    serializationTreeRoot = SerializableNode::ArrayNode();
                    currentNode = &serializationTreeRoot;
                }
                else
                    currentNode = currentNode->AddChild(SerializableNode::ArrayNode());
            }
            break;
            case Visitor::ObjectType::eFieldStart:
            {
                auto* field = dynamic_cast<ISerializableField*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(field);
                field->OnSerializationStart();

                if (!currentNode)
                {
                    serializationTreeRoot = SerializableNode::FieldNode(field->GetName());
                    currentNode = &serializationTreeRoot;
                }
                else
                    currentNode = currentNode->AddChild(SerializableNode::FieldNode(field->GetName()));
            }
            break;
            case Visitor::ObjectType::eFieldEnd:
            case Visitor::ObjectType::eArrayEnd:
            case Visitor::ObjectType::eObjectEnd:
            case Visitor::ObjectType::eOptionalEnd:
            {
                EXT_EXPECT(currentNode);
                currentNode = currentNode->Parent;
                objectsVisitor.GetCurrentObject()->OnSerializationEnd();
            }
            break;
            case Visitor::ObjectType::eValue:
            {
                auto* value = dynamic_cast<ISerializableValue*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(value);
                value->OnSerializationStart();

                if (!currentNode)
                {
                    serializationTreeRoot = SerializableNode::ValueNode(value->SerializeValue());
                    currentNode = &serializationTreeRoot;
                }
                else
                    currentNode->AddChild(SerializableNode::ValueNode(value->SerializeValue()));

                value->OnSerializationEnd();
            }
            break;
            case Visitor::ObjectType::eOptionalStart:
                objectsVisitor.GetCurrentObject()->OnSerializationStart();
            break;
            default:
                static_assert(ext::reflection::get_enum_size<Visitor::ObjectType>() == 9,
                    "Unsupported ObjectType, update serialization code");
            }
        } while (objectsVisitor.GoToNextObject());
    }
    catch (...)
    {
        std::string description = objectsVisitor.GetCurrentObjectTreeDescription();

        if (description.empty())
            throw;
        else
            std::throw_with_nested(ext::exception(std::source_location::current(),
                std::string_sprintf("Failed to serialize: '%s', err = %s",
                    description.c_str(), ext::ManageExceptionText("").c_str()).c_str()));
    }

    serializer->Serialize(serializationTreeRoot);
}

template <class Type>
inline void DeserializeObject(const std::unique_ptr<serializer::IDeserializer>& deserializer, Type& object) EXT_THROWS()
{
    static_assert(ext::serializable::details::is_serializable_v<Type>, "Type not serializable, maybe you forgot to add REGISTER_SERIALIZABLE_OBJECT?");
    
    EXT_REQUIRE(deserializer) << "No deserializer passed";

    std::shared_ptr<ISerializableObject> objectCollection;
    ISerializable* deserializableObject;
    if constexpr (ext::serializable::is_registered_serializable_object_v<Type>)
    {
        objectCollection = ext::get_singleton<ext::serializable::SerializableObjectDescriptor<Type>>().GetSerializable(object);
        deserializableObject = objectCollection.get();
    }
    else if constexpr (std::is_base_of_v<ISerializable, Type>)
    {
        // To avoid problems with private inheritance will use C++ magic
        deserializableObject = reinterpret_cast<ISerializable*>(&object);
    }
    else
    {
        static_assert(ext::serializable::is_serializable_object<Type>, "Object can be serializable in C++20");
       
        objectCollection = ext::get_singleton<ext::serializable::SerializableObjectDescriptor<Type>>().GetSerializable(object);
        deserializableObject = objectCollection.get();
    }

    EXT_EXPECT(deserializableObject) << "Can't find deserializable description for " << ext::type_name<Type>();

    SerializableNode& deserializationTreeRoot = deserializer->Deserialize();
    SerializableNode* currentNode = nullptr;

    Visitor objectsVisitor(deserializableObject);

    try
    {
        do
        {
            switch (objectsVisitor.GetCurrentObjectType())
            {
            case Visitor::ObjectType::eObjectStart:
            {
                if (!currentNode)
                    currentNode = &deserializationTreeRoot;
                else
                    currentNode = currentNode->GetChild(objectsVisitor.GetCurrentIndexInCollection());

                auto* objectPointer = dynamic_cast<ISerializableObject*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(objectPointer);

                EXT_EXPECT(currentNode) << "Can't find object node for " << objectPointer->ObjectName()
                    << " with index " << objectsVisitor.GetCurrentIndexInCollection();
                EXT_EXPECT(currentNode->Type == SerializableNode::NodeType::eObject) <<
                    "Unexpected node type, expect object, received: " << ext::reflection::get_enum_value_name(currentNode->Type) <<
                    ". Object name " << objectPointer->ObjectName();

                objectPointer->OnDeserializationStart(*currentNode);
                objectsVisitor.UpdateCurrentCollectionSize();

                currentNode->CacheChildNames();
                objectsVisitor.CacheChildNodeNames();
            }
            break;
            case Visitor::ObjectType::eArrayStart:
            {
                if (!currentNode)
                    currentNode = &deserializationTreeRoot;
                else
                    currentNode = currentNode->GetChild(objectsVisitor.GetCurrentIndexInCollection());

                EXT_EXPECT(currentNode) << "Can't find array node with index " << objectsVisitor.GetCurrentIndexInCollection();
                EXT_EXPECT(currentNode->Type == SerializableNode::NodeType::eArray) <<
                    "Unexpected node type, expect array, received: " << ext::reflection::get_enum_value_name(currentNode->Type);

                auto* arrayPointer = dynamic_cast<ISerializableArray*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(arrayPointer);
                arrayPointer->OnDeserializationStart(*currentNode);

                objectsVisitor.UpdateCurrentCollectionSize();
            }
            break;
            case Visitor::ObjectType::eFieldStart:
            {
                EXT_EXPECT(currentNode) << "Field can't be a root node";

                auto* field = dynamic_cast<ISerializableField*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(field);

                auto* fieldNode = currentNode->GetChild(field->GetName(), objectsVisitor.GetIndexAmongIdenticalNames());
                if (fieldNode)
                {
                    currentNode = fieldNode;
                    field->OnDeserializationStart(*currentNode);
                }
                else
                {
                    // Field doesn't exist, it can be optional
                    EXT_EXPECT(std::dynamic_pointer_cast<ISerializableOptional>(field->GetField()))
                        << "Can't find field '" << field->GetName() << "'";
                    objectsVisitor.SkipCollectionContent();
                }
            }
            break;
            case Visitor::ObjectType::eFieldEnd:
            case Visitor::ObjectType::eArrayEnd:
            case Visitor::ObjectType::eObjectEnd:
            case Visitor::ObjectType::eOptionalEnd:
            {
                EXT_EXPECT(currentNode);
                currentNode = currentNode->Parent;
                objectsVisitor.GetCurrentObject()->OnDeserializationEnd();
            }
            break;
            case Visitor::ObjectType::eOptionalStart:
            {
                auto* optionalField = dynamic_cast<ISerializableOptional*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(optionalField);

                if (!currentNode)
                {
                    currentNode = &deserializationTreeRoot;
                    if (!currentNode->Value.has_value() || currentNode->Value->Type != SerializableValue::ValueType::eNull)
                        optionalField->OnDeserializationStart(*currentNode);
                    else
                        objectsVisitor.SkipCollectionContent();
                }
                else
                {
                    auto* optionalNode = currentNode->GetChild(objectsVisitor.GetCurrentIndexInCollection());
                    if (optionalNode)
                        optionalField->OnDeserializationStart(*optionalNode);
                    else
                        objectsVisitor.SkipCollectionContent();
                }
            }
            break;
            case Visitor::ObjectType::eValue:
            {
                EXT_EXPECT(currentNode) << "Value can't be a root node";
                EXT_ASSERT(currentNode->Type != SerializableNode::NodeType::eValue) << "Value can't have another value as a parent node";

                currentNode = currentNode->GetChild(objectsVisitor.GetCurrentIndexInCollection());
                EXT_EXPECT(currentNode) << "Can't find value node " << objectsVisitor.GetCurrentIndexInCollection();
                EXT_EXPECT(currentNode->Type == SerializableNode::NodeType::eValue);
                EXT_EXPECT(currentNode->Value.has_value()) << "Value not set";

                auto* value = dynamic_cast<ISerializableValue*>(objectsVisitor.GetCurrentObject());
                EXT_EXPECT(value);

                value->OnDeserializationStart(*currentNode);
                value->DeserializeValue(currentNode->Value.value());
                value->OnDeserializationEnd();

                currentNode = currentNode->Parent;
            }
            break;
            default:
                static_assert(ext::reflection::get_enum_size<Visitor::ObjectType>() == 9,
                    "Unsupported ObjectType, update deserialization code");
            }

        } while (objectsVisitor.GoToNextObject());
    }
    catch (...)
    {
        std::string description = objectsVisitor.GetCurrentObjectTreeDescription();

        if (description.empty())
            throw;
        else
            std::throw_with_nested(ext::exception(std::source_location::current(),
                std::string_sprintf("Failed to deserialize: '%s', err = %s",
                    description.c_str(), ext::ManageExceptionText("").c_str()).c_str()));
    }
}

// Information about collection, collection types:
// array - array of objects/values
// object - map with field names and values
// field - array of size 1 where 1 element is field value
struct Visitor::CollectionInfo
{
    explicit CollectionInfo(const ISerializableArray* array_)
        : array(array_)
        , sizeOfCollection(array_->Size())
    {}
    explicit CollectionInfo(const ISerializableObject* object_)
        : object(object_)
        , sizeOfCollection(object_->Size())
    {}
    explicit CollectionInfo(const ISerializableField* field_)
        : field(field_)
        , sizeOfCollection(1L)
    {}
    explicit CollectionInfo(const std::shared_ptr<ISerializableArray>& array_)
        : CollectionInfo(array_.get())
    {
        collectionHolder = array_; 
    }
    explicit CollectionInfo(const std::shared_ptr<ISerializableObject>& object_)
        : CollectionInfo(object_.get())
    {
        collectionHolder = object_; 
    }
    explicit CollectionInfo(const std::shared_ptr<ISerializableField>& field_)
        : CollectionInfo(field_.get())
    {
        collectionHolder = field_; 
    }

    [[nodiscard]] std::shared_ptr<ISerializable> Get(size_t index)
    {
        EXT_EXPECT(index < sizeOfCollection);
        EXT_EXPECT(object || array || field);
        if (object)
            return const_cast<ISerializableObject*>(object)->Get(index);
        else if (array)
            return const_cast<ISerializableArray*>(array)->Get(index);
        else
            return const_cast<ISerializableField*>(field)->GetField();
    }

    [[nodiscard]] ISerializable* GetCollection() const
    {
        EXT_EXPECT(object || array || field);
        if (object)
            return const_cast<ISerializableObject*>(object);
        else if (array)
            return const_cast<ISerializableArray*>(array);
        else
            return const_cast<ISerializableField*>(field);
    }

    void UpdateCollectionSize()
    {
        EXT_EXPECT(object || array);
        if (object)
            sizeOfCollection = object->Size();
        else
            sizeOfCollection = array->Size();
    }

    void CacheChildNodeNames()
    {
        EXT_EXPECT(object);
        EXT_ASSERT(m_childsByNames.empty());

        for (size_t i = 0; i < sizeOfCollection; ++i)
        {
            if (const auto field_ = const_cast<ISerializableObject*>(object)->Get(i); field_)
            {
                m_childsByNames.emplace(field_->GetName(), i);
            }
        }
    }

    [[nodiscard]] size_t NumberFieldsWithSameNameBeforeIndex(const wchar_t* name, size_t index) const
    {
        EXT_EXPECT(object);
        size_t res = 0;
        auto range = m_childsByNames.equal_range(name);
        for (auto i = range.first; i != range.second && i->second < index; ++i)
        {
            ++res;
        }
        return res;
    }

public:
    size_t sizeOfCollection;
    size_t currentIndexInsideCollection = 0L;

private:
    const ISerializableArray* array = nullptr;
    const ISerializableObject* object = nullptr;
    const ISerializableField* field = nullptr;
    // we need to keep serializable objects on smart pointer, but root object can be created like ordinary object
    std::shared_ptr<ISerializable> collectionHolder;
    // Map with object fields by name
    std::multimap<std::wstring, size_t> m_childsByNames;
};

inline Visitor::Visitor(ISerializable* object)
    : m_currentSerializableObject(object)
{
    EXT_EXPECT(m_currentSerializableObject.Valid()) << "No info inside collection";
    UpdateObjectType();
}

inline void Visitor::CacheChildNodeNames()
{
    EXT_ASSERT(m_currentSerializableType == ObjectType::eObjectStart) << "Method called on unexpected object type";
    EXT_EXPECT(!m_collectionsDepth.empty()) << "No object collection to cache child";
    m_collectionsDepth.back().CacheChildNodeNames();
}

[[nodiscard]] inline size_t Visitor::GetCurrentIndexInCollection() const
{
    EXT_EXPECT(!m_collectionsDepth.empty()) << "No collection found";

    bool collectionStart =
        m_currentSerializableType == ObjectType::eObjectStart ||
        m_currentSerializableType == ObjectType::eArrayStart;

    if (collectionStart)
    {
        // Last item will be current collection and we need to take item from parent object
        EXT_EXPECT(m_collectionsDepth.size() >= 2) << "No object collection";
        return std::next(m_collectionsDepth.rbegin())->currentIndexInsideCollection;
    }
    return m_collectionsDepth.back().currentIndexInsideCollection;
}

[[nodiscard]] inline size_t Visitor::GetIndexAmongIdenticalNames()
{
    EXT_ASSERT(m_currentSerializableType == ObjectType::eFieldStart) << "Method called on unexpected object type";
    auto* field = dynamic_cast<ISerializableField*>(GetCurrentObject());
    EXT_EXPECT(field && m_collectionsDepth.size() > 1);

    const auto& collectionInfo = *std::next(m_collectionsDepth.rbegin());
    return collectionInfo.NumberFieldsWithSameNameBeforeIndex(field->GetName().c_str(), collectionInfo.currentIndexInsideCollection);
}

inline bool Visitor::GoToNextObject()
{
    switch (GetCurrentObjectType())
    {
    case ObjectType::eObjectStart:
    {
        ISerializableObject* object = m_currentSerializableObject;
        EXT_EXPECT(object);

        if (object->Size() == 0)
        {
            m_currentSerializableType = ObjectType::eObjectEnd;
            return true;
        }

        m_currentSerializableObject = object->Get(0);
        if (!m_currentSerializableObject.Valid())
        {
            EXT_ASSERT(false) << "Not serializable element inside object, skip it";
            return GoToNextObject();
        }
        UpdateObjectType();
    }
    break;
    case ObjectType::eArrayStart:
    {
        ISerializableArray* array = m_currentSerializableObject;
        EXT_EXPECT(array);

        if (array->Size() == 0)
        {
            m_currentSerializableType = ObjectType::eArrayEnd;
            return true;
        }

        m_currentSerializableObject = array->Get(0);
        if (!m_currentSerializableObject.Valid())
        {
            EXT_ASSERT(false) << "Not serializable element inside array, skip it";
            return GoToNextObject();
        }
        UpdateObjectType();
    }
    break;
    case ObjectType::eFieldStart:
    {
        ISerializableField* field = m_currentSerializableObject;
        EXT_EXPECT(field);
        m_currentSerializableObject = field->GetField();
        if (!m_currentSerializableObject.Valid())
        {
            EXT_ASSERT(false) << "Not serializable element as a field, skip it";
            return GoToNextObject();
        }
        UpdateObjectType();
    }
    break;
    case ObjectType::eOptionalStart:
    {
        ISerializableOptional* optional = m_currentSerializableObject;
        EXT_EXPECT(optional);
        m_currentSerializableObject = optional->Get();
        if (!m_currentSerializableObject.Valid())
        {
            EXT_ASSERT(false) << "Not serializable element inside optional, skip it";
            return GoToNextObject();
        }
        UpdateObjectType();
    }
    break;
    case ObjectType::eArrayEnd:
    case ObjectType::eObjectEnd:
    case ObjectType::eFieldEnd:
        // reduce collection depth
        if (!m_collectionsDepth.empty())
            m_collectionsDepth.pop_back();
        [[fallthrough]]; // go to next field
    case ObjectType::eValue:
    case ObjectType::eOptionalEnd:
    {
        if (m_collectionsDepth.empty())
            // only one field serializing
            return false;

        CollectionInfo& collectionInfo = m_collectionsDepth.back();

        // try find serializable element inside collection
        bool bGotSerializableObject = false;
        while (++collectionInfo.currentIndexInsideCollection < collectionInfo.sizeOfCollection)
        {
            m_currentSerializableObject = collectionInfo.Get(collectionInfo.currentIndexInsideCollection);
            if (m_currentSerializableObject.Valid())
            {
                bGotSerializableObject = true;
                break;
            }
        }

        if (bGotSerializableObject)
            UpdateObjectType();
        else
            // collection end
            MoveToCollectionEnd(collectionInfo.GetCollection());
    }
    break;
    default:
        static_assert(ext::reflection::get_enum_size<Visitor::ObjectType>() == 9, "Unsupported ObjectType, update go to next object code");
    }

    return true;
}

 inline void Visitor::SkipCollectionContent() noexcept
 {
    switch (m_currentSerializableType)
    {
    case ObjectType::eObjectStart:
        m_currentSerializableType = ObjectType::eObjectEnd;
        break;
    case ObjectType::eArrayStart:
        m_currentSerializableType = ObjectType::eArrayEnd;
        break;
    case ObjectType::eFieldStart:
        m_currentSerializableType = ObjectType::eFieldEnd;
        break;
    case ObjectType::eOptionalStart:
        m_currentSerializableType = ObjectType::eOptionalEnd;
        break;
    default:
        EXT_ASSERT(false) << "SkipCollectionContent called for not collection";
        m_currentSerializableType = ObjectType::eObjectEnd;
        break;
    }
}

inline void Visitor::UpdateCurrentCollectionSize()
{
    EXT_EXPECT(!m_collectionsDepth.empty()) << "The size of a collection that does not exist is updated";
    m_collectionsDepth.back().UpdateCollectionSize();
}

inline std::string Visitor::GetCurrentObjectTreeDescription() const
{
    std::string result;

    for (auto it = m_collectionsDepth.rbegin(), end = m_collectionsDepth.rend(); it != end; ++it)
    {
        ISerializable* currentSerializableObject = it->GetCollection();

        std::string description;
        if (ISerializableObject* object = dynamic_cast<ISerializableObject*>(currentSerializableObject); object)
            description = "object '" + std::string(object->ObjectName()) + "'";
        else if (ISerializableArray* array = dynamic_cast<ISerializableArray*>(currentSerializableObject); array)
            description = "array index '" + std::to_string(it->currentIndexInsideCollection) + "'";
        else if (ISerializableField* field = dynamic_cast<ISerializableField*>(currentSerializableObject); field)
            description = "field '" + std::narrow(field->GetName()) + "'";
        else if (ISerializableValue* value = dynamic_cast<ISerializableValue*>(currentSerializableObject); value)
            description = "value";
        else
            continue;

        if (result.empty())
            result = std::move(description);
        else
            result = std::move(description) + "->" + result;
    }

    return result;
}

inline void Visitor::UpdateObjectType() EXT_THROWS()
{
    if (const ISerializableObject* object = m_currentSerializableObject)
    {
        m_currentSerializableType = ObjectType::eObjectStart;

        // add to collections depth new object
        if (m_currentSerializableObject.serializableObjectHolder)
            m_collectionsDepth.emplace_back(std::dynamic_pointer_cast<ISerializableObject>(m_currentSerializableObject.serializableObjectHolder));
        else
            m_collectionsDepth.emplace_back(object);
    }
    else if (const ISerializableArray* array = m_currentSerializableObject)
    {
        m_currentSerializableType = ObjectType::eArrayStart;

        // add to collections depth new array
        if (m_currentSerializableObject.serializableObjectHolder)
            m_collectionsDepth.emplace_back(std::dynamic_pointer_cast<ISerializableArray>(m_currentSerializableObject.serializableObjectHolder));
        else
            m_collectionsDepth.emplace_back(array);
    }
    else if (const ISerializableField* field = m_currentSerializableObject)
    {
        m_currentSerializableType = ObjectType::eFieldStart;
        
        // add to collections depth new field
        if (m_currentSerializableObject.serializableObjectHolder)
            m_collectionsDepth.emplace_back(std::dynamic_pointer_cast<ISerializableField>(m_currentSerializableObject.serializableObjectHolder));
        else
            m_collectionsDepth.emplace_back(field);
    }
    else if (const ISerializableOptional* optional = m_currentSerializableObject; optional)
        m_currentSerializableType = ObjectType::eOptionalStart;
    else if (const ISerializableValue* value = m_currentSerializableObject; value)
        m_currentSerializableType = ObjectType::eValue;
    else 
    {
        static_assert(ext::reflection::get_enum_size<ObjectType>() == 9, "Not supported object type");
        EXT_EXPECT(false) << "Unknown type";
    }
}

inline void Visitor::MoveToCollectionEnd(ISerializable* collection) EXT_THROWS()
{
    m_currentSerializableObject = collection;
    if (const ISerializableObject* object = m_currentSerializableObject; object)
        m_currentSerializableType = ObjectType::eObjectEnd;
    else if (const ISerializableArray* array = m_currentSerializableObject; array)
        m_currentSerializableType = ObjectType::eArrayEnd;
    else if (const ISerializableField* field = m_currentSerializableObject; field)
        m_currentSerializableType = ObjectType::eFieldEnd;
    else if (const ISerializableOptional* optional = m_currentSerializableObject; optional)
        m_currentSerializableType = ObjectType::eOptionalEnd;
    else 
    {
        static_assert(ext::reflection::get_enum_size<ObjectType>() == 9,
            "Not supported object type");
        EXT_EXPECT(false) << "Unexpected collection type";
    }
}

template <class Type>
void SerializeKey(const Type& object, std::wstring& text) EXT_THROWS()
{
    SerializeToJson(object, text, L"", false);
}

template <class Type>
void DeserializeKey(Type& object, std::wstring& text) EXT_THROWS()
{
    DeserializeFromJson(object, text);
}

} // namespace ext::serializer
