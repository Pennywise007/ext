#pragma once

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif

#include <algorithm>
#include <exception>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ext/core/check.h>
#include <ext/reflection/object.h>
#include <ext/std/string.h>

namespace ext::core {

// Command line arguments parser, support flags in format --flag=value, --flag value and -f value,
// also support aliases for flags, e.g. for "port" flag alias can be "p", for "verbose" - "v"
class CommandLineParser
{
public:
    struct FlagInfo
    {
        // Short summary for the flag, e.g. "Server port"
        std::wstring summary;
        // Detailed description for the flag, can be empty
        std::wstring description;
        // Short alias for the flag, e.g. for "port" flag alias can be "p", for "verbose" - "v"
        std::wstring alias;
        // Is flag required or not, if required and not provided - error will be thrown during parsing
        bool required = false;
    };

    CommandLineParser()
        : m_values(ParseArguments(GetNativeCommandLineArguments()))
    {}

    explicit CommandLineParser(int argc, const wchar_t* const argv[])
        : m_values(ParseArguments(argc, argv))
    {}

    // Register flag with name and info
    void RegisterFlag(std::wstring_view name, FlagInfo&& info)
    {
        const std::wstring normalizedName = NormalizeFlagName(name);
        if (normalizedName.empty() || info.summary.empty())
        {
            EXT_ASSERT(false) << EXT_TRACE_FUNCTION << "Flag name and summary cannot be empty";
            return;
        }
        const auto normalizedAlias = NormalizeFlagName(info.alias);
        EXT_ASSERT(normalizedAlias != normalizedName) << EXT_TRACE_FUNCTION << "Alias cannot be the same as flag name";
        info.alias = normalizedAlias;

        m_flagInfo[normalizedName] = std::move(info);
    }

    // Register custom help message, if not registered - help will be generated based on registered flags
    void RegisterHelp(std::wstring_view help)
    {
        m_help = std::wstring(help);
    }

    // Get flag value by name, returns false if flag is not found or failed to parse value
    template <typename T>
    bool GetFlag(std::wstring_view flagName, T& field) const noexcept
    {
        const std::wstring normalized = NormalizeFlagName(flagName);
        if (normalized.empty())
            return false;

        auto valueIt = m_values.find(normalized);
        if (valueIt == m_values.end())
        {
            std::wstring alias;
            bool aliasFound = FindSecondaryName(normalized, alias);
            if (!aliasFound)
                return false;

            valueIt = m_values.find(alias);
            if (valueIt == m_values.end())
                return false;
        }

        return ParseValue(valueIt->second, field);
    }

    // Get flags from object fields, field names are used as flag names, returns false if required flag is missing or failed to parse value
    // Example:
    //      struct Flags { int port; bool verbose; };
    //      Flags flags;
    //      parser.GetFlags(flags);
    template <typename Type>
    void GetFlags(Type& obj) const
    {
        std::vector<std::string> missedRequiredFlags;

        ext::reflection::for_each_field(obj, [&](std::string_view fieldName, auto& field) {
            const std::wstring fieldNameStr(std::widen(std::string(fieldName.data(), fieldName.size())));

            const bool res = GetFlag(fieldNameStr, field);
            constexpr bool optionalField = is_std_optional<std::remove_cvref_t<decltype(field)>>::value;

            if constexpr (!optionalField)
            {
                if (!res)
                    missedRequiredFlags.emplace_back(std::string(fieldName.data(), fieldName.size()));
            }
        });

        if (!missedRequiredFlags.empty())
        {
            std::string message = "Missing required flags: ";
            for (size_t index = 0; index < missedRequiredFlags.size(); ++index)
            {
                message += "--" + missedRequiredFlags[index];
                if (index + 1 < missedRequiredFlags.size())
                    message += ", ";
            }
            throw std::runtime_error(message);
        }
    }

    // Get help message, if custom help is registered - returns it, otherwise generates help based on registered flags
    std::wstring GetHelp() const noexcept
    {
        if (!m_help.empty())
            return m_help;

        if (m_flagInfo.empty())
            return L"Available flags are not registered.";

        std::wstring requiredFlags;
        std::wstring optionalFlags;
        
        for (auto&& [flagName, info] : m_flagInfo)
        {
            std::wstring result = L"  --" + flagName;
            if (!info.alias.empty())
                result += L", -" + info.alias;

            result += L"\t" + info.summary;
            if (!info.description.empty())
                result += L": " + info.description;

            if (info.required)
                requiredFlags += std::move(result) + L"\n";
            else
                optionalFlags += std::move(result) + L"\n";
        }

        std::wstring result;
        if (requiredFlags.empty())
            return L"Available flags:\n" + std::move(optionalFlags);

        result = L"Required flags:\n" + std::move(requiredFlags) + L"\n";
        if (!optionalFlags.empty())
            result += L"Optional flags:\n" + std::move(optionalFlags);

        return result;
    }

private:
    static std::wstring NormalizeFlagName(std::wstring_view raw) noexcept
    {
        auto it = std::find_if_not(raw.begin(), raw.end(), [](wchar_t c) { return c == L'-'; });
        raw.remove_prefix(std::distance(raw.begin(), it));
        return std::wstring(raw);
    }

    // Returns normalized name for the flag, if alias is provided - returns alias, otherwise - normalized name
    bool FindSecondaryName(std::wstring_view normalizedName, std::wstring& secondaryName) const noexcept
    {
        for (auto&& [flagName, info] : m_flagInfo)
        {
            if (flagName == normalizedName)
            {
                if (!info.alias.empty() && info.alias != normalizedName)
                {
                    secondaryName = info.alias;
                    return true;
                }
                return false;
            }
            if (!info.alias.empty() && info.alias == normalizedName)
            {
                secondaryName = flagName;
                return true;
            }
        }
        return false;
    }

    static std::vector<std::wstring> GetNativeCommandLineArguments()
    {
#if defined(_WIN32)
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv == nullptr)
            return {};

        std::vector<std::wstring> result;
        result.reserve(argc);
        for (int index = 0; index < argc; ++index)
            result.emplace_back(argv[index]);

        LocalFree(argv);
        return result;
#else
        std::vector<std::wstring> result;
        std::ifstream input("/proc/self/cmdline", std::ios::binary);
        if (!input)
        {
            const auto exePath = std::filesystem::get_full_exe_path();
            if (!exePath.empty())
                result.emplace_back(exePath.wstring());
            return result;
        }

        std::string raw;
        raw.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
        const std::wstring wideRaw = std::widen(raw);

        size_t offset = 0;
        while (offset < wideRaw.size())
        {
            const size_t next = wideRaw.find(L'\0', offset);
            if (next == std::wstring::npos)
                break;
            if (next > offset)
                result.emplace_back(wideRaw.data() + offset, next - offset);
            offset = next + 1;
        }
        return result;
#endif
    }

    template <typename Target>
    static bool ParseValue(std::wstring_view rawValue, Target& output) noexcept
    {
        if constexpr (is_std_optional<std::remove_cvref_t<Target>>::value)
        {
            using ValueType = typename std::remove_cvref_t<Target>::value_type;
            std::optional<ValueType> temp;
            if (!ParseValue(rawValue, temp.emplace()))
                return false;
            output = std::move(temp);
            return true;
        }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Target>, bool>)
        {
            return ParseBoolean(rawValue, output);
        }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Target>, std::wstring>)
        {
            output = std::wstring(rawValue);
            return true;
        }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Target>, std::string>)
        {
            output = std::narrow(rawValue);
            return true;
        }
        else if constexpr (std::is_same_v<std::remove_cvref_t<Target>, std::wstring_view>)
        {
            output = rawValue;
            return true;
        }
        else if constexpr (std::is_enum_v<std::remove_cvref_t<Target>>)
        {
            using Underlying = std::underlying_type_t<std::remove_cvref_t<Target>>;
            Underlying parsed = {};
            if (!ParseValue(rawValue, parsed))
                return false;
            output = static_cast<std::remove_cvref_t<Target>>(parsed);
            return true;
        }
        else if constexpr (std::is_integral_v<std::remove_cvref_t<Target>>)
        {
            if (rawValue.empty())
                return false;

            const std::string narrowValue = std::narrow(std::wstring(rawValue.data(), rawValue.size()));
            auto begin = narrowValue.data();
            auto end = narrowValue.data() + narrowValue.size();
            const auto result = std::from_chars(begin, end, output);
            return result.ec == std::errc() && result.ptr == end;
        }
        else if constexpr (std::is_floating_point_v<std::remove_cvref_t<Target>>)
        {
            if (rawValue.empty())
                return false;
            const std::string narrowValue = std::narrow(rawValue);
            char* parseEnd = nullptr;
            errno = 0;
            const long double value = std::strtold(narrowValue.c_str(), &parseEnd);
            if (parseEnd != narrowValue.c_str() + narrowValue.size() || errno == ERANGE)
                return false;
            output = static_cast<std::remove_cvref_t<Target>>(value);
            return true;
        }
        else
        {
            return false;
        }
    }

    static bool ParseBoolean(std::wstring_view rawValue, bool& output) noexcept
    {
        if (rawValue.empty())
        {
            output = true;
            return true;
        }

        const auto lower = ToLower(rawValue);
        if (lower == L"true" || lower == L"1" || lower == L"yes" || lower == L"on")
        {
            output = true;
            return true;
        }
        if (lower == L"false" || lower == L"0" || lower == L"no" || lower == L"off")
        {
            output = false;
            return true;
        }
        return false;
    }

    static std::wstring ToLower(std::wstring_view value) noexcept
    {
        std::wstring result;
        result.reserve(value.size());
        for (wchar_t c : value)
            result.push_back(static_cast<wchar_t>(std::towlower(c)));
        return result;
    }

    static std::unordered_map<std::wstring, std::wstring> ParseArguments(int argc, const wchar_t* const argv[])
    {
        std::vector<std::wstring> result;
        result.reserve(argc);
        for (int index = 0; index < argc; ++index)
            result.emplace_back(argv[index]);
        return ParseArguments(result);
    }

    static std::unordered_map<std::wstring, std::wstring> ParseArguments(const std::vector<std::wstring>& args)
    {
        std::unordered_map<std::wstring, std::wstring> parsed;
        for (size_t index = 1; index < args.size(); ++index)
        {
            const std::wstring& token = args[index];
            if (token.size() < 1 || token[0] != '-')
                continue;

            const bool isLong = token.size() >= 2 && token[1] == '-';
            const std::wstring raw = isLong ? token.substr(2) : token.substr(1);
            if (raw.empty())
                continue;

            const size_t equalPos = raw.find('=');
            const std::wstring name = NormalizeFlagName(equalPos == std::wstring::npos ? raw : raw.substr(0, equalPos));
            const std::wstring value = equalPos == std::wstring::npos
                ? (index + 1 < args.size() && args[index + 1].size() > 0 && args[index + 1][0] != '-' ? args[++index] : std::wstring())
                : raw.substr(equalPos + 1);

            parsed[name] = value;
        }
        return parsed;
    }

    template <typename T>
    struct is_std_optional : std::false_type {};

    template <typename T>
    struct is_std_optional<std::optional<T>> : std::true_type {};

    const std::unordered_map<std::wstring, std::wstring> m_values;
    std::unordered_map<std::wstring, FlagInfo> m_flagInfo;
    std::wstring m_help;
};

} // namespace ext::core
