#pragma once

// SerializableValue implementation and definition for deserialize_value/serialize_value functions

#include <string>
#include <type_traits>
#include <optional>

#include <ext/core/defines.h>
#include <ext/utils/string.h>

namespace ext::serializable {

// Serializable value, string with real data type, allow to adopt data on serialization
struct SerializableValue : std::wstring
{
    enum class ValueType
    {
        eNull,
        eString,
        eBool,
        eData,
        eShort,
        eInt,
        eLong,
        eFloat,
        eUnsigned
    } Type;

    template <typename... Args>
    SerializableValue(Args&&... args) : std::wstring(std::forward<Args>(args)...), Type(ValueType::eString) {}

    static SerializableValue Create(std::wstring str, const ValueType& type)
    {
        SerializableValue val(std::move(str));
        val.Type = type;
        return val;
    }
    static SerializableValue CreateNull()
    {
        SerializableValue val(L"null");
        val.Type = ValueType::eNull;
        return val;
    }
};

// Deserialization function, allow convert SerializableValue to types, write your own function for special data type
// or overload std::wistringstream operator>> operator
template<class T>
EXT_NODISCARD T deserialize_value(const SerializableValue& value);
template<>
EXT_NODISCARD inline std::wstring deserialize_value<std::wstring>(const SerializableValue& value) { return static_cast<std::wstring>(value); }
template<>
EXT_NODISCARD inline std::string deserialize_value<std::string>(const SerializableValue& value) { return std::narrow(deserialize_value<std::wstring>(value).c_str()); }
template<>
EXT_NODISCARD inline bool deserialize_value<bool>(const SerializableValue& value) { return value == L"true"; }
#ifdef __ATLTIME_H__
template<>
EXT_NODISCARD inline CTime deserialize_value<CTime>(const SerializableValue& value)
{
    SYSTEMTIME st = { 0 };
    swscanf_s(value.c_str(), L"%hd.%hd.%hd %hd:%hd:%hd", &st.wDay, &st.wMonth, &st.wYear, &st.wHour, &st.wMinute, &st.wSecond);
    return st;
}
#endif
#ifdef __AFXSTR_H__
template<>
EXT_NODISCARD inline CString deserialize_value<CString>(const SerializableValue& value) { return deserialize_value<std::wstring>(value).c_str(); }
#endif

template<class T>
EXT_NODISCARD T deserialize_value(const SerializableValue& value)
{
    T result;
    std::wistringstream stream(value);
    if constexpr (std::is_enum_v<T>)
    {
        long tempValue;
        stream >> tempValue;
        result = static_cast<T>(tempValue);
    }
    else
        stream >> result;

    return result;
}

// Serialization function, allow convert types to SerializableValue, write your own function for special data type
// or overload std::wistringstream operator<< operator
template<class T>
EXT_NODISCARD SerializableValue serialize_value(const T& val)
{
    if constexpr (std::is_enum_v<T>)
        return serialize_value<long>(static_cast<long>(val));
#ifdef __ATLTIME_H__
    else if constexpr (std::is_same_v<CTime, T>)
        return SerializableValue::Create(val.Format(L"%d.%m.%Y %H:%M:%S").GetString(), SerializableValue::ValueType::eData);
#endif // __ATLTIME_H__
#ifdef __AFXSTR_H__
    else if constexpr (std::is_same_v<CString, T>)
        return serialize_value(std::wstring(val.GetString()));
#endif // __AFXSTR_H__
    else if constexpr (std::is_same_v<bool, T>)
        return SerializableValue::Create(val ? L"true" : L"false", SerializableValue::ValueType::eBool);
    else
    {
        std::wostringstream stream;
        stream << val;
        if constexpr (std::is_floating_point_v<T>)
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eFloat);
        else if constexpr (std::is_unsigned_v<T>)
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eUnsigned);
        else if constexpr (std::is_same_v<T, short>)
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eShort);
        else if constexpr (std::is_same_v<T, long> || std::is_same_v<T, long long>)
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eLong);
        else if constexpr (std::is_integral_v<T>)
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eInt);
        else
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eString);
    }
}

} // namespace ext::serializable