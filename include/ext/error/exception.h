#pragma once

/*
 * Allow to manage exception text
 *
 * Example

#include <ext/error/exception.h>

try
{
    EXT_EXPECT(is_ok()) << "Something wrong!";
}
catch (...)
{
    try
    {
        std::throw_with_nested(ext::exception(EXT_SRC_LOCATION, "Job failed"));
    }
    catch (...)
    {
        ::MessageBox(NULL, ext::ManageExceptionText("Big bang"));
    }
}
 */
#include <algorithm>
#if defined(_WIN32) || defined(__CYGWIN__) // windows
#include <comdef.h>     // _com_error
#endif // windows
#include <exception>
#include <filesystem>
#include <locale>
#include <string>
#include <type_traits>
#include <sstream>

#include <ext/core/defines.h>
#include <ext/core/tracer.h>
#include <ext/std/string.h>
#include <ext/std/type_traits.h>

namespace ext {

// todo: remove source_location when c++20 will be available
class source_location
{
public:
    constexpr source_location() noexcept = default;

    constexpr source_location(const char* file, int line) noexcept
        : m_fileName(file)
        , m_fileLine(line)
    {}

    std::string to_string() const noexcept
    {
        return std::string_sprintf("\'%s\'(%d)", m_fileName, m_fileLine);
    }

    constexpr bool exist() const noexcept
    {
        return m_fileName != nullptr;
    }

private:
    const char* m_fileName = nullptr;
    const int m_fileLine = 0;
};

#define EXT_SRC_LOCATION ext::source_location(__FILE__, __LINE__)

struct exception : std::exception
{
    explicit exception(source_location&& source, const char* description = "", const char* exceptionType = "Exception") noexcept
        : m_exceptionType(exceptionType)
        , m_description(description)
        , m_source(std::move(source))
    {
        EXT_TRACE_ERR() << to_string().c_str();
    }

    explicit exception(const char* description, const char* exceptionType = "Exception") noexcept
        : m_exceptionType(exceptionType)
        , m_description(description)
    {
        EXT_TRACE_ERR() << to_string().c_str();
    }
    explicit exception(const wchar_t* description, const char* exceptionType = "Exception") noexcept
        : m_exceptionType(exceptionType)
        , m_description(std::narrow(description).c_str())
    {
        EXT_TRACE_ERR() << to_string().c_str();
    }

    [[nodiscard]] std::string to_string() const
    {
        auto externalText = external_text();
        if (!externalText.empty())
            externalText = " - " + std::move(externalText);

        std::string text = std::string_sprintf("%s %s%s", m_description.c_str(), m_exceptionType, externalText.c_str());
        if (m_source.has_value())
            text += " At " + m_source->to_string() + ".";
        std::string_trim_all(text);
        return text;
    }

    [[nodiscard]] virtual std::string external_text() const { return {}; }
    virtual const char* what() const noexcept override { return m_description.c_str(); }

    template <class T>
    auto& operator<<(const T& data)
    {
        std::ostringstream string;
        string << data;
        m_description += string.str();
        return *this;
    }

private:
    template <typename Stream>
    inline friend Stream& operator<<(Stream& stream, const exception& exception)
    {
        stream << exception.to_string().c_str();
        return stream;
    }

private:
    const char* m_exceptionType;
    std::string m_description;

    std::optional<source_location> m_source;
};

/*
* Show trace error and get text from exception
*
* Example:
  ::MessageBox(NULL, ManageExceptionText("Failed to open file"));

* Example 2
  try { throw ext::exception("Failed to do sth"); }
  catch (...)
  {
      try { std::throw_with_nested(ext::exception(ext::source_location("File name", 11), "Job failed")); }
      catch (...)
      {
          EXPECT_STREQ("Job failed Exception At 'File name'(11).\nFailed to do sth Exception", ext::ManageExceptionText("Main error catched"));
      }
  }
*/
template <class CharType>
inline auto /*std::string*/ManageExceptionText(const CharType* prefixText = nullptr, bool splitExceptionText = true) noexcept
{
    // std::ostringstream / wostringstream
    std::basic_ostringstream<CharType> exceptionStream;
    if (prefixText)
        exceptionStream << prefixText;
    if (!exceptionStream.str().empty())
        exceptionStream << ".";
    else
        exceptionStream << "Error.";

    if (splitExceptionText)
        exceptionStream << "\n\nException: ";
    else
        exceptionStream << " Exception: ";

    auto exception_to_stream = [&exceptionStream]()
    {
        try
        {
            throw;
        }
        catch (const ::ext::exception& e)
        {
            if constexpr (std::is_same_v<CharType, wchar_t>)
                exceptionStream << std::widen(e.to_string().c_str());
            else
                exceptionStream << e.to_string().c_str();
        }
#ifdef __AFX_H__
        catch (CException* e)
        {
            WCHAR errorText[MAX_PATH];
            e->GetErrorMessage(errorText, MAX_PATH);

            if constexpr (std::is_same_v<CharType, char>)
                exceptionStream << std::narrow(errorText);
            else
                exceptionStream << errorText;
        }
#endif // __AFX_H__
        catch (const std::exception& e)
        {
            if constexpr (std::is_same_v<CharType, wchar_t>)
                exceptionStream << std::widen(e.what());
            else
                exceptionStream << e.what();
        }
#if defined(_WIN32) || defined(__CYGWIN__) // windows
        catch (const _com_error& e)
        {
            if constexpr (std::is_same_v<CharType, TCHAR>)
                exceptionStream << e.ErrorMessage();
            else if constexpr (std::is_same_v<CharType, wchar_t>)
                exceptionStream << std::widen(e.ErrorMessage());
            else
                exceptionStream << std::narrow(e.ErrorMessage());
        }
#endif
        catch (...)
        {
            exceptionStream << "Unknown type of exception";
        }
    };

    std::function<void(const std::optional<std::nested_exception>&)> print_exception_stack;
    print_exception_stack = [&](const std::optional<std::nested_exception>& e)
    {
        try
        {
            if (!e.has_value())
                throw;

            if (e->nested_ptr())
                e->rethrow_nested();
        }
        catch (const std::nested_exception& e)
        {
            exception_to_stream();
            if (splitExceptionText)
                exceptionStream << "\n";
            print_exception_stack(e);
        }
        catch (...)
        {
            exception_to_stream();
        }
    };

    print_exception_stack(std::nullopt);

    const auto string = exceptionStream.str();
    EXT_TRACE_ERR() << string;
    return string;
}

/*
* Show trace error and get text from exception
*
* Example:
* ManageException(EXT_TRACE_FUNCTION));
*/
inline void ManageException(const char* prefixText = nullptr) noexcept
{
    EXT_UNUSED(ManageExceptionText(prefixText, false));
}

inline void ManageException(const wchar_t* prefixText = nullptr) noexcept
{
    EXT_UNUSED(ManageExceptionText(prefixText, false));
}

} // namespace ext