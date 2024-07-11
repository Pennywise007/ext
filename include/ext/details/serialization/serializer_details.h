#pragma once

/*
* Implementation for ext/serialization/serializer.h
*/

#include <string.h>
#include <map>

#include <ext/core/defines.h>
#include <ext/core/check.h>

#include <ext/serialization/serializer.h>

#ifdef USE_PUGI_XML
#include <ext/serialization/serializers/xml.h>
#endif
#include <ext/serialization/serializers/text.h>

namespace ext::serializer {

#ifdef USE_PUGI_XML
inline std::unique_ptr<ISerializer> Factory::XMLSerializer(const std::filesystem::path& filePath)
{
    return std::make_unique<SerializerXML>(filePath);
}

inline std::unique_ptr<IDeserializer> Factory::XMLDeserializer(const std::filesystem::path& filePath)
{
    return std::make_unique<SerializerXML>(filePath);
}
#endif

inline std::unique_ptr<ISerializer> Factory::TextSerializer(std::wstring& outputText)
{
    return std::make_unique<SerializerText>(&outputText);
}

inline std::unique_ptr<IDeserializer> Factory::TextDeserializer(const std::wstring& inputText)
{
    return std::make_unique<SerializerText>(inputText);
}

struct Visitor::CollectionInfo
{
    explicit CollectionInfo(const ISerializableCollection* collection_) EXT_THROWS()
        : collection(collection_)
        , sizeOfCollection(collection_->Size())
        , currentIndexInsideCollection(0L)
    {
        EXT_EXPECT(collection);
    }
    explicit CollectionInfo(const std::shared_ptr<ISerializableCollection>& collection_) EXT_THROWS()
        : CollectionInfo(collection_.get())
    {
        collectionHolder = collection_;
    }
    const ISerializableCollection* collection;
    // we need to keep serializable objects on smart pointer, but root object can be created like ordinary object
    std::shared_ptr<ISerializableCollection> collectionHolder;
    size_t sizeOfCollection;
    size_t currentIndexInsideCollection;

    size_t NumberChildsWithSameNameBeforeIndex(const char* name, size_t index)
    {
        if (m_childsByNames.size() != sizeOfCollection)
        {
            m_childsByNames.clear();
            for (size_t i = 0; i < sizeOfCollection; ++i)
            {
                if (const auto object = collection->Get(i); object)
                {
                    m_childsByNames.emplace(std::pair(object->GetName(), i));
                }
            }
        }
        size_t res = 0;
        auto range = m_childsByNames.equal_range(name);
        for (auto i = range.first; i != range.second && i->second < index; ++i)
        {
            ++res;
        }
        return res;
    }
private:
    std::multimap<std::string, size_t> m_childsByNames;
};

inline Visitor::Visitor(const ISerializable* object) : m_currentSerializableObject(object)
{
    EXT_EXPECT(CheckObject(m_currentSerializableObject)) << "No info inside collection";
    EXT_EXPECT(UpdateObjectType()) << "Unknown collection type";
}

inline size_t Visitor::GetIndexAmongIdenticalNames(bool bCollectionStart)
{
    const char* curObjectName = m_currentSerializableObject.GetName();
    EXT_ASSERT(curObjectName);
    if (!curObjectName)
        return 0;

    size_t counter = 0;
    // check collection depth size, if it is collection start - m_collectionsDepth already increased, take penultimate element
    if ((!bCollectionStart && !m_collectionsDepth.empty()) || (bCollectionStart && m_collectionsDepth.size() > 1))
    {
        auto collectionInfoIt = m_collectionsDepth.rbegin();
        if (bCollectionStart)
            ++collectionInfoIt;

        counter = collectionInfoIt->NumberChildsWithSameNameBeforeIndex(curObjectName, collectionInfoIt->currentIndexInsideCollection);
    }

    return counter;
}

inline bool Visitor::GoToNextObject()
{
    switch (GetCurrentObjectType())
    {
    case ObjectType::eCollectionStart:
    {
        const ISerializableCollection* collection = m_currentSerializableObject;
        EXT_EXPECT(collection);

        if (collection->Size() == 0)
        {
            m_currentObjectType = eCollectionEnd;
            return true;
        }

        m_currentSerializableObject = collection->Get(0);

        if (!UpdateObjectType())
        {
            EXT_ASSERT(false) << "Can`t find collection element type, skip it";
            m_currentObjectType = ObjectType::eField;
            return GoToNextObject();
        }

        if (!CheckObject(m_currentSerializableObject))
        {
            EXT_ASSERT(false) << "Not serializable element inside collection, skip it";
            return GoToNextObject();
        }
    }
    break;
    case ObjectType::eCollectionEnd:
    {
        // reduce collection depth
        if (!m_collectionsDepth.empty())
            m_collectionsDepth.pop_back();

        if (m_collectionsDepth.empty())
            return false;

        // go to next field in collection
        const CollectionInfo& collectionInfo = m_collectionsDepth.back();
        m_currentSerializableObject = collectionInfo.collection->Get(collectionInfo.currentIndexInsideCollection);
        m_currentObjectType = ObjectType::eField;
        return GoToNextObject();
    }
    break;
    case ObjectType::eField:
    {
        if (m_collectionsDepth.empty())
            // only one field serializing
            return false;

        CollectionInfo& collectionInfo = m_collectionsDepth.back();

        // try find serializable element inside collection
        bool bGotSerializableObject = false;
        while (++collectionInfo.currentIndexInsideCollection < collectionInfo.sizeOfCollection)
        {
            m_currentSerializableObject = collectionInfo.collection->Get(collectionInfo.currentIndexInsideCollection);
            if (CheckObject(m_currentSerializableObject))
            {
                bGotSerializableObject = true;
                break;
            }
        }

        if (bGotSerializableObject)
        {
            if (!UpdateObjectType())
            {
                // go to next field
                m_currentObjectType = ObjectType::eField;
                return GoToNextObject();
            }
        }
        else
        {
            // collection end
            m_currentObjectType = ObjectType::eCollectionEnd;
            m_currentSerializableObject = collectionInfo.collection;
        }
    }
    break;
    case ObjectType::eOptional:
    {
        const ISerializableOptional* optional = m_currentSerializableObject;
        EXT_EXPECT(optional);
        m_currentSerializableObject = optional->Get();
        if (!UpdateObjectType())
        {
            // go to next field
            m_currentObjectType = ObjectType::eField;
            return GoToNextObject();
        }
    }
    break;
    default:
        EXT_UNREACHABLE();
    }

    return true;
}

inline void Visitor::UpdateCurrentCollectionSize()
{
    EXT_EXPECT(!m_collectionsDepth.empty()) << "The size of a collection that does not exist is updated";

    CollectionInfo& collectionInfo = m_collectionsDepth.back();
    collectionInfo.sizeOfCollection = collectionInfo.collection->Size();
}

inline bool Visitor::UpdateObjectType()
{
    if (const ISerializableOptional* optional = m_currentSerializableObject; optional)
        m_currentObjectType = ObjectType::eOptional;
    else if (const ISerializableCollection* collection = m_currentSerializableObject)
    {
        m_currentObjectType = ObjectType::eCollectionStart;

        // add to collections depth new collection
        if (m_currentSerializableObject.serializableObjectHolder)
            m_collectionsDepth.emplace_back(std::dynamic_pointer_cast<ISerializableCollection>(m_currentSerializableObject.serializableObjectHolder));
        else
            m_collectionsDepth.emplace_back(collection);
    }
    else if (const ISerializableField* field = m_currentSerializableObject; field)
        m_currentObjectType = ObjectType::eField;
    else
    {
        EXT_ASSERT(false) << "Unknown type of serializable object";
        return false;
    }

    return true;
}

template <class Type>
inline bool SerializeObject(const std::unique_ptr<ISerializer>& serializer, const Type& object)
{
    static_assert(ext::serializable::details::is_serializable_v<Type>, "Type not serializable, maybe you forgot to add REGISTER_SERIALIZABLE_OBJECT?");

    EXT_REQUIRE(serializer) << "No serializer passed";

    std::shared_ptr<ISerializableCollection> objectCollection;
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

    std::shared_ptr<SerializableNode> serializationTreeRoot;
    std::shared_ptr<SerializableNode> currentNode;

    Visitor objectsVisitor(serializableObject);
    do
    {
        switch (objectsVisitor.GetCurrentObjectType())
        {
        case Visitor::ObjectType::eCollectionStart:
        {
            if (!currentNode)
            {
                EXT_ASSERT(!serializationTreeRoot);
                serializationTreeRoot = std::make_shared<SerializableNode>(serializableObject->GetName());
                currentNode = serializationTreeRoot;
            }
            else
            {
                currentNode = currentNode->AddChild(objectsVisitor.GetCurrentObject()->GetName(), currentNode);
            }

            const auto* collection = dynamic_cast<const ISerializableCollection*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(collection);
            const_cast<ISerializableCollection*>(collection)->OnSerializationStart();
        }
        break;
        case Visitor::ObjectType::eCollectionEnd:
        {
            EXT_EXPECT(currentNode);
            EXT_ASSERT(currentNode->Name == objectsVisitor.GetCurrentObject()->GetName());
            currentNode = currentNode->Parent.lock();

            const auto* collection = dynamic_cast<const ISerializableCollection*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(collection);
            const_cast<ISerializableCollection*>(collection)->OnSerializationEnd();
        }
        break;
        case Visitor::ObjectType::eField:
        {
            const auto* field = dynamic_cast<const ISerializableField*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(field);

            const_cast<ISerializableField*>(field)->OnSerializationStart();
            if (!currentNode)
            {
                EXT_ASSERT(!serializationTreeRoot);
                serializationTreeRoot = std::make_shared<SerializableNode>(serializableObject->GetName());
                currentNode = serializationTreeRoot;
            }
            else
            {
                currentNode->AddChild(field->GetName(), currentNode)->Value = field->SerializeValue();
            }
            const_cast<ISerializableField*>(field)->OnSerializationEnd();
        }
        break;
        case Visitor::ObjectType::eOptional:
        break;
        default:
            EXT_UNREACHABLE();
        }

    } while (objectsVisitor.GoToNextObject());
    EXT_EXPECT(serializationTreeRoot);

    return serializer->Serialize(serializationTreeRoot);
}

template <class Type>
inline bool DeserializeObject(const std::unique_ptr<serializer::IDeserializer>& deserializer, Type& object)
{
    static_assert(ext::serializable::details::is_serializable_v<Type>, "Type not serializable, maybe you forgot to add REGISTER_SERIALIZABLE_OBJECT?");
    
    EXT_REQUIRE(deserializer) << "No deserializer passed";

    std::shared_ptr<ISerializableCollection> objectCollection;
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

    std::shared_ptr<SerializableNode> deserializationTreeRoot;
    if (!deserializer->Deserialize(deserializationTreeRoot))
        return false;

    std::shared_ptr<SerializableNode> currentNode;
    Visitor objectsVisitor(deserializableObject);

    do
    {
        switch (objectsVisitor.GetCurrentObjectType())
        {
        case Visitor::ObjectType::eCollectionStart:
        {
            const auto* collection = dynamic_cast<const ISerializableCollection*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(collection);

            std::shared_ptr<SerializableNode> childNode;
            if (!currentNode)
            {
                childNode = deserializationTreeRoot->Name == collection->GetName() ? deserializationTreeRoot : nullptr;
                currentNode = deserializationTreeRoot;
            }
            else
                childNode = currentNode->GetChild(collection->GetName(), objectsVisitor.GetIndexAmongIdenticalNames(true));

            if (childNode)
            {
                const_cast<ISerializableCollection*>(collection)->OnDeserializationStart(childNode);
                objectsVisitor.UpdateCurrentCollectionSize();

                currentNode = childNode;
            }
            else
            {
                EXT_ASSERT(false) << "Can`t find node for collection " << collection->GetName();
                objectsVisitor.SkipCollectionContent();
            }
        }
        break;
        case Visitor::ObjectType::eCollectionEnd:
        {
            EXT_EXPECT(currentNode);
            EXT_ASSERT(currentNode->Name == objectsVisitor.GetCurrentObject()->GetName()) << "Invalid name for cuurent collection";

            const auto* collection = dynamic_cast<const ISerializableCollection*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(collection);
            const_cast<ISerializableCollection*>(collection)->OnDeserializationEnd();

            currentNode = currentNode->Parent.lock();
        }
        break;
        case Visitor::ObjectType::eField:
        {
            const auto* field = dynamic_cast<const ISerializableField*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(field);

            std::shared_ptr<SerializableNode> childNode;
            if (!currentNode)
            {
                childNode = deserializationTreeRoot->Name == field->GetName() ? deserializationTreeRoot : nullptr;
                currentNode = deserializationTreeRoot;
            }
            else
                childNode = currentNode->GetChild(field->GetName(), objectsVisitor.GetIndexAmongIdenticalNames(false));

            if (childNode)
            {
                auto* deserializeField = const_cast<ISerializableField*>(field);
                deserializeField->OnDeserializationStart(childNode);
                if (childNode->Value.has_value())
                    deserializeField->DeserializeValue(childNode->Value.value());
                else
                    EXT_ASSERT(false) << "Can`t find value for field " << field->GetName();
                deserializeField->OnDeserializationEnd();
            }
            else
                EXT_ASSERT(false) << "Can`t find node for field " << field->GetName();
        }
        break;
        case Visitor::ObjectType::eOptional:
        {
            const auto* optionalField = dynamic_cast<const ISerializableOptional*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(optionalField);

            std::shared_ptr<SerializableNode> childNode;
            if (!currentNode)
            {
                childNode = deserializationTreeRoot->Name == optionalField->GetName() ? deserializationTreeRoot : nullptr;
                currentNode = deserializationTreeRoot;
            }
            else
                childNode = currentNode->GetChild(optionalField->GetName(), objectsVisitor.GetIndexAmongIdenticalNames(false));

            if (childNode)
            {
                // Just call OnDeserializationStart to create object in SerializableOptional
                auto* field = const_cast<ISerializableOptional*>(optionalField);
                field->OnDeserializationStart(childNode);
            }
            else
                EXT_ASSERT(false) << "Can`t find node for field " << optionalField->GetName();
        }
        break;
        default:
            EXT_UNREACHABLE();
        }

    } while (objectsVisitor.GoToNextObject());

    return true;
}

} // namespace ext::serializer
