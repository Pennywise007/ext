#pragma once

#include <atomic>

namespace ext::call_once {

template <uint32_t id>
struct CallOnceGuard
{
    inline static std::atomic_bool called = false;
    static bool CanCall() { return called ? false : (called = true, true); }
};

} // namespace ext::call_once

// Call code only once during execution
#define CALL_ONCE(expr)                                                 \
    if (!ext::call_once::CallOnceGuard<__COUNTER__>::CanCall()) {}      \
    else { expr; }
