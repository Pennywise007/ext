#pragma once

#include <stdio.h>
#include <stdint.h>
#include <type_traits>

#include <ext/core/tracer.h>
#include <ext/error/exception.h>
#include <ext/error/dump_writer.h>
#include <ext/utils/call_once.h>

namespace ext::check {

struct CheckFailedException : ::ext::exception
{
    explicit CheckFailedException(std::source_location&& source, const char* expression) noexcept
        : exception(std::move(source), "", "CheckFailedException")
        , m_expression(expression)
    {}

    [[nodiscard]] std::string external_text() const override { return "expression: '" + m_expression + "'"; }

    template <class T>
    CheckFailedException& operator<<(const T& data) { ::ext::exception::operator<<(data); return *this; }

    std::string m_expression;
};

} // namespace ext::check

// Checks expression via check function, if check fails - throw exception
// Example:  EXT_CHECK_RESULT(val, _result == 0, ::ext::check::CheckFailedException(std::source_location::current(), "Failed")) << "Text";
#define EXT_CHECK_RESULT(expr, check_func, exception)                       \
    for (auto &&__result = (expr); !(check_func);)                          \
        throw exception

// Checks boolean expression, in case of fail - throw CheckFailedException
// Example:  EXT_CHECK(val == true) << "Check failed";
#define EXT_CHECK(bool_expression)      \
    EXT_CHECK_RESULT(bool_expression, !!(__result), ::ext::check::CheckFailedException(std::source_location::current(), #bool_expression))

/*
Checks expression via check function, if check fails:
* generate breakpoint if debugger present, otherwise - create dump.
* throw exception

Example:
EXT_EXPECT_RESULT(val, _result == 0, ::ext::check::CheckFailedException(std::source_location::current(), "Value check failed")) << "Check failed";
*/
#define EXT_EXPECT_RESULT(expr, check_func, exception)                              \
    for (auto &&__result = (expr); !(check_func);)                                  \
         for (bool __firstEnter = true;; __firstEnter = false)                      \
            if (__firstEnter)                                                       \
            {                                                                       \
                CALL_ONCE(DEBUG_BREAK_OR_CREATE_DUMP())                             \
            }                                                                       \
            else                                                                    \
                throw exception

/*
Checks boolean expression or HRESULT,  if check fails:
* generate breakpoint if debugger present, otherwise - create dump.
* throw exception

Example:
EXT_EXPECT(val == true) << "Check failed";
*/
#define EXT_EXPECT(bool_expression)     \
    EXT_EXPECT_RESULT(bool_expression, !!(__result), ::ext::check::CheckFailedException(std::source_location::current(), #bool_expression))

/*
Checks boolean expression or HRESULT in DEBUG, if check fails:
* generate breakpoint if debugger present, otherwise - create dump.
* show error in log

Example:
EXT_ASSERT(val == true) << "Check failed";
*/
#ifdef _DEBUG
#define EXT_ASSERT(bool_expression) EXT_DUMP_IF(!(bool_expression))
#else
#define EXT_ASSERT(...) if (true) {} else EXT_TRACE_ERR()
#endif	// _DEBUG

/* Unreachable code
*
* if (true) return;
* EXT_UNREACHABLE()
*/
#ifdef __GNUC__
#define EXT_UNREACHABLE() EXT_ASSERT(false) << EXT_TRACE_FUNCTION; __builtin_unreachable()
#else
#define EXT_UNREACHABLE() EXT_ASSERT(false) << EXT_TRACE_FUNCTION; __assume(0)
#endif

/*
Checks boolean expression or HRESULT, if check fails:
* generate breakpoint if debugger present, otherwise - create dump.
* throw exception

Example:
EXT_REQUIRE(MainInterface) << "No main interface";
*/
#define EXT_REQUIRE(bool_expression) EXT_ASSERT(bool_expression)
