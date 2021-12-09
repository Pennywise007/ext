#pragma once

#include <string>
#include <string_view>
#include <stack>

#include <ext/core/check.h>
#include <ext/serialization/serializer.h>

namespace ext::serializable::serializer {

/*
* Serializer objects to Text, format:

"Main node": {
    "schedule": {
        "days": 1,
        "every" : 0
    },
    "settings" : {
        "compoundFilesSettings": {
            "installationPackagesScanMode": 2,
            "officeArchivesScanMode" : 2,
            "sizeLimits" : {
                "enabled": false,
                "text" : "test"
            }
        },
        "useIChecker": true
    }
}
*/
class SerializerText : public ISerializer, public INodeSerializer, public IDeserializer
{
public:
    explicit SerializerText(std::wstring* outputText) : m_outputText(outputText) {}
    explicit SerializerText(const std::wstring& inputText) SSH_THROWS();

protected:
// ISerializer
    SSH_NODISCARD bool Serialize(const std::shared_ptr<SerializableNode>& serializationTreeRoot) SSH_THROWS() override;
// INodeSerializer
    void WriteCollectionStart(const std::wstring& name, const size_t& collectionLevel) override;
    void WriteCollectionEnd(const std::wstring& name, const size_t& collectionLevel, bool nextFieldExist) override;
    void WriteField(const std::wstring& name, const SerializableValue& value, const size_t& fieldLevel, bool nextFieldExist) override;

// IDeserializer
    SSH_NODISCARD bool Deserialize(_Out_ std::shared_ptr<SerializableNode>& deserializationTreeRoot) SSH_THROWS() override;

private:
    static std::wstring GenerateIndent(const size_t& indentLevel);
    static bool TrimLeft(std::wstring_view& string);
    static bool TrimRight(std::wstring_view& string);
    // Find open and close braces position by current level
    SSH_NODISCARD static bool FindBracesPositions(const std::wstring_view& string, std::pair<size_t, size_t>& bracesPositions);
    static std::wstring GetFieldName(std::wstring_view& text);
    static void GetFieldValue(std::wstring_view& text, std::optional<SerializableValue>& value);
    // Fill node info for current braces level
    void GetFieldInfoOnCurrentBracesLevel(std::wstring_view& textInsideBraces, const std::shared_ptr<SerializableNode>& currentNode) const SSH_THROWS();

private:
    std::shared_ptr<SerializableNode> m_rootNode;

    std::wstring* m_outputText = nullptr;
};

inline SerializerText::SerializerText(const std::wstring& inputText) SSH_THROWS()
{
    std::wstring_view text(inputText);
    SSH_EXPECT(TrimLeft(text)) << "Input text empty " << inputText;
    m_rootNode = std::make_shared<SerializableNode>(GetFieldName(text));
    if (text.front() == L'{')
        GetFieldInfoOnCurrentBracesLevel(text, m_rootNode);
    else
        GetFieldValue(text, m_rootNode->Value);

    SSH_ASSERT(text.empty());
}

SSH_NODISCARD inline bool SerializerText::Serialize(const std::shared_ptr<SerializableNode>& serializationTreeRoot) SSH_THROWS()
{
    SSH_REQUIRE(serializationTreeRoot && m_outputText) << "Root node uninitialized!";
    m_outputText->clear();
    TreeSerializer::SerializeTree(serializationTreeRoot, this);
    return true;
}

SSH_NODISCARD inline bool SerializerText::Deserialize(_Out_ std::shared_ptr<SerializableNode>& deserializationTreeRoot) SSH_THROWS()
{
    SSH_REQUIRE(m_rootNode) << "Root node uninitialized!";
    deserializationTreeRoot = m_rootNode;
    return true;
}

inline std::wstring SerializerText::GenerateIndent(const size_t& indentLevel)
{
    return std::wstring(indentLevel * 4, L' ');
}

inline void SerializerText::WriteCollectionStart(const std::wstring& name, const size_t& collectionLevel)
{
    *m_outputText += GenerateIndent(collectionLevel) + std::string_swprintf(LR"("%s": {)", name.c_str()) + L'\n';
}

inline void SerializerText::WriteCollectionEnd(const std::wstring& /*name*/, const size_t& collectionLevel, bool nextFieldExist)
{
    *m_outputText += GenerateIndent(collectionLevel) + L'}' + (nextFieldExist ? L",\n" : L"\n");
}

inline void SerializerText::WriteField(const std::wstring& name, const SerializableValue& value, const size_t& fieldLevel, bool nextFieldExist)
{
    *m_outputText += GenerateIndent(fieldLevel) + std::string_swprintf(LR"("%s": )", name.c_str());

    if (value.Type == SerializableValue::ValueType::eString)
        *m_outputText += L'\"' + value + L'\"';
    else
        *m_outputText += value;

    if (nextFieldExist)
        *m_outputText += L',';

    *m_outputText += L'\n';
}

inline bool SerializerText::TrimLeft(std::wstring_view& string)
{
    const auto it = std::find_if_not(string.begin(), string.end(), [](const wchar_t ch)
                                     {
                                         return std::iswspace(ch);
                                     });
    string = string.substr(std::distance(string.begin(), it));
    return !string.empty();
}

inline bool SerializerText::TrimRight(std::wstring_view& string)
{
    const auto it = std::find_if_not(string.rbegin(), string.rend(), [](const wchar_t ch)
                                     {
                                         return std::iswspace(ch);
                                     });
    string = string.substr(0, string.size() - std::distance(string.rbegin(), it));
    return !string.empty();
}

SSH_NODISCARD inline bool SerializerText::FindBracesPositions(const std::wstring_view& string, std::pair<size_t, size_t>& bracesPositions)
{
    bool startBraceFound = false;

    size_t currentLevel = 0;
    for (size_t index = 0, length = string.size(); index < length; ++index)
    {
        if (const auto& symbol = string.at(index); symbol == '{')
        {
            if (!startBraceFound)
            {
                startBraceFound = true;
                bracesPositions.first = index;
            }
            ++currentLevel;
        }
        else if (symbol == '}')
        {
            if (!startBraceFound)
                return false;
            --currentLevel;
            if (currentLevel == 0)
            {
                bracesPositions.second = index;
                return true;
            }
        }
    }

    SSH_EXPECT(!startBraceFound) << "Can`t find closing braces in text: " << string.substr(bracesPositions.first).data();
    return false;
}

inline void SerializerText::GetFieldInfoOnCurrentBracesLevel(std::wstring_view& textInsideBraces, const std::shared_ptr<SerializableNode>& currentNode) const SSH_THROWS()
{
    SSH_CHECK(TrimLeft(textInsideBraces) && TrimRight(textInsideBraces)) << "Text empty";
    SSH_CHECK(textInsideBraces.front() == L'{' && textInsideBraces.back() == L'}') << "Text must be in braces";

    // remove braces
    textInsideBraces = textInsideBraces.substr(1, textInsideBraces.size() - 2);
    TrimLeft(textInsideBraces);
    TrimRight(textInsideBraces);

    while (!textInsideBraces.empty())
    {
        std::shared_ptr<SerializableNode> node = currentNode->ChildNodes.emplace_back(std::make_shared<SerializableNode>(GetFieldName(textInsideBraces), currentNode));

        // Find field value
        if (textInsideBraces.front() == L'{')
        {
            // start of collection like : "schedule" : { "days": 1, "every" : 0 }
            std::pair<size_t, size_t> bracesPositions;
            SSH_CHECK(FindBracesPositions(textInsideBraces, bracesPositions)) << "Failed to find braces end" << textInsideBraces;
            SSH_ASSERT(bracesPositions.first == 0) << "We already have found start brace position";

            std::wstring_view text = textInsideBraces.substr(0, bracesPositions.second + 1);
            GetFieldInfoOnCurrentBracesLevel(text, node);
            SSH_ASSERT(text.empty());

            textInsideBraces = textInsideBraces.substr(bracesPositions.second + 1);
            if (!TrimLeft(textInsideBraces))
                break;
            if (textInsideBraces.front() == ',') // expect next field
            {
                textInsideBraces = textInsideBraces.substr(1);
                SSH_EXPECT(TrimLeft(textInsideBraces)) << "After \',\' text is empty";
            }
        }
        else
            // field value like: "extScheduleFlags": 0,
            GetFieldValue(textInsideBraces, node->Value);
    }
}

inline std::wstring SerializerText::GetFieldName(std::wstring_view& text)
{
    SSH_EXPECT(text.front() == L'"') << "Expect start of field name in quotes: " << text;
    size_t fieldNameSize = 0;
    for (size_t quotesIndex = 1, length = text.size(); quotesIndex < length; ++quotesIndex)
    {
        if (text.at(quotesIndex) == L'"')
        {
            fieldNameSize = quotesIndex - 1;
            break;
        }
    }
    SSH_EXPECT(fieldNameSize > 0) << "Can`t find field name ending quote: " << text;

    std::wstring result = { text.data() + 1, fieldNameSize };

    text = text.substr(fieldNameSize + 2);
    SSH_CHECK(TrimLeft(text)) << "Failed to find field value";

    SSH_EXPECT(text.front() == L':') << "Expect field value after \':\' symbol" << text;
    text = text.substr(1);
    SSH_CHECK(TrimLeft(text)) << "Value after field name not found";

    return result;
}

inline void SerializerText::GetFieldValue(std::wstring_view& text, std::optional<SerializableValue>& value)
{
    SSH_EXPECT(!value.has_value()) << "Value already found";

    // field value like: "extScheduleFlags": 0,
    for (size_t valueIndex = 0, length = text.size(); valueIndex < length; ++valueIndex)
    {
        if (text.at(valueIndex) == L',')
        {
            auto fieldValue = text.substr(0, valueIndex);
            SSH_EXPECT(TrimRight(fieldValue)) << "Field value empty " << text;
            value.emplace(fieldValue);
            text = text.substr(valueIndex + 1);
            TrimLeft(text);

            break;
        }
    }
    if (!value.has_value())
    {
        SSH_EXPECT(TrimRight(text)) << "Field value empty";
        value.emplace(text);
        std::wstring_view().swap(text);
    }

    // remove quotes for string values
    if (value->front() == L'\"' && value->back() == L'\"')
        value.emplace(value->substr(1, value->size() - 2));
}

} // namespace ext::serializable::serializer