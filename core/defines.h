#pragma once

#include <yvals_core.h>

// Unused variable or return value
#define SSH_UNUSED(variable) static_cast<void>(variable)

// Functions declaration
#define SSH_NODISCARD   _NODISCARD
#define SSH_NOEXCEPT    noexcept
#define SSH_THROWS(...)  noexcept(false)

/* Unreachable code
 *
 * if (true) return;
 * SSH_UNREACHABLE()
*/
#ifdef __GNUC__
#define SSH_UNREACHABLE() __builtin_unreachable()
#else
#define SSH_UNREACHABLE() __assume(0)
#endif

// SSH_PP_CAT(x, y) => xy
#define SSH_PP_EXPAND(x)  x
#define SSH_PP_CAT_I(x,y) SSH_PP_EXPAND(x##y)
#define SSH_PP_CAT(x,y) SSH_PP_CAT_I(x,y)

// STRINGINIZE(ttt) = "ttt"
#define TO_STRING(x) #x
#define STRINGINIZE(val) TO_STRING(val)

// REMOVE_PARENTHES((X)) => "X"
#define REMOVE_PARENTHES(X) ESC(ISH X)
#define ISH(...) ISH __VA_ARGS__
#define ESC(...) VAN ## __VA_ARGS__
#define VANISH
