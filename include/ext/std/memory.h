#pragma once

#include <functional>
#include <memory>
#include <type_traits>

#include <ext/utils/lazy_type.h>

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

/*
* Allows to execute object creation only on first call, example

   struct Test
   {
       Test(bool,int) {}
       void Init() {}
   };
   ext::lazy_shared_ptr<Test> lazyPointer([]() { return std::make_shared<Test>(100, 500); });
//* object not created yet

   lazyPointer->Init();
//* object created and function executed
*/
template <typename Type>
struct lazy_shared_ptr : ext::lazy_type<std::shared_ptr<Type>>
{
    lazy_shared_ptr(std::function<std::shared_ptr<Type>()>&& getterFunction)
        : ext::lazy_type<std::shared_ptr<Type>>(std::move(getterFunction))
    {}

    EXT_NODISCARD const Type* get() const EXT_THROWS(...)
    {
        return ext::lazy_type<std::shared_ptr<Type>>::value().get();
    }

    EXT_NODISCARD Type* get() EXT_THROWS(...)
    {
        return ext::lazy_type<std::shared_ptr<Type>>::value().get();
    }

    EXT_NODISCARD Type* operator->() EXT_THROWS(...)
    {
        return get();
    }

    EXT_NODISCARD const Type* operator->() const EXT_THROWS(...)
    {
        return get();
    }
};

template <typename Type>
struct lazy_weak_ptr : ext::lazy_type<std::weak_ptr<Type>>
{
    lazy_weak_ptr(std::function<std::weak_ptr<Type>()>&& getterFunction)
        : ext::lazy_type<std::weak_ptr<Type>>(std::move(getterFunction))
    {}

    EXT_NODISCARD const Type* get() const EXT_THROWS(...)
    {
        return ext::lazy_type<std::weak_ptr<Type>>::value().lock().get();
    }

    EXT_NODISCARD Type* get() EXT_THROWS(...)
    {
        return ext::lazy_type<std::weak_ptr<Type>>::value().lock().get();
    }

    EXT_NODISCARD Type* operator->() EXT_THROWS(...)
    {
        return get();
    }

    EXT_NODISCARD const Type* operator->() const EXT_THROWS(...)
    {
        return get();
    }
};

} // namespace ext