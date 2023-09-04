// @see https://github.com/microsoft/STL/blob/main/stl/inc/atomic

#pragma once

#include <ext/core/defines.h>

#if defined(_WIN32) || defined(__CYGWIN__) // windows

    #if defined(_M_ARM) || defined(_M_ARM64)

        #include <intrin.h>

        extern "C"
        {
            void __dmb(unsigned int);
            __int32 __iso_volatile_load32(const volatile __int32 *);

    #       pragma intrinsic(__dmb)
    #       pragma intrinsic(__iso_volatile_load32)
        }

    #   define INTERLOCKED_OP_ACQUIRE(op)    _Interlocked ## op ## _acq
    #   define INTERLOCKED_OP_RELEASE(op)    _Interlocked ## op ## _rel
    #   define INTERLOCKED_OP_SEQ_CST(op)    _Interlocked ## op

    #elif defined(_M_IX86) || defined(_M_AMD64)
        extern "C"
        {
            void _ReadWriteBarrier(void);
    #       pragma intrinsic(_ReadWriteBarrier)
        }

    #   define INTERLOCKED_OP_ACQUIRE(op)    _Interlocked ## op
    #   define INTERLOCKED_OP_RELEASE(op)    _Interlocked ## op
    #   define INTERLOCKED_OP_SEQ_CST(op)    _Interlocked ## op
    #else
    #   error Unsupported platform
    #endif

    extern "C"
    {
    long __cdecl INTERLOCKED_OP_RELEASE(ExchangeAdd)(long volatile *, long);
    long __cdecl INTERLOCKED_OP_RELEASE(Exchange)(long volatile *, long);
    long __cdecl INTERLOCKED_OP_ACQUIRE(CompareExchange)(long volatile *, long, long);

    #pragma intrinsic(INTERLOCKED_OP_RELEASE(ExchangeAdd))
    #pragma intrinsic(INTERLOCKED_OP_RELEASE(Exchange))
    #pragma intrinsic(INTERLOCKED_OP_ACQUIRE(CompareExchange))
    }

#elif defined(__GNUC__) // linux
#include <atomic>
#else
#   error Unsupported platform
#endif
namespace ext {
namespace detail {

EXT_NOTHROW inline void atomic_thread_fence_acquire() EXT_NOEXCEPT
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    #if defined(_M_ARM) || defined(_M_ARM64)
        __dmb(0xB); // 0xB == _ARM_BARRIER_ISH == _ARM64_BARRIER_ISH
    #elif defined(_M_IX86) || defined(_M_AMD64)
        _ReadWriteBarrier();
    #else
    #   error Unsupported platform
    #endif
#elif defined(__GNUC__) // linux
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
#else
#   error Unsupported platform
#endif
}

EXT_NOTHROW inline uint32_t atomic_uint32_load_relaxed(const uint32_t *ptr) EXT_NOEXCEPT
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    #if defined(_M_ARM) || defined(_M_ARM64)
        return __iso_volatile_load32(reinterpret_cast<const volatile __int32 *>(ptr));
    #elif defined(_M_IX86) || defined(_M_AMD64)
        return *ptr;
    #else
    #   error Unsupported platform
    #endif
#elif defined(__GNUC__) // linux
    return __atomic_load_n(ptr, __ATOMIC_RELAXED);
#else
#   error Unsupported platform
#endif
}

EXT_NOTHROW inline uint32_t atomic_uint32_load_acquire(const uint32_t *ptr) EXT_NOEXCEPT
{
    auto val = atomic_uint32_load_relaxed(ptr);
    atomic_thread_fence_acquire();
    return val;
}

EXT_NOTHROW inline uint32_t atomic_uint32_sub_fetch_release(uint32_t *ptr, uint32_t val) EXT_NOEXCEPT
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    return static_cast<uint32_t>(
        INTERLOCKED_OP_RELEASE(ExchangeAdd)(
            reinterpret_cast<volatile long *>(ptr), -static_cast<long>(val))
        - static_cast<long>(val));
#elif defined(__GNUC__) // linux
    return __atomic_sub_fetch(ptr, val, __ATOMIC_RELEASE);
#endif
}

EXT_NOTHROW inline bool atomic_uint32_compare_exchange_weak_acquire_relaxed(uint32_t *ptr, uint32_t *cmp, uint32_t with) EXT_NOEXCEPT
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    uint32_t old = INTERLOCKED_OP_ACQUIRE(CompareExchange)(
        reinterpret_cast<volatile long *>(ptr),
        static_cast<long>(with),
        static_cast<long>(*cmp)
    );

    if (old == *cmp)
        return true;

    *cmp = old;
    return false;
#elif defined(__GNUC__) // linux
    constexpr bool weak = true;
    return __atomic_compare_exchange_n(ptr, cmp, with, weak, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED);
#endif
}

EXT_NOTHROW inline bool atomic_uint32_compare_exchange_weak_acqrel_acquire(uint32_t *ptr, uint32_t *cmp, uint32_t with) EXT_NOEXCEPT
{
#if defined(_WIN32) || defined(__CYGWIN__) // windows
    uint32_t old = INTERLOCKED_OP_SEQ_CST(CompareExchange)(
        reinterpret_cast<volatile long *>(ptr),
        static_cast<long>(with),
        static_cast<long>(*cmp)
    );

    if (old == *cmp)
        return true;

    *cmp = old;
    return false;
#elif defined(__GNUC__) // linux
    constexpr bool weak = true;
    return __atomic_compare_exchange_n(ptr, cmp, with, weak, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#endif
}

} // namespace detail
} // namespace ext

#if defined(_WIN32) || defined(__CYGWIN__) // windows
#undef INTERLOCKED_OP_ACQUIRE
#undef INTERLOCKED_OP_RELEASE
#undef INTERLOCKED_OP_SEQ_CST
#endif
