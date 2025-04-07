
/*
    Serialization of objects

    Implementation of base classes to simplify serialization / deserialization of data

    Usage example:

#if C++20 // we use reflection to get fields info, no macro needed, to use base classes you need to use REGISTER_SERIALIZABLE_OBJECT
struct Settings
{
    struct User
    {
        std::int64_t id;
        std::string firstName;
        std::string userName;
    };
    
    std::wstring password;
    std::list<User> registeredUsers;
};

Settings settings;

std::wstring json;
try {
    SerializeToJson(settings, json);
}
catch (...) {
    ext::ManageException(EXT_TRACE_FUNCTION);
}
...
try {
    DeserializeFromJson(settings, json);
}
catch (...) {
    ext::ManageException(EXT_TRACE_FUNCTION);
}

#endif // C++20

struct InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(long, value);
    DECLARE_SERIALIZABLE_FIELD(std::list<int>, valueList);
};

struct CustomValue : ISerializableValue {
// ISerializableValue
    [[nodiscard]] SerializableValue SerializeValue() const override { return std::to_wstring(val); }
    void DeserializeValue(const SerializableValue& value) override { val = std::wtoi(value); }
    int val = 10;
};

struct TestStruct : InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT(InternalStruct);

    DECLARE_SERIALIZABLE_FIELD(long, valueLong, 2);
    DECLARE_SERIALIZABLE_FIELD(int, valueInt);
    DECLARE_SERIALIZABLE_FIELD(std::vector<bool>, boolVector, { true, false });

    DECLARE_SERIALIZABLE_FIELD(CustomValue, value);
    DECLARE_SERIALIZABLE_FIELD(InternalStruct, internalStruct);

    // Instead of using macroses - use REGISTER_SERIALIZABLE_FIELD in constructor
    std::list<int> m_listOfParams;

    MyTestStruct()
    {
        REGISTER_SERIALIZABLE_FIELD(m_listOfParams); // or use DECLARE_SERIALIZABLE_FIELD macro

        if (!std::filesystem::exists(kFilePath))
            return;
        try
        {
            std::wifstream file(kFilePath);
            EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
            EXT_DEFER(file.close());

            const std::wstring json{ std::istreambuf_iterator<wchar_t>(file),
                                        std::istreambuf_iterator<wchar_t>() };

            ext::serializer::DeserializeFromJson(settings, json);
        }
        catch (const std::exception&)
        {
            ext::ManageException("Failed to load settings");
        }
    }

    ~MyTestStruct()
    {
        try
        {
            std::wstring json;
            ext::serializer::SerializeToJson(*this, json);

            std::wofstream file(kFullFileName);
            EXT_CHECK(file.is_open()) << "Failed to open file " << kFileName;
            EXT_DEFER(file.close());
            file << json;
        }
        catch (const std::exception&)
        {
            ext::ManageException("Failed to save settings");
        }
    }
};

You can also declare this functions in your REGISTER_SERIALIZABLE_OBJECT object to get notified when (de)serialization was called:
    // Called before collection serialization
    void OnSerializationStart() {}
    // Called after collection serialization
    void OnSerializationEnd() {};

    // Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allow to add upgrade for old stored settings
    // Also used to allocate collections elements
    void OnDeserializationStart(SerializableNode& serializableTree) {}
    // Called after collection deserialization
    void OnDeserializationEnd() {};
*/

#pragma once

#include <filesystem>
#include <memory>
#include <list>
#include <stack>
#include <string>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/reflection/enum.h>

#include <ext/serialization/iserializable.h>

namespace ext::serializer {

/// <summary>Serialize object to json string, in case of error - throws exception</summary>
/// <param name="object">Serializable object.</param>
/// <param name="outputJson">Json with serialization result.</param>
/// <param name="indentText">Used for pretty formatting, indent text for each ocollection level</param>
/// <param name="addNewLines">Used for pretty formatting, will place object fields on new lines with indent from indentText param.</param>
template <class Type>
static void SerializeToJson(
    const Type& object,
    std::wstring& outputJson,
    std::wstring indentText = std::wstring(4, L' '),
    bool addNewLines = true) EXT_THROWS();

/// <summary>Deserialize object, in case of error - throws exception.</summary>
/// <param name="object">Serializable object.</param>
/// <param name="json">Json text which contain object.</param>
template <class Type>
static void DeserializeFromJson(Type& object, const std::wstring& json) EXT_THROWS();

using ext::serializable::ISerializable;
using ext::serializable::ISerializableValue;
using ext::serializable::ISerializableField;
using ext::serializable::ISerializableArray;
using ext::serializable::ISerializableObject;
using ext::serializable::ISerializableOptional;

using ext::serializable::SerializableNode;
using ext::serializable::SerializableValue;

// Serialization object interface
struct ISerializer
{
    virtual ~ISerializer() = default;
    // Serialize tree
    virtual void Serialize(const SerializableNode& serializationTreeRoot) EXT_THROWS() = 0;
};

// Deserialization object interface
struct IDeserializer
{
    virtual ~IDeserializer() = default;
    // Deserialize tree data
    [[nodiscard]] virtual SerializableNode& Deserialize() EXT_THROWS() = 0;
};

/// <summary>Serialize object, in case of error - throws exception</summary>
/// <param name="serializer">Serialization interface, @see Factory.</param>
/// <param name="object">Serializable object.</param>
template <class Type>
static void SerializeObject(const std::unique_ptr<ISerializer>& serializer, const Type& object) EXT_THROWS();

/// <summary>Deserialize object, in case of error - throws exception.</summary>
/// <param name="deserializer">Deserialization interface, @see Factory.</param>
/// <param name="object">Serializable object.</param>
template <class Type>
static void DeserializeObject(const std::unique_ptr<IDeserializer>& deserializer, Type& object) EXT_THROWS();

// Visitor class by fields and collections of the object being serialized
// Used to unify iteration over serializable objects
class Visitor
{
public:
    explicit Visitor(ISerializable* rootObject) EXT_THROWS();

public:
    // Current serializable object type
    enum ObjectType
    {
        eObjectStart = 0,       // Start of the object
        eObjectEnd,             // End of the object
        eFieldStart,            // Start field of the collection
        eFieldEnd,              // End Field of the collection
        eArrayStart,            // Start of the array
        eArrayEnd,              // End of the array
        eOptionalStart,         // Optional object start
        eOptionalEnd,           // Optional object end
        eValue,                 // Value, array value or field value
    };
    // Get type of current visitor serializable object
    [[nodiscard]] ObjectType GetCurrentObjectType() const noexcept { return m_currentSerializableType; }
    // Get current serializable object
    [[nodiscard]] ISerializable* GetCurrentObject() const noexcept { return static_cast<ISerializable*>(m_currentSerializableObject); }
    // Notification that we need to cache nodes on the current collection level, used for the fields search speed up
    void CacheChildNodeNames();
    // Get current object index inside parents collection
    [[nodiscard]] size_t GetCurrentIndexInCollection() const;
    // Get the number of identical names in the collection for the current object
    // in case several objects with the same name have been serialized
    [[nodiscard]] size_t GetIndexAmongIdenticalNames();
    // Move to next serializable object
    [[nodiscard]] bool GoToNextObject();
    // Skip the entire collection/array
    void SkipCollectionContent() noexcept;
    // Reread the size of the current collection
    void UpdateCurrentCollectionSize();
    // Getting information about current object
    std::string GetCurrentObjectTreeDescription() const;

private:
    // Update the current object type
    void UpdateObjectType() EXT_THROWS();
    void MoveToCollectionEnd(ISerializable* collection) EXT_THROWS();

private:
    ObjectType m_currentSerializableType = ObjectType::eObjectStart;
    struct CurrentSerializableObject
    {
        ISerializable* serializableObjectPointer;
        // we need to keep serializable objects on smart pointer, but root object can be created like ordinary object
        std::shared_ptr<ISerializable> serializableObjectHolder;

        CurrentSerializableObject(ISerializable* object) : serializableObjectPointer(object) {}
        CurrentSerializableObject& operator=(const std::shared_ptr<ISerializable>& object)
        {
            serializableObjectHolder = object;
            serializableObjectPointer = object.get();
            return *this;
        }

        operator ISerializable*() const { return serializableObjectPointer; }
        operator ISerializableValue*() const { return dynamic_cast<ISerializableValue*>(serializableObjectPointer); }
        operator ISerializableField*() const { return dynamic_cast<ISerializableField*>(serializableObjectPointer); }
        operator ISerializableArray*() const { return dynamic_cast<ISerializableArray*>(serializableObjectPointer); }
        operator ISerializableObject*() const { return dynamic_cast<ISerializableObject*>(serializableObjectPointer); }
        operator ISerializableOptional*() const { return dynamic_cast<ISerializableOptional*>(serializableObjectPointer); }

        // Check the object for the possibility of serialization
        [[nodiscard]] bool Valid() const noexcept { return !!serializableObjectPointer; }
    } m_currentSerializableObject;

    struct CollectionInfo;
    // the current roughness in nested collections, the last item is the current level
    std::list<CollectionInfo> m_collectionsDepth;
};

// Help interface for serializing SerializableNode tree, @see SerializableVisitor, can be inherited by serializers
struct INodeSerializer
{
    /// <summary>
    /// Write object start
    /// </summary>
    /// <param name="indentLevel">Indent of current object based on root</param>
    /// <param name="emptyObject">flag that object doesn't have fields</param>
    virtual void WriteObjectStart(size_t indentLevel, bool emptyObject) = 0;
    /// <summary>
    /// Write object end
    /// </summary>
    /// <param name="indentLevel">Indent of current object based on root</param>
    /// <param name="emptyObject">flag that object doesn't have fields</param>
    /// <param name="nextObjectExist">Flag signifying that on current level will be another objects</param>
    virtual void WriteObjectEnd(size_t indentLevel, bool emptyObject, bool nextObjectExist) = 0;
    /// <summary>
    /// Write object field start
    /// </summary>
    /// <param name="name">Field name</param>
    /// <param name="indentLevel">Indent of field based on root</param>
    virtual void WriteFieldStart(const SerializableValue& name, size_t indentLevel) = 0;
    /// <summary>
    /// Write object field end
    /// </summary>
    /// <param name="name">Field name</param>
    /// <param name="indentLevel">Indent of field based on root</param>
    /// <param name="nextObjectExist">Flag signifying that on current level will be another objects</param>
    virtual void WriteFieldEnd(size_t indentLevel, bool nextObjectExist) = 0;
    /// <summary>
    /// Write start of array
    /// </summary>
    /// <param name="indentLevel">Indent of current collection based on root</param>
    /// <param name="emptyArray">flag that array is empty</param>
    virtual void WriteArrayStart(size_t indentLevel, bool emptyArray) = 0;
    /// <summary>
    /// Write array end
    /// </summary>
    /// <param name="indentLevel">Indent of current array based on root</param>
    /// <param name="nextObjectExist">Flag signifying that on current level will be another objects</param>
    /// <param name="emptyArray">flag that array is empty</param>
    virtual void WriteArrayEnd(size_t indentLevel, bool emptyArray, bool nextObjectExist) = 0;
    /// <summary>
    /// Write value
    /// </summary>
    /// <param name="value">Field value</param>
    /// <param name="indentLevel">Indent of element based on root</param>
    /// <param name="nextObjectExist">Flag signifying that on current level will be another objects</param>
    virtual void WriteValue(const SerializableValue& value, size_t indentLevel, bool nextObjectExist) = 0;
};

// Help class for serializing SerializableNode tree
struct TreeSerializer
{
    static void SerializeTree(const SerializableNode& serializationTreeRoot, INodeSerializer* serializer)
    {
        SerializableNode* currentNode = &const_cast<SerializableNode&>(serializationTreeRoot);
        std::stack<std::pair<size_t, size_t>> childLevelInfo;
        childLevelInfo.push(std::make_pair(currentNode->CountChilds(), 0));

        while (currentNode)
        {
            switch (currentNode->Type)
            {
            case SerializableNode::NodeType::eObject:
            {
                serializer->WriteObjectStart(childLevelInfo.size() - 1, currentNode->CountChilds() == 0);
                
                if (currentNode->CountChilds() == 0)
                {
                    const bool nextNodeExist = GoToNextChild(childLevelInfo, currentNode);
                    serializer->WriteObjectEnd(childLevelInfo.size() - 1, true, nextNodeExist);
                    if (!nextNodeExist)
                        GoToNextNodeByParent(childLevelInfo, currentNode, serializer);
                }
                else
                    GoToChild(childLevelInfo, currentNode, 0);
            }
            break;
            case SerializableNode::NodeType::eArray:
            {
                serializer->WriteArrayStart(childLevelInfo.size() - 1, currentNode->CountChilds() == 0);
                
                if (currentNode->CountChilds() == 0)
                {
                    const bool nextNodeExist = GoToNextChild(childLevelInfo, currentNode);
                    serializer->WriteArrayEnd(childLevelInfo.size() - 1, true, nextNodeExist);
                    if (!nextNodeExist)
                        GoToNextNodeByParent(childLevelInfo, currentNode, serializer);
                }
                else
                    GoToChild(childLevelInfo, currentNode, 0);
            }
            break;
            case SerializableNode::NodeType::eField:
            {
                EXT_EXPECT(currentNode->CountChilds() == 1) << "Field should have only 1 child node with value";
                serializer->WriteFieldStart(currentNode->Name.value(), childLevelInfo.size() - 1);
                GoToChild(childLevelInfo, currentNode, 0);
            }
            break;
            case SerializableNode::NodeType::eValue:
            {
                const auto* prevNode = currentNode;
                const bool nextNodeExist = GoToNextChild(childLevelInfo, currentNode);
                serializer->WriteValue(prevNode->Value.value(), childLevelInfo.size() - 1, nextNodeExist);
                if (!nextNodeExist)
                    GoToNextNodeByParent(childLevelInfo, currentNode, serializer);
            }
            break;
            default:
                EXT_EXPECT(false) << "Unknown node type";
                static_assert(ext::reflection::get_enum_size<SerializableNode::NodeType>() == 4, "Unhandled enum value");
            }
        }
    }

private:
    // Find child element inside current node child collection
    static void GoToChild(std::stack<std::pair<size_t, size_t>>& childLevelInfo, SerializableNode*& currentNode, const size_t& index)
    {
        childLevelInfo.push(std::make_pair(currentNode->CountChilds(), 0));
        EXT_ASSERT(index < childLevelInfo.top().first) << "Iterate over list size";
        currentNode = currentNode->GetChild(index);
    }

    // Try to go to next child on current child level
    static bool GoToNextChild(std::stack<std::pair<size_t, size_t>>& childLevelInfo, SerializableNode*& currentNode)
    {
        const auto* parentNode = currentNode->Parent;
        if (!parentNode || ++childLevelInfo.top().second >= childLevelInfo.top().first)
            return false;

        currentNode = parentNode->GetChild(childLevelInfo.top().second);
        return true;
    }

    // Go to parent node from current
    static bool GoToParentNode(std::stack<std::pair<size_t, size_t>>& childLevelInfo, SerializableNode*& currentNode)
    {
        EXT_ASSERT(!childLevelInfo.empty());
        childLevelInfo.pop();
        currentNode = const_cast<SerializableNode*>(currentNode->Parent);
        return currentNode != nullptr;
    }

    // Find next node to serialize
    static void GoToNextNodeByParent(std::stack<std::pair<size_t, size_t>>& childLevelInfo,
                                     SerializableNode*& currentNode,
                                     INodeSerializer* serializer)
    {
        const auto writeCollectionEnd = [&](const SerializableNode* node, bool nextElementExist)
        {
            switch (node->Type)
            {
            case SerializableNode::NodeType::eObject:
                serializer->WriteObjectEnd(childLevelInfo.size() - 1, false, nextElementExist);
                break;
            case SerializableNode::NodeType::eArray:
                serializer->WriteArrayEnd(childLevelInfo.size() - 1, false, nextElementExist);
                break;
            case SerializableNode::NodeType::eField:
                serializer->WriteFieldEnd(childLevelInfo.size() - 1, nextElementExist);
                break;
            default:
                EXT_EXPECT(false) << "Unknown parent node type " << ext::reflection::enum_to_string(node->Type);
            }
        };

        while (GoToParentNode(childLevelInfo, currentNode))
        {
            const auto* prevNode = currentNode;
            if (GoToNextChild(childLevelInfo, currentNode))
            {
                writeCollectionEnd(prevNode, true);
                return;
            }

            writeCollectionEnd(currentNode, false);
        }
    }
};

} // namespace ext::serializer

#include <ext/details/serialization/serializer_details.h>
