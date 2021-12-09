#pragma once

#include <atomic>
#include <debugapi.h>
#include <stdio.h>
#include <stdint.h>
#include <type_traits>
#include <winerror.h>

#include <ext/core/defines.h>
#include <ext/error/exception.h>
#include <ext/error/dump_writer.h>
#include <ext/trace/tracer.h>
#include <ext/utils/call_once.h>


namespace ext::check {

struct CheckFailedException : ::ext::exception
{
    explicit CheckFailedException(source_location&& source, const char* expression) noexcept
        : exception(std::move(source), "", "CheckFailedException")
        , m_expression(expression)
    {}

    SSH_NODISCARD std::string external_text() const override { return "expression: '" + m_expression + "'"; }

    template <class T>
    auto& operator<<(const T& data) { ::ext::exception::operator<<(data); return *this; }

    std::string m_expression;
};

} // namespace ext::check

// Checks expression via check function, if check failes - throw exception
// Example:  SSH_CHECK_RESULT(val, _result == 0, ::ext::check::CheckFailedException(__FILE__, __LINE__, "Failed")) << "Text";
#define SSH_CHECK_RESULT(expr, check_func, exception)                   \
    for (auto &&__result = (expr); !(check_func);)                       \
        throw exception

// Checks boolean expression, in case of fail - throw CheckFailedException
// Example:  SSH_CHECK(val == true) << "Check failed";
#define SSH_CHECK(bool_expression)      \
    SSH_CHECK_RESULT(bool_expression, !!(__result), ::ext::check::CheckFailedException(SSH_SRC_LOCATION, #bool_expression))

/*
Checks expression via check function, if check failes:
* generate breakpoint if debuuger present, othervise - create dump.
* throw exception

Example:
SSH_EXPECT_RESULT(val, _result == 0, ::ext::check::CheckFailedException(__FILE__, __LINE__, "Valueu check failed")) << "Check failed";
*/
#define SSH_EXPECT_RESULT(expr, check_func, exception)                              \
    for (auto &&__result = (expr); !(check_func);)                                   \
         for (bool __firstEnter = true;; __firstEnter = false)                      \
            if (__firstEnter)                                                       \
            {                                                                       \
                CALL_ONCE(DEBUG_BREAK_OR_CREATE_DUMP)                               \
            }                                                                       \
            else                                                                    \
                throw exception

/*
Checks boolean expression or HRESULT,  if check failes:
* generate breakpoint if debuuger present, othervise - create dump.
* throw exception

Example:
SSH_EXPECT(val == true) << "Check failed";
*/
#define SSH_EXPECT(bool_expression)     \
    SSH_EXPECT_RESULT(bool_expression, !!(__result), ::ext::check::CheckFailedException(SSH_SRC_LOCATION, #bool_expression))

/*
Checks boolean expression or HRESULT, if check failes:
* generate breakpoint if debuuger present, othervise - create dump.
* throw exception

Example:
SSH_REQUIRE(MainInterface) << "No main interface";
*/
#define SSH_REQUIRE(bool_expression) SSH_EXPECT(bool_expression)

/*
Checks boolean expression or HRESULT in DEBUG, if check failes:
* generate breakpoint if debugger present, otherwise - create dump.
* show error in log

Example:
SSH_ASSERT(val == true) << "Check failed";
*/
#ifdef _DEBUG
#define SSH_ASSERT(bool_expression) SSH_DUMP_IF(!(bool_expression))
#else
#define SSH_ASSERT(bool_expression) if (true) {} else SSH_TRACE_ERR()
#endif	// _DEBUG
