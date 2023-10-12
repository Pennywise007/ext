#pragma once

#include <memory>
#include <type_traits>

namespace ext {

#if defined(_WIN32) || defined(__CYGWIN__) // windows
/*
 * Copy of std::enable_shared_from_this with checking of inheritance in compile time
 * std version do it in runtime only
 */
template<typename ThisClass, typename SharedClass = ThisClass>
struct enable_shared_from_this : std::enable_shared_from_this<SharedClass>
{
protected:
    constexpr enable_shared_from_this() noexcept
    {
        // @see shared_ptr constructor: _Set_ptr_rep_and_enable_shared and _Enable_shared_from_this1
        // check if enable_shared_from_this is inherited as public
        static_assert(std::bool_constant<std::conjunction_v<
                          std::negation<std::is_array<SharedClass>>,
                          std::negation<std::is_volatile<ThisClass>>,
                          std::_Can_enable_shared<ThisClass>>>{}, "Unambigous and accessible inheritance detected");
    }

    enable_shared_from_this(const enable_shared_from_this& other) noexcept
        : std::enable_shared_from_this(static_cast<const std::enable_shared_from_this&>(other))
    {}
};
#else

template<typename _Tp, typename SharedClass = _Tp>
using enable_shared_from_this = std::enable_shared_from_this<SharedClass>;

#endif

} // namespace ext