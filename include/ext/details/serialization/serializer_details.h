#pragma once

/*
* Implementation for ext/serialization/serializer.h
*/

#include <string.h>

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

        for (size_t index = 0; index < collectionInfoIt->currentIndexInsideCollection; ++index)
        {
            if (const auto object = collectionInfoIt->collection->Get(index); object)
            {
                if (strcmp(object->GetName(), curObjectName) == 0)
                    ++counter;
            }
        }
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
    if (const ISerializableOptional* optional = m_currentSerializableObject)
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
    else if (const ISerializableField* field = m_currentSerializableObject)
        m_currentObjectType = ObjectType::eField;
    else
    {
        EXT_ASSERT(false) << "Unknown type of serializable object";
        return false;
    }

    return true;
}

inline bool Executor::SerializeObject(const std::unique_ptr<ISerializer>& serializer, const ISerializable* object)
{
    EXT_REQUIRE(serializer && object) << "didn't pass what to serialize";

    std::shared_ptr<SerializableNode> serializationTreeRoot;
    std::shared_ptr<SerializableNode> currentNode;

    Visitor objectsVisitor(object);
    do
    {
        switch (objectsVisitor.GetCurrentObjectType())
        {
        case Visitor::ObjectType::eCollectionStart:
        {
            if (!currentNode)
            {
                EXT_ASSERT(!serializationTreeRoot);
                serializationTreeRoot = std::make_shared<SerializableNode>(object->GetName());
                currentNode = serializationTreeRoot;
            }
            else
            {
                currentNode->ChildNodes.emplace_back(std::make_shared<SerializableNode>(objectsVisitor.GetCurrentObject()->GetName(), currentNode));
                currentNode = currentNode->ChildNodes.back();
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
            if (!currentNode)
            {
                EXT_ASSERT(!serializationTreeRoot);
                serializationTreeRoot = std::make_shared<SerializableNode>(object->GetName());
                currentNode = serializationTreeRoot;
            }
            else
            {
                currentNode->ChildNodes.emplace_back(std::make_shared<SerializableNode>(field->GetName(), currentNode));
                currentNode->ChildNodes.back()->Value = field->SerializeValue();
            }
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

inline bool Executor::DeserializeObject(const std::unique_ptr<serializer::IDeserializer>& deserializer, ISerializable* object)
{
    EXT_REQUIRE(deserializer && object) << "didn't pass what to deserialize";;

    std::shared_ptr<SerializableNode> deserializationTreeRoot;
    if (!deserializer->Deserialize(deserializationTreeRoot))
        return false;

    object->PrepareToDeserialize(deserializationTreeRoot);

    std::shared_ptr<SerializableNode> currentNode;
    Visitor objectsVisitor(object);

    do
    {
        switch (objectsVisitor.GetCurrentObjectType())
        {
        case Visitor::ObjectType::eCollectionStart:
        {
            const auto* collection = dynamic_cast<const ISerializableCollection*>(objectsVisitor.GetCurrentObject());
            EXT_EXPECT(collection);
            const_cast<ISerializableCollection*>(collection)->OnDeserializationStart();

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
                const_cast<ISerializableCollection*>(collection)->PrepareToDeserialize(childNode);
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
                deserializeField->PrepareToDeserialize(childNode);
                if (childNode->Value.has_value())
                    deserializeField->DeserializeValue(childNode->Value.value());
                else
                    EXT_ASSERT(false) << "Can`t find value for field " << field->GetName();
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
                auto* field = const_cast<ISerializableOptional*>(optionalField);
                field->PrepareToDeserialize(childNode);
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
