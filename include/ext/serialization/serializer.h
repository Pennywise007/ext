#pragma once

/*
    Interfaces and factory for upload/download/data transfer
    Currently used only for unloading the download in XML and text
    If necessary, it can be extended to save settings to DB/JSON/File
*/

#include <filesystem>
#include <memory>
#include <list>
#include <stack>
#include <string>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/serialization/iserializable.h>

namespace ext::serializer {

using ISerializable = ext::serializable::ISerializable;
using ISerializableField = ext::serializable::ISerializableField;
using ISerializableCollection = ext::serializable::ISerializableCollection;
using ISerializableOptional = ext::serializable::ISerializableOptional;

using SerializableNode = ext::serializable::SerializableNode;
using SerializableValue = ext::serializable::SerializableValue;

// Serialization object interface
struct ISerializer
{
    virtual ~ISerializer() = default;
    // Serialize tree
    [[nodiscard]] virtual bool Serialize(const std::shared_ptr<SerializableNode>& serializationTreeRoot) EXT_THROWS() = 0;
};

// Deserialization object interface
struct IDeserializer
{
    virtual ~IDeserializer() = default;
    // Deserialize tree data
    [[nodiscard]] virtual bool Deserialize(std::shared_ptr<SerializableNode>& deserializationTreeRoot) EXT_THROWS() = 0;
};

// Serializer interfaces factory
struct Factory
{
// Predefine USE_PUGI_XML and add pugi XML lib to project if you want serialize to/from xml.
#ifdef USE_PUGI_XML
    // Create serializer to/from XML file.
    [[nodiscard]] static std::unique_ptr<ISerializer> XMLSerializer(const std::filesystem::path& filePath);
    [[nodiscard]] static std::unique_ptr<IDeserializer> XMLDeserializer(const std::filesystem::path& filePath);
#endif

    // Create serializer to/from text.
    [[nodiscard]] static std::unique_ptr<ISerializer> TextSerializer(std::wstring& outputText);
    [[nodiscard]] static std::unique_ptr<IDeserializer> TextDeserializer(const std::wstring& inputText) EXT_THROWS();
};

/// <summary>Serialize object.</summary>
/// <param name="serializer">Serialization interface, @see Factory.</param>
/// <param name="object">Serializable object.</param>
/// <returns>True if serialization successfully, may throw exception.</returns>
template <class Type>
static bool SerializeObject(const std::unique_ptr<ISerializer>& serializer, const Type& object) EXT_THROWS();

/// <summary>Deserialize object.</summary>
/// <param name="deserializer">Deserialization interface, @see Factory.</param>
/// <param name="object">Serializable object.</param>
/// <returns>True if deserialization successfully, may throw exception.</returns>
template <class Type>
static bool DeserializeObject(const std::unique_ptr<IDeserializer>& deserializer, Type& object) EXT_THROWS();

// Visitor class by fields and collections of the object being serialized
// Used to unify iteration over serializable objects
class Visitor
{
public:
    explicit Visitor(const ISerializable* rootObject) EXT_THROWS();

public:
    // Current serializable object type
    enum ObjectType
    {
        eCollectionStart = 0,   // Start of current collection
        eCollectionEnd,         // End of current collection
        eField,                 // Field
        eOptional               // Optional field
    };
    // Get type of current visitor serializable object
    [[nodiscard]] ObjectType GetCurrentObjectType() const noexcept { return m_currentObjectType; }
    // Get current serializable object
    [[nodiscard]] const ISerializable* GetCurrentObject() const noexcept { return m_currentSerializableObject; }
    // Get the number of identical names in the collection for the current object
    // in case several objects with the same name have been serialized
    [[nodiscard]] size_t GetIndexAmongIdenticalNames(bool bCollectionStart);
    // Move to next serializable object
    [[nodiscard]] bool GoToNextObject();
    // Skip the entire collection
    void SkipCollectionContent() noexcept { m_currentObjectType = ObjectType::eCollectionEnd; }
    // Reread the size of the current collection
    void UpdateCurrentCollectionSize();

private:
    // Update the current object type
    bool UpdateObjectType();
    // Check the object for the possibility of serialization
    [[nodiscard]] static bool CheckObject(const ISerializable* object) { return object && object->GetName(); }

private:
    ObjectType m_currentObjectType = ObjectType::eCollectionStart;
    struct CurrentSerializableObject
    {
        const ISerializable* serializableObjectPointer;
        // we need to keep serializable objects on smart pointer, but root object can be created like ordinary object
        std::shared_ptr<ISerializable> serializableObjectHolder;

        CurrentSerializableObject(const ISerializable* object) : serializableObjectPointer(object) {}
        CurrentSerializableObject& operator=(const std::shared_ptr<ISerializable>& object)
        {
            serializableObjectHolder = object;
            serializableObjectPointer = object.get();
            return *this;
        }

        operator const ISerializable*() const { return serializableObjectPointer; }
        operator const ISerializableField*() const { return dynamic_cast<const ISerializableField*>(serializableObjectPointer); }
        operator const ISerializableCollection*() const { return dynamic_cast<const ISerializableCollection*>(serializableObjectPointer); }
        operator const ISerializableOptional*() const { return dynamic_cast<const ISerializableOptional*>(serializableObjectPointer); }

        [[nodiscard]] const char* GetName() const noexcept { return serializableObjectPointer->GetName(); }

    } m_currentSerializableObject;

    struct CollectionInfo;
    // the current roughness in nested collections, the last item is the current level
    std::list<CollectionInfo> m_collectionsDepth;
};

// Help interface for serializing SerializableNode tree, @see SerializableVisitor, can be inherited by serializers
struct INodeSerializer
{
    /// <summary>
    /// Write start of collection
    /// </summary>
    /// <param name="name">Collection name</param>
    /// <param name="collectionLevel">Level of current collection based on root(indent)</param>
    virtual void WriteCollectionStart(const std::string& name, const size_t& collectionLevel) = 0;
    /// <summary>
    /// Write collection end
    /// </summary>
    /// <param name="name">Collection name</param>
    /// <param name="collectionLevel">Level of current collection based on root(indent)</param>
    /// <param name="nextFieldExist">Flag signifying that on current collection level will be another objects</param>
    virtual void WriteCollectionEnd(const std::string& name, const size_t& collectionLevel, bool nextFieldExist) = 0;
    /// <summary>
    /// Write field
    /// </summary>
    /// <param name="name">Field name</param>
    /// <param name="value">Field value</param>
    /// <param name="fieldLevel">Level of current collection based on root(indent)</param>
    /// <param name="nextFieldExist">Flag signifying that on current collection level will be another objects</param>
    virtual void WriteField(const std::string& name, const SerializableValue& value, const size_t& fieldLevel, bool nextFieldExist) = 0;
};

// Help class for serializing SerializableNode tree
struct TreeSerializer
{
    static void SerializeTree(const std::shared_ptr<SerializableNode>& serializationTreeRoot, INodeSerializer* serializer)
    {
        std::shared_ptr<SerializableNode> currentNode = serializationTreeRoot;
        std::stack<std::pair<size_t, size_t>> childLevelInfo;
        childLevelInfo.push(std::make_pair(currentNode->CountChilds(), 0));

        while (currentNode)
        {
            EXT_ASSERT(!currentNode->Name.empty()) << "Must be value OR child nodes";

            if (!currentNode->Value.has_value())
            {
                serializer->WriteCollectionStart(currentNode->Name, childLevelInfo.size() - 1);

                if (!currentNode->HasChilds())
                {
                    const auto prevNode = currentNode;
                    const bool nextNodeExist = GoToNextChild(childLevelInfo, currentNode);
                    serializer->WriteCollectionEnd(prevNode->Name, childLevelInfo.size() - 1, nextNodeExist);
                    if (!nextNodeExist)
                        GoToNextNodeByParent(childLevelInfo, currentNode, serializer);
                }
                else
                    GoToChild(childLevelInfo, currentNode, 0);
            }
            else
            {
                const auto prevNode = currentNode;
                const bool nextNodeExist = GoToNextChild(childLevelInfo, currentNode);
                serializer->WriteField(prevNode->Name, prevNode->Value.value(), childLevelInfo.size() - 1, nextNodeExist);
                if (!nextNodeExist)
                    GoToNextNodeByParent(childLevelInfo, currentNode, serializer);
            }
        }
    }

private:
    // Find child element inside current node child collection
    static void GoToChild(std::stack<std::pair<size_t, size_t>>& childLevelInfo, std::shared_ptr<SerializableNode>& currentNode, const size_t& index)
    {
        childLevelInfo.push(std::make_pair(currentNode->CountChilds(), 0));
        EXT_ASSERT(index < childLevelInfo.top().first) << "Iterate over list size";
        currentNode = currentNode->GetChild(index);
    }

    // Try to go to next child on current child level
    static bool GoToNextChild(std::stack<std::pair<size_t, size_t>>& childLevelInfo, std::shared_ptr<SerializableNode>& currentNode)
    {
        const auto parentNode = currentNode->Parent.lock();
        if (!parentNode || ++childLevelInfo.top().second >= childLevelInfo.top().first)
            return false;

        currentNode = parentNode->GetChild(childLevelInfo.top().second);
        return true;
    }

    // Go to parent node from current
    static bool GoToParentNode(std::stack<std::pair<size_t, size_t>>& childLevelInfo, std::shared_ptr<SerializableNode>& currentNode)
    {
        EXT_ASSERT(!childLevelInfo.empty());
        childLevelInfo.pop();
        currentNode = currentNode->Parent.lock();
        return currentNode != nullptr;
    }

    // Find next node to serialize
    static void GoToNextNodeByParent(std::stack<std::pair<size_t, size_t>>& childLevelInfo,
                                     std::shared_ptr<SerializableNode>& currentNode,
                                     INodeSerializer* serializer)
    {
        while (GoToParentNode(childLevelInfo, currentNode))
        {
            const auto prevNode = currentNode;
            if (GoToNextChild(childLevelInfo, currentNode))
            {
                serializer->WriteCollectionEnd(prevNode->Name, childLevelInfo.size() - 1, true);
                return;
            }
            serializer->WriteCollectionEnd(currentNode->Name, childLevelInfo.size() - 1, false);
        }
    }
};

} // namespace ext::serializer

#include <ext/details/serialization/serializer_details.h>