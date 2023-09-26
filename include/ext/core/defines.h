#pragma once

// Unused variable or return value
#define EXT_UNUSED(variable) static_cast<void>(variable)
#define EXT_IGNORE_RESULT(...) static_cast<void>(__VA_ARGS__)

// Functions declaration
#define EXT_NODISCARD       [[nodiscard]]
#define EXT_NOEXCEPT        noexcept
#define EXT_THROWS(...)     noexcept(false)

#define EXT_CALL            __cdecl
#if defined(_WIN32) || defined(__CYGWIN__) // windows
#define EXT_NOTHROW         __declspec(nothrow)
#define EXT_NOINLINE        __declspec(noinline)
#define EXT_FORCEINLINE     __forceinline
#elif defined(__GNUC__) // linux
#define EXT_NOTHROW         __attribute__((nothrow))
#define EXT_NOINLINE        __attribute__((noinline))
#define EXT_FORCEINLINE     __attribute__((always_inline))
#endif


// EXT_PP_CAT(x, y) => xy
#define EXT_PP_EXPAND(x)    x
#define EXT_PP_CAT_I(x, y)  EXT_PP_EXPAND(x##y)
#define EXT_PP_CAT(x, y)    EXT_PP_CAT_I(x, y)

// STRINGINIZE(ttt) = "ttt"
#define TO_STRING(x)        #x
#define STRINGINIZE(val)    TO_STRING(val)

// REMOVE_PARENTHESES((X)) => X
#define REMOVE_PARENTHESES(X) ESC(ISH X)
#define ISH(...) ISH __VA_ARGS__
#define ESC(...) ESC_(__VA_ARGS__)
#define ESC_(...) VAN ## __VA_ARGS__
#define VANISH
