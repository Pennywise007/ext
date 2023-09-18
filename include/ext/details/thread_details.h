#pragma once

#include <thread>
#include <type_traits>

#include <ext/core/defines.h>

namespace std {

// remove in v142 toolset VS 2019
template <class _Rep, class _Period>
EXT_NODISCARD auto _To_absolute_time_custom(const chrono::duration<_Rep, _Period>& _Rel_time) noexcept {
    constexpr auto _Zero                 = chrono::duration<_Rep, _Period>::zero();
    const auto _Now                      = chrono::steady_clock::now();
    decltype(_Now + _Rel_time) _Abs_time = _Now; // return common type
    if (_Rel_time > _Zero) {
        constexpr auto _Forever = (chrono::steady_clock::time_point::max)();
        if (_Abs_time < _Forever - _Rel_time) {
            _Abs_time += _Rel_time;
        } else {
            _Abs_time = _Forever;
        }
    }
    return _Abs_time;
}
} // namespace std

namespace ext::thread_details {

struct exponential_wait
{
    void operator()() EXT_NOEXCEPT
    {
        constexpr uint32_t fast_limit = 4;
        constexpr uint32_t slow_limit = 8;
        if (m_step < fast_limit)
            FastWait(1u << m_step);
        else if (m_step < slow_limit)
            SlowWait();
        else
            MaxWait();
        ++m_step;
    }

    uint32_t get_step() const EXT_NOEXCEPT { return m_step; }

public:
    static void FastWait(uint32_t mul) EXT_NOEXCEPT
    {
        for (uint32_t i = 0; i < mul; ++i)
#if defined(_WIN32) || defined(__CYGWIN__) // windows
            _mm_pause();
#elif defined(__GNUC__) // linux
//  If X86
#   if defined(__x86_64__) || defined(__amd64__) || defined(__i686__) || defined(__i586__) || defined(__i486__) || defined(__i386__)
        __asm__ __volatile__("rep; nop" ::: "memory");
//  If ARM
#   elif defined(__arm64) || defined(__arm64__) || defined(__aarch64__) || defined(__ARM__) || defined(__arm__)
//      If no yield support        
#       if  defined(__ARM_ARCH_2__)     || \ 
            defined(__ARM_ARCH_3__)     || \
            defined(__ARM_ARCH_3M__)    || \
            defined(__ARM_ARCH_4T__)    || \
            defined(__TARGET_ARM_4T)    || \
            defined(__ARM_ARCH_5__)     || \
            defined(__ARM_ARCH_5E__)    || \
            defined(__ARM_ARCH_5T__)    || \
            defined(__ARM_ARCH_5TE__)   || \
            defined(__ARM_ARCH_5TEJ__)  || \
            defined(__ARM_ARCH_6__)     || \
            defined(__ARM_ARCH_6J__)
            __asm__ __volatile__("nop" ::: "memory");
#       else
            __asm__ __volatile__("yield" ::: "memory");
#       endif
#   endif
#else
#error Not implemented
#endif
    }
    static void SlowWait() EXT_NOEXCEPT
    {
        std::this_thread::yield();
    }
    static void MaxWait() EXT_NOEXCEPT
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

private:
    uint32_t m_step{ 0 };
};

} // namespace ext::thread_details
