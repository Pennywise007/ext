#pragma once

#include <yvals_core.h>

// Unused variable or return value
#define EXT_UNUSED(variable)    static_cast<void>(variable)
#define EXT_IGNORE_RESULT(...)  static_cast<void>(__VA_ARGS__)

// Functions declaration
#define EXT_NODISCARD       _NODISCARD
#define EXT_NOEXCEPT        noexcept
#define EXT_THROWS(...)     noexcept(false)

#define EXT_NOINLINE        __declspec(noinline)
#define EXT_FORCEINLINE     __forceinline
#define EXT_NOTHROW         __declspec(nothrow)
#define EXT_CALL            __cdecl

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
