#pragma once

// SerializableValue implementation and definition for deserialize_value/serialize_value functions

#include <chrono>
#include <string>
#include <math.h>
#include <type_traits>
#include <optional>

#include <ext/reflection/enum.h>
#include <ext/std/string.h>

namespace ext::serializable {

template <typename T, typename = void>
struct is_duration : std::false_type {};
template <typename T>
struct is_duration<T, std::void_t<typename T::rep, typename T::period>> : std::true_type {};
template <typename T>
inline constexpr bool is_duration_v = is_duration<T>::value;

// Serializable value, string with real data type, allow to adopt data on serialization
struct SerializableValue : std::wstring
{
    enum class ValueType
    {
        eNull,
        eString,
        eNumber,
        eBool,
        eDate
    } Type;

    SerializableValue(std::wstring str) : std::wstring(std::move(str)), Type(ValueType::eString) {}
    SerializableValue(const wchar_t* str) : std::wstring(str), Type(ValueType::eString) {}

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
[[nodiscard]] T deserialize_value(const SerializableValue& value);
template<>
[[nodiscard]] inline std::wstring deserialize_value<std::wstring>(const SerializableValue& value) { return static_cast<std::wstring>(value); }
template<>
[[nodiscard]] inline std::string deserialize_value<std::string>(const SerializableValue& value) { return std::narrow(deserialize_value<std::wstring>(value).c_str()); }
template<>
[[nodiscard]] inline bool deserialize_value<bool>(const SerializableValue& value) { return value == L"true"; }
#ifdef __ATLTIME_H__
template<>
[[nodiscard]] inline CTime deserialize_value<CTime>(const SerializableValue& value)
{
    SYSTEMTIME st = { 0 };
    swscanf_s(value.c_str(), L"%hd.%hd.%hd %hd:%hd:%hd", &st.wDay, &st.wMonth, &st.wYear, &st.wHour, &st.wMinute, &st.wSecond);
    return st;
}
#endif
#ifdef __AFXSTR_H__
template<>
[[nodiscard]] inline CString deserialize_value<CString>(const SerializableValue& value) { return deserialize_value<std::wstring>(value).c_str(); }
#endif

template<>
[[nodiscard]] inline std::chrono::system_clock::time_point deserialize_value<std::chrono::system_clock::time_point>(const SerializableValue& value)
{
    std::tm tm = {};
    std::wistringstream(value) >> std::get_time(&tm, L"%d.%m.%Y %H:%M:%S");
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    return std::chrono::system_clock::from_time_t(_mkgmtime(&tm));
#else
    return std::chrono::system_clock::from_time_t(timegm(&tm));
#endif
}

template<>
[[nodiscard]] inline std::filesystem::path deserialize_value<std::filesystem::path>(const SerializableValue& value) { return static_cast<std::wstring>(value); }

template<class T>
[[nodiscard]] T deserialize_value(const SerializableValue& value)
{
    T result;
    std::wistringstream stream(value);
    if constexpr (std::is_enum_v<T>)
    {
        long tempValue;
        stream >> tempValue;
        result = static_cast<T>(tempValue);

        EXT_EXPECT(ext::reflection::is_enum_value<T>(result)) <<
            "Value " << tempValue << " is not a valid enum value for " << ext::type_name<T>();
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        const static std::wstring nanString = []()
        {
            std::wostringstream stream;
            stream << NAN;
            return stream.str();
        }();
        if (value == nanString)
            result = NAN;
        else
            stream >> result;
    }
    else if constexpr (is_duration_v<T>)
    {
        long long val;
        stream >> val;
        result = std::chrono::duration_cast<T>(std::chrono::nanoseconds(val));
    }
    else
        stream >> result;

    return result;
}

// Serialization function, allow convert types to SerializableValue, write your own function for special data type
// or overload std::wistringstream operator<< operator
template<class T>
[[nodiscard]] SerializableValue serialize_value(const T& val)
{
    if constexpr (std::is_enum_v<T>)
        return serialize_value<long>(static_cast<long>(val));
#ifdef __ATLTIME_H__
    else if constexpr (std::is_same_v<CTime, T>)
        return SerializableValue::Create(val.Format(L"%d.%m.%Y %H:%M:%S").GetString(), SerializableValue::ValueType::eDate);
#endif // __ATLTIME_H__
#ifdef __AFXSTR_H__
    else if constexpr (std::is_same_v<CString, T>)
        return serialize_value(std::wstring(val.GetString()));
#endif // __AFXSTR_H__
    else if constexpr (std::is_same_v<bool, T>)
        return SerializableValue::Create(val ? L"true" : L"false", SerializableValue::ValueType::eBool);
    else if constexpr (std::is_same_v<std::chrono::system_clock::time_point, T>)
    {
        std::time_t time_t_point = std::chrono::system_clock::to_time_t(val);
        std::tm gmt;
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        EXT_EXPECT(gmtime_s(&gmt, &time_t_point) == 0);
#else
        EXT_EXPECT(gmtime_r(&time_t_point, &gmt) != 0);
#endif
        return SerializableValue::Create((std::wstringstream() << std::put_time(&gmt, L"%d.%m.%Y %H:%M:%S")).str(), SerializableValue::ValueType::eDate);
    }
    else if constexpr (is_duration_v<T>)
        return SerializableValue::Create((std::wostringstream() << std::chrono::duration_cast<std::chrono::nanoseconds>(val).count()).str(),
                                         SerializableValue::ValueType::eNumber);
    else if constexpr (std::is_same_v<T, std::filesystem::path>)
        return SerializableValue::Create(std::widen(val.string()), SerializableValue::ValueType::eString);
    else
    {
        std::wostringstream stream;
        stream << val;
        if constexpr (std::is_floating_point_v<T>)
            return SerializableValue::Create(stream.str(),
                std::isnan(val) ? SerializableValue::ValueType::eString : SerializableValue::ValueType::eNumber);
        else if constexpr (
            std::is_unsigned_v<T> ||
            std::is_same_v<T, short> ||
            std::is_same_v<T, long> ||
            std::is_same_v<T, long long> ||
            std::is_integral_v<T>)
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eNumber);
        else
            return SerializableValue::Create(stream.str(), SerializableValue::ValueType::eString);
    }
}

} // namespace ext::serializable