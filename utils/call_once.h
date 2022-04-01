#pragma once

#include <atomic>

#include <ext/core/defines.h>

namespace ext::call_once {

template <uint32_t id>
struct CallOnceGuard
{
    inline static std::atomic_bool called = false;
    static bool CanCall() { return called ? false : (called = true, true); }
};

} // namespace ext::call_once

// Call code only once during execution
// Examples:
//
// CALL_ONCE(val = 9);
// CALL_ONCE(( val = 9; val2 = 3; ));
#define CALL_ONCE(expr)                                                 \
    if (!ext::call_once::CallOnceGuard<__COUNTER__>::CanCall()) {}      \
    else { REMOVE_PARENTHES(expr); }
