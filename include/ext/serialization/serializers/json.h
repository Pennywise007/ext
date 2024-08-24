#pragma once

#include <string_view>

#include <ext/core/defines.h>
#include <ext/core/check.h>

#include <ext/serialization/serializer.h>

#include <ext/scope/defer.h>

namespace ext::serializer {

// Serializer objects to json
class SerializerJson : public ISerializer, public INodeSerializer
{
private:
    inline static constexpr wchar_t kArrayStart = L'[';
    inline static constexpr wchar_t kArrayEnd = L']';
    inline static constexpr wchar_t kFieldValueDelimiter = L':';
    inline static constexpr wchar_t kObjectStart = L'{';
    inline static constexpr wchar_t kObjectEnd = L'}';
    inline static constexpr wchar_t kCollectionObjectsDelimiter = L',';

    friend class DeserializerJson;
public:
    explicit SerializerJson(
        std::wstring& outputText,
        std::wstring indentText = std::wstring(4, L' '),
        bool addNewLines = true)
        : m_outputText(outputText)
        , m_indentText(indentText)
        , m_addNewLines(addNewLines)
    {
        outputText.clear();
    }

protected:
// ISerializer
    void Serialize(const SerializableNode& serializationTreeRoot) EXT_THROWS() override;
// INodeSerializer
    void WriteObjectStart(size_t collectionLevel, bool emptyObject) override;
    void WriteObjectEnd(size_t collectionLevel, bool emptyObject, bool nextFieldExist) override;
    void WriteFieldStart(const SerializableValue& name, size_t indentLevel) override;
    void WriteFieldEnd(size_t indentLevel, bool nextObjectExist) override;
    void WriteArrayStart(size_t indentLevel, bool emptyArray) override;
    void WriteArrayEnd(size_t indentLevel, bool emptyArray, bool nextObjectExist) override;
    void WriteValue(const SerializableValue& value, size_t indentLevel, bool nextObjectExist) override;

private:
    void AddIndent(size_t indentLevel);
    void AddNewLine();
    void AddValue(const SerializableValue& value);
private:
    std::wstring& m_outputText;
    const std::wstring m_indentText;
    const bool m_addNewLines;
};

[[nodiscard]] inline void SerializerJson::Serialize(const SerializableNode& serializationTreeRoot) EXT_THROWS()
{
    TreeSerializer::SerializeTree(serializationTreeRoot, this);
}

inline void SerializerJson::AddIndent(size_t indentLevel)
{
    if (m_indentText.empty())
        return;

    for (size_t i = 0; i < indentLevel; ++i)
    {
        m_outputText += m_indentText;
    }
}

inline void SerializerJson::AddNewLine()
{
    if (m_addNewLines)
        m_outputText += L"\n";
}

inline void SerializerJson::WriteObjectStart(size_t, bool emptyObject)
{
    m_outputText += kObjectStart;
    if (!emptyObject)
        AddNewLine();
}

inline void SerializerJson::WriteObjectEnd(size_t indentLevel, bool emptyObject, bool nextFieldExist)
{
    if (!emptyObject)
        AddIndent(indentLevel);
    m_outputText += kObjectEnd;
    if (nextFieldExist)
    {
        m_outputText += kCollectionObjectsDelimiter;
        m_outputText += L' ';
    }
}

inline void SerializerJson::WriteFieldStart(const SerializableValue& name, size_t indentLevel)
{
    AddIndent(indentLevel);
    AddValue(name);
    m_outputText += kFieldValueDelimiter;
    m_outputText += L' ';
}

inline void SerializerJson::WriteFieldEnd(size_t, bool nextFieldExist)
{
    if (nextFieldExist)
        m_outputText += kCollectionObjectsDelimiter;
    AddNewLine();
}

inline void SerializerJson::WriteArrayStart(size_t, bool)
{
    m_outputText += kArrayStart;
}

inline void SerializerJson::WriteArrayEnd(size_t, bool, bool nextFieldExist)
{
    m_outputText += kArrayEnd;
    if (nextFieldExist)
        m_outputText += kCollectionObjectsDelimiter;
}

inline void SerializerJson::WriteValue(const SerializableValue& value, size_t, bool nextObjectExist)
{
    AddValue(value);
    if (nextObjectExist)
    {
        m_outputText += kCollectionObjectsDelimiter;
        m_outputText += L' ';
    }
}

inline void SerializerJson::AddValue(const SerializableValue &value)
{
    const auto escapeValue = [&]() {
        std::wostringstream escaped;
        for (wchar_t c : value) {
            switch (c) {
            case L'"':
                escaped << L"\\\"";  // Escape double quote
                break;
            case L'\\':
                escaped << L"\\\\";  // Escape backslash
                break;
            case L'\b':
                escaped << L"\\b";   // Escape backspace
                break;
            case L'\f':
                escaped << L"\\f";   // Escape formfeed
                break;
            case L'\n':
                escaped << L"\\n";   // Escape newline
                break;
            case L'\r':
                escaped << L"\\r";   // Escape carriage return
                break;
            case L'\t':
                escaped << L"\\t";   // Escape tab
                break;
            default:
                if (L'\x00' <= c && c <= L'\x1f')
                    // Escape other control characters as \uXXXX
                    escaped << L"\\u" << std::hex << std::setw(4) << std::setfill(L'0') << static_cast<int>(c);
                else
                    escaped << c;  // All other characters are added directly
            }
        }
        return escaped.str();
    };

    switch (value.Type)
    {
    case SerializableValue::ValueType::eDate:
    case SerializableValue::ValueType::eString:
        m_outputText += L'\"' + escapeValue() + L'\"';
    break;
    case SerializableValue::ValueType::eNumber:
    {
        // replace , with . symbol
        std::wstring text = value;\
        std::replace(text.begin(), text.end(), ',', '.');
        m_outputText += text;
    }
    break;
    default:
        m_outputText += value;
        break;
    }
}

// Deserializer json to nodes
class DeserializerJson : public IDeserializer
{
public:
    explicit DeserializerJson(const std::wstring& inputText) EXT_THROWS();

protected:
// IDeserializer
    [[nodiscard]] SerializableNode& Deserialize() EXT_THROWS() override;

private:
    // parse collection
    [[nodiscard]] size_t parseCollection(std::wstring_view text) EXT_THROWS();
    // Find collection end
    [[nodiscard]] size_t findCollectionEnd(std::wstring_view text) EXT_THROWS();
    // Check next object type in the text
    [[nodiscard]] SerializableNode getNextNode(std::wstring_view& text) EXT_THROWS();
    // Extract text in quotes
    [[nodiscard]] std::wstring getNextValueInQuotes(std::wstring_view& text) EXT_THROWS();
    // Extract next serializable value from numeric
    [[nodiscard]] SerializableValue getNextNumericOrNull(std::wstring_view& text) EXT_THROWS();
    // Trim text
    static bool trimLeft(std::wstring_view& text);
    static bool trimRight(std::wstring_view& text);

private:
    SerializableNode m_rootNode = SerializableNode::ObjectNode();
    SerializableNode* m_currentNode = nullptr;
};

inline DeserializerJson::DeserializerJson(const std::wstring& inputText) EXT_THROWS()
{
    std::wstring_view text(inputText);
    EXT_EXPECT(trimLeft(text)) << "Input text is empty";

    m_rootNode = getNextNode(text);
    EXT_EXPECT(m_rootNode.Type == SerializableNode::NodeType::eObject || m_rootNode.Type == SerializableNode::NodeType::eArray)
        << "Unexpected root object in input text '" << inputText
        << "', text should start with '" << std::wstring(1, SerializerJson::kObjectStart)
        << "' or '" << std::wstring(1, SerializerJson::kArrayStart) << "'";

    m_currentNode = &m_rootNode;
    auto rootCollectionEnd = parseCollection(text);

    text.remove_prefix(rootCollectionEnd + 1);
    EXT_ASSERT(!trimRight(text)) << "After finishing parsing text should be empty";
}

[[nodiscard]] inline SerializableNode& DeserializerJson::Deserialize() EXT_THROWS()
{
    return m_rootNode;
}

[[nodiscard]] inline size_t DeserializerJson::parseCollection(std::wstring_view text) EXT_THROWS()
{
    EXT_ASSERT(m_currentNode->Type == SerializableNode::NodeType::eArray
        || m_currentNode->Type == SerializableNode::NodeType::eObject) << "Current node expected to be array or object, type: "
        << ext::reflection::enum_to_string(m_currentNode->Type);

    EXT_EXPECT(text.size() >= 2) << "Text doesn't contain collection end";

    size_t collectionEnd = findCollectionEnd(text);
    EXT_ASSERT(text[collectionEnd] == SerializerJson::kArrayEnd || text[collectionEnd] == SerializerJson::kObjectEnd) << "Invalid collection end";
    EXT_ASSERT(collectionEnd != 0) << "Invalid collection end index";

    // remove collection start and end symbols
    text = text.substr(1, collectionEnd - 1);
    trimLeft(text);
    trimRight(text);

    while (!text.empty())
    {
        auto nextNode = getNextNode(text);
        switch (m_currentNode->Type)
        {
        case SerializableNode::NodeType::eObject:
            EXT_EXPECT(nextNode.Type == SerializableNode::NodeType::eField) << "Unexpected child type for object: "
                << ext::reflection::enum_to_string(m_currentNode->Type) << ", expect field, text: " << text;
            break;
        case SerializableNode::NodeType::eArray:
            EXT_EXPECT(nextNode.Type != SerializableNode::NodeType::eField) << "Unexpected child type for array, array can't contain fields, text: " << text;
            break;
        default:
            EXT_EXPECT(false) << "Unexpected collection type " << ext::reflection::enum_to_string(m_currentNode->Type);
        }

        bool fieldNode = nextNode.Type == SerializableNode::NodeType::eField;
        if (fieldNode)
        {
            // If we receive field - add field value as child
            m_currentNode = m_currentNode->AddChild(std::move(nextNode));

            nextNode = getNextNode(text);
            EXT_EXPECT(nextNode.Type != SerializableNode::NodeType::eField) << "Field can't contain another field, text: " << text;
        }

        m_currentNode = m_currentNode->AddChild(std::move(nextNode));
        switch (m_currentNode->Type)
        {
        case SerializableNode::NodeType::eValue:
            break;
        case SerializableNode::NodeType::eField:
            EXT_EXPECT(false) << "Internal error, we should already handle this case";
        case SerializableNode::NodeType::eArray:
        case SerializableNode::NodeType::eObject:
            text.remove_prefix(parseCollection(text) + 1);
            break;
        default:
            EXT_EXPECT(false) << "Unexpected object type " << ext::reflection::enum_to_string(m_currentNode->Type);
        }

        m_currentNode = m_currentNode->Parent;

        // restore current node as field parent
        if (fieldNode)
            m_currentNode = m_currentNode->Parent;

        // remove child objects delimiter
        if (trimLeft(text))
        {
            EXT_EXPECT(text.front() == SerializerJson::kCollectionObjectsDelimiter) << "Unexpected objects delimiter symbol, text: " << text;
            text.remove_prefix(1);
            trimLeft(text);
        }
    }

    return collectionEnd;
}

[[nodiscard]] inline size_t DeserializerJson::findCollectionEnd(std::wstring_view text) EXT_THROWS()
{
    // validate collection start
    bool currentObjectIsArray = m_currentNode->Type == SerializableNode::NodeType::eArray;

    wchar_t startCollectionSymbol = currentObjectIsArray ? SerializerJson::kArrayStart : SerializerJson::kObjectStart;
    wchar_t endCollectionSymbol = currentObjectIsArray ? SerializerJson::kArrayEnd : SerializerJson::kObjectEnd;

    EXT_EXPECT(text.front() == startCollectionSymbol) << "Unexpected collection start symbol, text: " << text;

    int depth = 0;
    bool inQuotes = false;
    bool escape = false;

    for (size_t i = 0, size = text.size(); i < size; ++i) {
        wchar_t ch = text[i];

        if (escape)
            escape = false;
        else if (ch == L'\\')
            escape = true;
        else if (ch == L'"')
            inQuotes = !inQuotes;
        else if (!inQuotes)
        {
            if (ch == startCollectionSymbol)
                depth++;
            else if (ch == endCollectionSymbol)
            {
                depth--;
                if (depth == 0)
                    return i;
            }
        }
    }

    EXT_EXPECT(false) << "Can't find " << (currentObjectIsArray ? "array" : "object") << " end in text: " << text;
    EXT_UNREACHABLE();
}

[[nodiscard]] inline SerializableNode DeserializerJson::getNextNode(std::wstring_view& text) EXT_THROWS()
{
    EXT_EXPECT(!text.empty()) << "Unexpected string end";

    auto firstStringSymbol = text.front();
    switch (firstStringSymbol)
    {
    case SerializerJson::kObjectStart:
        return SerializableNode::ObjectNode();
    case SerializerJson::kArrayStart:
        return SerializableNode::ArrayNode();
    case L'"':
    {
        // beginning of the field like '"field_name":' or value like '"value"'
        auto valueInQuotes = getNextValueInQuotes(text);

        if (!text.empty() && text.front() == SerializerJson::kFieldValueDelimiter)
        {
            text.remove_prefix(1);
            EXT_EXPECT(trimLeft(text)) << "Can't find field value, field is: " << valueInQuotes;
            return SerializableNode::FieldNode(std::move(valueInQuotes));
        }
        else
            return SerializableNode::ValueNode(std::move(valueInQuotes));
    }
    default:
    {
        auto numericOrNullValue = getNextNumericOrNull(text);
        if (!text.empty() && text.front() == SerializerJson::kFieldValueDelimiter)
        {
            // it can be map with int values as a key
            text.remove_prefix(1);
            EXT_EXPECT(trimLeft(text)) << "Can't find field value, field is: " << numericOrNullValue;
            return SerializableNode::FieldNode(std::move(numericOrNullValue));
        }
        else
            return SerializableNode::ValueNode(numericOrNullValue);
    }
    }
}

[[nodiscard]] inline std::wstring DeserializerJson::getNextValueInQuotes(std::wstring_view& text) EXT_THROWS()
{
    EXT_EXPECT(text.front() == L'"') << "Expect start of field name in quotes: " << text;
    
    constexpr size_t kNotFoundIndex = static_cast<size_t>(-1);
    // find unescaped quote
    size_t fieldNameSize = kNotFoundIndex;
    bool escape = false;
    for (size_t i = 1, size = text.size(); i < size; ++i) {
        wchar_t ch = text[i];

        if (escape)
            escape = false;
        else if (ch == L'\\')
            escape = true;
        else if (ch == L'"')
        {
            fieldNameSize = i;
            break;
        }
    }

    EXT_EXPECT(fieldNameSize != kNotFoundIndex) << "Can't find value in quotes, maybe it is not closed? Text: " << text;

    const auto unescapeText = [&]() {
        size_t size = fieldNameSize - 1;

        std::wstring unescaped;
        unescaped.reserve(size);

        bool escape = false;

        for (size_t i = 1; i <= size; ++i) {
            wchar_t c = text[i];

            if (escape)
            {
                switch (c) {
                case L'"': unescaped += L'"'; break;
                case L'\\': unescaped += L'\\'; break;
                case L'b': unescaped += L'\b'; break;
                case L'f': unescaped += L'\f'; break;
                case L'n': unescaped += L'\n'; break;
                case L'r': unescaped += L'\r'; break;
                case L't': unescaped += L'\t'; break;
                case L'u': {
                    if (i + 4 <= size)
                    {
                        std::wstring hex(text.data() + i + 1, 4);
                        unescaped += static_cast<char>(std::stoi(hex, nullptr, 16));
                        i += 4;
                    }
                    break;
                }
                default: unescaped += c; break;
                }
                escape = false;
            }
            else if (c == '\\')
                escape = true;
            else
                unescaped += c;
        }
        return unescaped;
    };
    std::wstring result = unescapeText();

    // skip quotes and value
    text.remove_prefix(fieldNameSize + 1);
    trimLeft(text);

    return result;
}

[[nodiscard]] inline SerializableValue DeserializerJson::getNextNumericOrNull(std::wstring_view& text) EXT_THROWS()
{
    EXT_ASSERT(!text.empty());
    EXT_ASSERT(m_currentNode->Parent != nullptr &&
        m_currentNode->Parent->Type != SerializableNode::NodeType::eValue);

    EXT_DEFER({
        if (trimLeft(text))
        {
            auto nextSymbolAfterValue = text.front();
            EXT_EXPECT(nextSymbolAfterValue == SerializerJson::kCollectionObjectsDelimiter ||
                nextSymbolAfterValue == SerializerJson::kFieldValueDelimiter || // map where key is number
                nextSymbolAfterValue == SerializerJson::kArrayEnd ||
                nextSymbolAfterValue == SerializerJson::kObjectEnd) << "Unexpected symbol after value end: " << text;
        }
    });

    const auto valuesArray = std::array{
        std::pair{ std::wstring(L"null"), SerializableValue::ValueType::eNull },
        std::pair{ std::wstring(L"false"), SerializableValue::ValueType::eBool },
        std::pair{ std::wstring(L"true"), SerializableValue::ValueType::eBool },
    };

    // Check predefined values
    for (auto&& [fixedText, type] : valuesArray)
    {
        if (text.substr(0, fixedText.size()) == fixedText)
        {
            text.remove_prefix(fixedText.size());
            return SerializableValue::Create(fixedText, type);
        }
    }

    // If it is not a fixed string from valuesArray it can be only number
    auto firstStringSymbol = text.front();
    EXT_EXPECT(firstStringSymbol == L'-' || std::iswdigit(firstStringSymbol))
        << "Unexpected symbol in `" << text << "', expect number/nan/null/array/object start";

    size_t pointIndex = static_cast<size_t>(-1);
    bool eExist = false;
    size_t numberLength = 0;
    if (firstStringSymbol == L'-')
        ++numberLength;

    for (size_t size = text.size(); numberLength < size; ++numberLength)
    {
        wchar_t symbol = text[numberLength];
        if (std::iswdigit(symbol))
            continue;
        if (symbol == L'.')
        {
            EXT_EXPECT(pointIndex == -1) << "Unexpected second point symbol in the number " << text;
            pointIndex = numberLength;
        }
        else if (symbol == L'e')
        {
            EXT_EXPECT(eExist) << "Unexpected second `e` symbol in the number " << text;
            eExist = true;
        }
        else
            break;
    }

    std::wstring numerText(text.data(), numberLength);
    if (pointIndex != -1)
    {
        // Replacing decimal point with the current decimal symbol
        std::wostringstream woss;
        const std::numpunct<wchar_t>& np = std::use_facet<std::numpunct<wchar_t>>(woss.getloc());
        numerText[pointIndex] = np.decimal_point();
    }

    text.remove_prefix(numberLength);
    return SerializableValue::Create(std::move(numerText), SerializableValue::ValueType::eNumber);
}

inline bool DeserializerJson::trimLeft(std::wstring_view& text)
{
    const auto it = std::find_if_not(text.begin(), text.end(), [](const wchar_t ch) { return std::iswspace(ch); });
    text = text.substr(std::distance(text.begin(), it));
    return !text.empty();
}

inline bool DeserializerJson::trimRight(std::wstring_view& text)
{
    const auto it = std::find_if_not(text.rbegin(), text.rend(), [](const wchar_t ch) { return std::iswspace(ch); });
    text = text.substr(0, text.size() - std::distance(text.rbegin(), it));
    return !text.empty();
}

} // namespace ext::serializer