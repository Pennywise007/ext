#pragma once

#include <filesystem>
#include <optional>
#include <wchar.h>

#ifndef PUGIXML_WCHAR_MODE
#error "You need to predefine PUGIXML_WCHAR_MODE, xml serializer works only with wchar_t"
#endif
#include <pugixml.hpp>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/scope/on_exit.h>
#include <ext/serialization/serializer.h>

namespace ext::serializer {

////////////////////////////////////////////////////////////////////////////////
// Сериализатор объектов в XML
class SerializerXML : public ISerializer, public INodeSerializer, public IDeserializer
{
public:
    SerializerXML(std::filesystem::path filePath) : m_filePath(std::move(filePath)) {}

protected:
// ISerializer
    [[nodiscard]] bool Serialize(const std::shared_ptr<SerializableNode>& serializationTreeRoot) EXT_THROWS() override;
// INodeSerializer
    void WriteCollectionStart(const std::string& name, const size_t& collectionLevel) override;
    void WriteCollectionEnd(const std::string& name, const size_t& collectionLevel, bool nextFieldExist) override;
    void WriteField(const std::string& name, const SerializableValue& value, const size_t& fieldLevel, bool nextFieldExist) override;

// IDeserializer
    [[nodiscard]] bool Deserialize(std::shared_ptr<SerializableNode>& deserializationTreeRoot) EXT_THROWS() override;

public:
    // Установить значение в ноду
    static void SetValue(pugi::xml_node xmlNode, const wchar_t* value);
    // Получения значение из ноды
    static const wchar_t* GetValue(const pugi::xml_node& xmlNode);

    // Добавление значения атрибута в ноду по имени
    static void SetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, const wchar_t* attributeValue);
    static void SetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, const char* attributeValue);
    // Получение значения атрибута из ноды
    static bool GetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, std::wstring& attributeValue);
    static bool GetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, std::string& attributeValue);

private:
    static constexpr auto kAttributeName = L"Name";
    static constexpr auto kAttributeValue = L"Value";
    static constexpr auto kAttributeType = L"Type";

    static constexpr auto kNodeCollection = L"Collection";
    static constexpr auto kNodeField = L"Field";

    const std::filesystem::path m_filePath;
    std::optional<pugi::xml_node> m_currentSerializationNode;
};

inline bool SerializerXML::Serialize(const std::shared_ptr<SerializableNode>& serializationTreeRoot) EXT_THROWS()
{
    pugi::xml_document document;
    m_currentSerializationNode = document;
    EXT_SCOPE_ON_EXIT({ m_currentSerializationNode.reset(); });

    EXT_REQUIRE(serializationTreeRoot) << "Root node uninitialized!";
    TreeSerializer::SerializeTree(serializationTreeRoot, this);

    return document.save_file(m_filePath.c_str(), PUGIXML_TEXT("    "), pugi::format_default, pugi::encoding_utf8);;
}

inline void SerializerXML::WriteCollectionStart(const std::string& name, const size_t& /*collectionLevel*/)
{
    EXT_REQUIRE(m_currentSerializationNode) << "No current node!";
    m_currentSerializationNode = m_currentSerializationNode->append_child(kNodeCollection);
    SetAttributeValue(*m_currentSerializationNode, kAttributeName, name.c_str());
}

inline void SerializerXML::WriteCollectionEnd(const std::string& name, const size_t& /*collectionLevel*/, bool /*nextFieldExist*/)
{
    EXT_REQUIRE(m_currentSerializationNode) << "No current node!";

    EXT_ASSERT(_wcsicmp(m_currentSerializationNode->name(), kNodeCollection) == 0);
    std::wstring currentName;
    EXT_ASSERT(GetAttributeValue(*m_currentSerializationNode, kAttributeName, currentName));
    EXT_ASSERT(currentName == std::widen(name));
    EXT_UNUSED(name);

    m_currentSerializationNode = m_currentSerializationNode->parent();
}

inline void SerializerXML::WriteField(const std::string& name, const SerializableValue& value, const size_t& /*fieldLevel*/, bool /*nextFieldExist*/)
{
    EXT_REQUIRE(m_currentSerializationNode) << "No current node!";
    auto fieldNode = m_currentSerializationNode->append_child(kNodeField);
    SetAttributeValue(fieldNode, kAttributeName, name.c_str());;
    SetAttributeValue(fieldNode, kAttributeValue, value.c_str());;
    SetAttributeValue(fieldNode, kAttributeType, std::to_wstring((int)value.Type).c_str());;
}

inline bool SerializerXML::Deserialize(std::shared_ptr<SerializableNode>& deserializationTreeRoot) EXT_THROWS()
{
    pugi::xml_document hFile;
    const pugi::xml_parse_result parseRes = hFile.load_file(m_filePath.c_str(), pugi::parse_default, pugi::encoding_utf8);
    EXT_CHECK(parseRes && hFile) << parseRes.description();

    auto currentNode = hFile.document_element();
    EXT_EXPECT(currentNode) << "Can`t find data inside document";

    deserializationTreeRoot = std::make_shared<SerializableNode>(std::narrow(currentNode.name()));
    std::shared_ptr<SerializableNode> currentTreeNode = deserializationTreeRoot;

    auto goToNextNode = [&]()
    {
        while (currentNode)
        {
            currentTreeNode = currentTreeNode->Parent.lock();
            if (!currentTreeNode)
            {
                currentNode = pugi::xml_node();
                return;
            }

            if (auto nextNode = currentNode.next_sibling())
            {
                currentTreeNode = currentTreeNode->ChildNodes.emplace_back(std::make_shared<SerializableNode>("Temp", currentTreeNode));
                currentNode = std::move(nextNode);
                break;
            }
            currentNode = currentNode.parent();
        }
    };

    while (currentNode)
    {
        EXT_EXPECT(GetAttributeValue(currentNode, kAttributeName, currentTreeNode->Name)) << "Can`t find name of node";

        std::wstring value;
        if (GetAttributeValue(currentNode, kAttributeValue, value))
        {
            EXT_ASSERT(_wcsicmp(currentNode.name(), kNodeField) == 0);
            currentTreeNode->Value = std::move(value);
            if (GetAttributeValue(currentNode, kAttributeType, value))
                currentTreeNode->Value->Type = static_cast<SerializableValue::ValueType>(wcstol(value.c_str(), 0, 10));
            else
                EXT_ASSERT(false);

            goToNextNode();
        }
        else
        {
            EXT_ASSERT(_wcsicmp(currentNode.name(), kNodeCollection) == 0);
            if (const auto firstChild = currentNode.first_child())
            {
                currentNode = firstChild;
                currentTreeNode = currentTreeNode->ChildNodes.emplace_back(std::make_shared<SerializableNode>("Temp", currentTreeNode));
            }
            else
                goToNextNode();
        }
    }

    return true;
}

inline void SerializerXML::SetValue(pugi::xml_node xmlNode, const wchar_t* value)
{
    xmlNode.append_child(pugi::node_pcdata).set_value(value);
}

inline const wchar_t* SerializerXML::GetValue(const pugi::xml_node& xmlNode)
{
    return xmlNode.child_value();
}

inline void SerializerXML::SetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, const wchar_t* attributeValue)
{
    xmlNode.append_attribute(attributeName).set_value(attributeValue);
}

inline void SerializerXML::SetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, const char* attributeValue)
{
    xmlNode.append_attribute(attributeName).set_value(std::widen(attributeValue).c_str());
}

inline bool SerializerXML::GetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, std::wstring& attributeValue)
{
    const pugi::xml_attribute xmlAttribute = xmlNode.attribute(attributeName);
    if (xmlAttribute == nullptr)
        return false;
    attributeValue = xmlAttribute.as_string();
    return true;
}

inline bool SerializerXML::GetAttributeValue(pugi::xml_node xmlNode, const wchar_t* attributeName, std::string& attributeValue)
{
    std::wstring value;
    const auto res = GetAttributeValue(xmlNode, attributeName, value);
    if (!res)
        return false;

    attributeValue = std::narrow(value);
    return true;
}

} // namespace ext::serialization::serializer