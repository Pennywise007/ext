#pragma once

#include <sstream>

#include <ext/core/check.h>
#include <ext/core/defines.h>
#include <ext/error/exception.h>
#include <ext/scope/on_exit.h>

namespace ext {

enum ResultCodes
{
    sOk = 1,
    sFalse = 0,

    eFailed = -1,
    eNotFound = -2,
    eOutOfMemory = -3,
    eUnknown = -4
};

template <typename T>
inline T& operator<<(T& stream, const ::ext::ResultCodes res)
{
    switch (res)
    {
    case ::ext::ResultCodes::sOk:
        return stream << "sOk";
    case ::ext::ResultCodes::sFalse:
        return stream << "sFalse";
    case ::ext::ResultCodes::eFailed:
        return stream << "eFailed";
    case ::ext::ResultCodes::eNotFound:
        return stream << "eNotFound";
    case ::ext::ResultCodes::eOutOfMemory:
        return stream << "eOutOfMemory";
    case ::ext::ResultCodes::eUnknown:
        return stream << "eUnknown";
    }

    EXT_UNREACHABLE();
}

namespace result {

struct CheckResultFailedException : ::ext::exception
{
    explicit CheckResultFailedException(std::source_location&& srcLocation, ::ext::ResultCodes result) noexcept
        : exception(std::move(srcLocation), "", "CheckResultFailedException")
        , m_result(result)
    {}

    [[nodiscard]] std::string external_text() const override
    {
        std::ostringstream externalText;
        externalText << "result: " << (int)m_result << "(" << m_result << ")";
        return externalText.str();
    }

    template <class T>
    auto& operator<<(const T& data)
    {
        exception::operator<<(data);
        return *this;
    }

    ::ext::ResultCodes m_result;
};

} // namespace result

/*
* Show trace error and get result code from exception
*
* Example:
* return ManageExceptionResultCode("Failed to open file");
*/
template <class CharType>
inline ResultCodes ManageExceptionResultCode(const CharType* prefixText) noexcept
{
    EXT_UNUSED(ext::ManageExceptionText(prefixText));
    try
    {
        throw;
    }
    catch (const ::ext::result::CheckResultFailedException& e)
    {
        return e.m_result;
    }
    catch (const std::bad_alloc& e)
    {
        return ::ext::ResultCodes::eOutOfMemory;
    }
    catch (const std::length_error& e)
    {
        return ::ext::ResultCodes::eOutOfMemory;
    }
    catch (...)
    {
        return ::ext::ResultCodes::eUnknown;
    }
}

} // namespace ext

// Checks ext::result expression, in case of fail - throw CheckResultFailedException
// Example:  EXT_CHECK_SUCCEEDED(ext::ResultCode) << "Check failed";
#define EXT_CHECK_SUCCEEDED(expr)       \
    EXT_CHECK_RESULT(expr, EXT_SUCCEEDED(_result), ::ext::result::CheckResultFailedException(std::source_location::current(), _result))

/*
Checks ext::result expression,  if check failes:
* generate breakpoint if debuuger present, othervise - create dump.
* throw exception

Example:
EXT_EXPECT_SUCCEEDED(ext::result) << "Check failed";
*/
#define EXT_EXPECT_SUCCEEDED(expr)      \
    EXT_EXPECT_RESULT(expr, EXT_SUCCEEDED(_result), ::ext::result::CheckResultFailedException(std::source_location::current(), _result))

inline constexpr [[nodiscard]] bool EXT_FAILED(const ext::ResultCodes res) noexcept    { return (res < 0); }
inline constexpr [[nodiscard]] bool EXT_SUCCEEDED(const ext::ResultCodes res) noexcept { return (res >= 0); }
