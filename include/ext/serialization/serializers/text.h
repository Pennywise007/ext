#pragma once

#include <string>
#include <string_view>
#include <stack>

#include <ext/core/defines.h>
#include <ext/core/check.h>

#include <ext/serialization/serializer.h>

#include <ext/std/string.h>

namespace ext::serializer {

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
    explicit SerializerText(const std::wstring& inputText) EXT_THROWS();

protected:
// ISerializer
    [[nodiscard]] bool Serialize(const std::shared_ptr<SerializableNode>& serializationTreeRoot) EXT_THROWS() override;
// INodeSerializer
    void WriteCollectionStart(const std::string& name, const size_t& collectionLevel) override;
    void WriteCollectionEnd(const std::string& name, const size_t& collectionLevel, bool nextFieldExist) override;
    void WriteField(const std::string& name, const SerializableValue& value, const size_t& fieldLevel, bool nextFieldExist) override;

// IDeserializer
    [[nodiscard]] bool Deserialize(std::shared_ptr<SerializableNode>& deserializationTreeRoot) EXT_THROWS() override;

private:
    static std::wstring GenerateIndent(const size_t& indentLevel);
    static bool TrimLeft(std::wstring_view& string);
    static bool TrimRight(std::wstring_view& string);
    // Find open and close braces position by current level
    [[nodiscard]] static bool FindBracesPositions(const std::wstring_view& string, std::pair<size_t, size_t>& bracesPositions);
    static std::string GetFieldName(std::wstring_view& text);
    static void GetFieldValue(std::wstring_view& text, std::optional<SerializableValue>& value);
    // Fill node info for current braces level
    void GetFieldInfoOnCurrentBracesLevel(std::wstring_view& textInsideBraces, const std::shared_ptr<SerializableNode>& currentNode) const EXT_THROWS();

private:
    std::shared_ptr<SerializableNode> m_rootNode;

    std::wstring* m_outputText = nullptr;
};

inline SerializerText::SerializerText(const std::wstring& inputText) EXT_THROWS()
{
    std::wstring_view text(inputText);
    EXT_EXPECT(TrimLeft(text)) << "Input text empty " << inputText;
    m_rootNode = std::make_shared<SerializableNode>(GetFieldName(text));
    if (text.front() == L'{')
        GetFieldInfoOnCurrentBracesLevel(text, m_rootNode);
    else
        GetFieldValue(text, m_rootNode->Value);

    EXT_ASSERT(text.empty());
}

[[nodiscard]] inline bool SerializerText::Serialize(const std::shared_ptr<SerializableNode>& serializationTreeRoot) EXT_THROWS()
{
    EXT_REQUIRE(serializationTreeRoot && m_outputText) << "Root node uninitialized!";
    m_outputText->clear();
    TreeSerializer::SerializeTree(serializationTreeRoot, this);
    return true;
}

[[nodiscard]] inline bool SerializerText::Deserialize(std::shared_ptr<SerializableNode>& deserializationTreeRoot) EXT_THROWS()
{
    EXT_REQUIRE(m_rootNode) << "Root node uninitialized!";
    deserializationTreeRoot = m_rootNode;
    return true;
}

inline std::wstring SerializerText::GenerateIndent(const size_t& indentLevel)
{
    return std::wstring(indentLevel * 4, L' ');
}

inline void SerializerText::WriteCollectionStart(const std::string& name, const size_t& collectionLevel)
{
    *m_outputText += GenerateIndent(collectionLevel) + L"\"" + std::widen(name) + L"\": {" + L'\n';
}

inline void SerializerText::WriteCollectionEnd(const std::string& /*name*/, const size_t& collectionLevel, bool nextFieldExist)
{
    *m_outputText += GenerateIndent(collectionLevel) + L'}' + (nextFieldExist ? L",\n" : L"\n");
}

inline void SerializerText::WriteField(const std::string& name, const SerializableValue& value, const size_t& fieldLevel, bool nextFieldExist)
{
    *m_outputText += GenerateIndent(fieldLevel) + L"\"" + std::widen(name) + L"\": ";

    switch (value.Type)
    {
    case SerializableValue::ValueType::eString:
    {
        std::wstring escapedString = value;
        for (size_t length = escapedString.length(); length != 0; --length)
        {
            if (escapedString.at(length - 1) == L'\"')
                escapedString.insert(length - 1, 1, L'\\');
        }
        *m_outputText += L'\"' + escapedString + L'\"';
    }
    break;
    case SerializableValue::ValueType::eFloat:  // do not write text like value: 4,5
        *m_outputText += L'\"' + value + L'\"';
        break;
    default:
        *m_outputText += value;
        break;
    }

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

[[nodiscard]] inline bool SerializerText::FindBracesPositions(const std::wstring_view& string, std::pair<size_t, size_t>& bracesPositions)
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

    EXT_EXPECT(!startBraceFound) << "Can`t find closing braces in text: " << string.substr(bracesPositions.first).data();
    return false;
}

inline void SerializerText::GetFieldInfoOnCurrentBracesLevel(std::wstring_view& textInsideBraces, const std::shared_ptr<SerializableNode>& currentNode) const EXT_THROWS()
{
    EXT_CHECK(TrimLeft(textInsideBraces) && TrimRight(textInsideBraces)) << "Text empty";
    EXT_CHECK(textInsideBraces.front() == L'{' && textInsideBraces.back() == L'}') << "Text must be in braces";

    // remove braces
    textInsideBraces = textInsideBraces.substr(1, textInsideBraces.size() - 2);
    TrimLeft(textInsideBraces);
    TrimRight(textInsideBraces);

    while (!textInsideBraces.empty())
    {
        std::shared_ptr<SerializableNode> node = currentNode->AddChild(GetFieldName(textInsideBraces), currentNode);

        // Find field value
        if (textInsideBraces.front() == L'{')
        {
            // start of collection like : "schedule" : { "days": 1, "every" : 0 }
            std::pair<size_t, size_t> bracesPositions;
            EXT_CHECK(FindBracesPositions(textInsideBraces, bracesPositions)) << "Failed to find braces end" << textInsideBraces;
            EXT_ASSERT(bracesPositions.first == 0) << "We already have found start brace position";

            std::wstring_view text = textInsideBraces.substr(0, bracesPositions.second + 1);
            GetFieldInfoOnCurrentBracesLevel(text, node);
            EXT_ASSERT(text.empty());

            textInsideBraces = textInsideBraces.substr(bracesPositions.second + 1);
            if (!TrimLeft(textInsideBraces))
                break;
            if (textInsideBraces.front() == ',') // expect next field
            {
                textInsideBraces = textInsideBraces.substr(1);
                EXT_EXPECT(TrimLeft(textInsideBraces)) << "After \',\' text is empty";
            }
        }
        else
            // field value like: "extScheduleFlags": 0,
            GetFieldValue(textInsideBraces, node->Value);
    }
}

inline std::string SerializerText::GetFieldName(std::wstring_view& text)
{
    EXT_EXPECT(text.front() == L'"') << "Expect start of field name in quotes: " << text;
    size_t fieldNameSize = 0;
    for (size_t quotesIndex = 1, length = text.size(); quotesIndex < length; ++quotesIndex)
    {
        if (text.at(quotesIndex) == L'"')
        {
            fieldNameSize = quotesIndex - 1;
            break;
        }
    }
    EXT_EXPECT(fieldNameSize > 0) << "Can`t find field name ending quote: " << text;

    std::wstring result = { text.data() + 1, fieldNameSize };

    text = text.substr(fieldNameSize + 2);
    EXT_CHECK(TrimLeft(text)) << "Failed to find field value";

    EXT_EXPECT(text.front() == L':') << "Expect field value after \':\' symbol" << text;
    text = text.substr(1);
    EXT_CHECK(TrimLeft(text)) << "Value after field name not found";

    return std::narrow(result);
}

inline void SerializerText::GetFieldValue(std::wstring_view& text, std::optional<SerializableValue>& value)
{
    EXT_EXPECT(!value.has_value()) << "Value already found";

    EXT_EXPECT(!text.empty()) << "Field value text empty";

    const bool startsWithQuotes = text.at(0) == L'\"';
    const wchar_t lastValueSymbol = startsWithQuotes ? L'\"' : L',';

    // field value like: "extScheduleFlags": 0,
    for (size_t valueIndex = startsWithQuotes, length = text.size(); valueIndex < length; ++valueIndex)
    {
        if (text.at(valueIndex) == lastValueSymbol && (!startsWithQuotes || text.at(valueIndex - 1) != L'\\'))
        {
            auto fieldValue = text.substr(startsWithQuotes, valueIndex - startsWithQuotes);
            TrimRight(fieldValue);
            value.emplace(fieldValue);
            text = text.substr(valueIndex + 1);
            TrimLeft(text);
            if (startsWithQuotes && !text.empty() && text.at(0) == L',')
            {
                text = text.substr(1);
                TrimLeft(text);
            }
            break;
        }
    }

    if (!value.has_value())
    {
        EXT_EXPECT(!startsWithQuotes) << "Can`t find field value";
        EXT_EXPECT(TrimRight(text)) << "Field value empty";
        value.emplace(text);
        std::wstring_view().swap(text);
    }

    if (value == L"null")
        value->Type = SerializableValue::ValueType::eNull;
    else if (startsWithQuotes)
    {
        // remove escaping symbols
        for (size_t length = value->length(); length != 0; --length)
        {
            if (value->at(length - 1) == L'\"')
            {
                EXT_EXPECT(length >= 2 && value->at(length - 2) == L'\\') << "Can`t find escape symbol in text: " << value->c_str();
                value->erase(length-- - 2, 1);
            }
        }
    }
}

} // namespace ext::serializer