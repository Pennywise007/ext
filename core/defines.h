#pragma once

#include <yvals_core.h>

// Unused variable or return value
#define EXT_UNUSED(variable) static_cast<void>(variable)

// Functions declaration
#define EXT_NODISCARD   _NODISCARD
#define EXT_NOEXCEPT    noexcept
#define EXT_THROWS(...)  noexcept(false)

/* Unreachable code
 *
 * if (true) return;
 * EXT_UNREACHABLE()
*/
#ifdef __GNUC__
#define EXT_UNREACHABLE() __builtin_unreachable()
#else
#define EXT_UNREACHABLE() __assume(0)
#endif

// EXT_PP_CAT(x, y) => xy
#define EXT_PP_EXPAND(x)  x
#define EXT_PP_CAT_I(x,y) EXT_PP_EXPAND(x##y)
#define EXT_PP_CAT(x,y) EXT_PP_CAT_I(x,y)

// STRINGINIZE(ttt) = "ttt"
#define TO_STRING(x) #x
#define STRINGINIZE(val) TO_STRING(val)

// REMOVE_PARENTHES((X)) => "X"
#define REMOVE_PARENTHES(X) ESC(ISH X)
#define ISH(...) ISH __VA_ARGS__
#define ESC(...) VAN ## __VA_ARGS__
#define VANISH
