#pragma once

#if __cplusplus >= 202002L

#include <source_location>

#else // not c++20

#include "stdint.h"

namespace std {

#if defined(__GNUC__)

// copy from <source_location>
struct source_location
{
private:
    using uint_least32_t = __UINT_LEAST32_TYPE__;
    struct __impl
    {
      const char* _M_file_name;
      const char* _M_function_name;
      unsigned _M_line;
      unsigned _M_column;
    };
    using __builtin_ret_type = decltype(__builtin_source_location());

public:

    // [support.srcloc.cons], creation
    static constexpr source_location
    current(__builtin_ret_type __p = __builtin_source_location()) noexcept
    {
      source_location __ret;
      __ret._M_impl = static_cast <const __impl*>(__p);
      return __ret;
    }

    constexpr source_location() noexcept { }

    // [support.srcloc.obs], observers
    constexpr uint_least32_t
    line() const noexcept
    { return _M_impl ? _M_impl->_M_line : 0u; }

    constexpr uint_least32_t
    column() const noexcept
    { return _M_impl ? _M_impl->_M_column : 0u; }

    constexpr const char*
    file_name() const noexcept
    { return _M_impl ? _M_impl->_M_file_name : ""; }

    constexpr const char*
    function_name() const noexcept
    { return _M_impl ? _M_impl->_M_function_name : ""; }

private:
    const __impl* _M_impl = nullptr;
};

#else // __GNUC__

// copy from <source_location>
struct source_location {
    [[nodiscard]] static constexpr source_location current(const uint_least32_t _Line_ = __builtin_LINE(),
        const uint_least32_t _Column_ = __builtin_COLUMN(), const char* const _File_ = __builtin_FILE(),
#if _USE_DETAILED_FUNCTION_NAME_IN_SOURCE_LOCATION
        const char* const _Function_ = __builtin_FUNCSIG()
#else // ^^^ detailed / basic vvv
        const char* const _Function_ = __builtin_FUNCTION()
#endif // ^^^ basic ^^^
            ) noexcept {
        source_location _Result{};
        _Result._Line     = _Line_;
        _Result._Column   = _Column_;
        _Result._File     = _File_;
        _Result._Function = _Function_;
        return _Result;
    }

    [[nodiscard]] constexpr source_location() noexcept = default;

    [[nodiscard]] constexpr uint_least32_t line() const noexcept {
        return _Line;
    }
    [[nodiscard]] constexpr uint_least32_t column() const noexcept {
        return _Column;
    }
    [[nodiscard]] constexpr const char* file_name() const noexcept {
        return _File;
    }
    [[nodiscard]] constexpr const char* function_name() const noexcept {
        return _Function;
    }

private:
    uint_least32_t _Line{};
    uint_least32_t _Column{};
    const char* _File     = "";
    const char* _Function = "";
};

#endif // __GNUC__

} // namespace std

#endif // not c++20
