#pragma once

#include <functional>
#include <optional>
#include <memory>

#include <ext/core/check.h>
#include <ext/core/defines.h>

namespace ext {

/*
    Allows to define an object with an object getter function. Allows to simplify resources management.
    The object will be created only of first object call and get function will be freed.

    struct Test
    {
        Test(bool,int) {}
        void Get() {}
    };
    ext::lazy_type<Test> lazyObject([]() { return Test(100, 500); });
    * Object not created

    * Object will be created by provided GetObjectFunction during value() call
    lazyObject.value().Get();
*/
template <typename Type, typename GetObjectFunction = std::function<Type()>>
struct lazy_type
{
    lazy_type(GetObjectFunction&& getterFunction) noexcept
        : m_getterFunction(std::move(getterFunction))
    {
        EXT_REQUIRE(!!m_getterFunction);
    }
    lazy_type(lazy_type<Type, GetObjectFunction>&& other) noexcept
        : m_object(std::move(other.m_object))
        , m_getterFunction(std::move(other.m_getterFunction))
    {}
    lazy_type(const lazy_type<Type, GetObjectFunction>& other)
        : m_object(other.m_object)
        , m_getterFunction(other.m_getterFunction)
    {}
    virtual ~lazy_type() = default;

    [[nodiscard]] const Type& value() const EXT_THROWS(...)
    {
        if (!m_object.has_value())
        {
            EXT_EXPECT(!!m_getterFunction);
            GetObjectFunction getFunction;
            std::swap(getFunction, m_getterFunction);
            m_object.emplace(getFunction());
        }
        return m_object.value();
    }

    [[nodiscard]] Type& value() EXT_THROWS(...)
    {
        if (!m_object.has_value())
        {
            EXT_EXPECT(!!m_getterFunction);
            GetObjectFunction getFunction;
            std::swap(getFunction, m_getterFunction);
            m_object.emplace(getFunction());
        }
        return m_object.value();
    }

    [[nodiscard]] operator const Type&() const EXT_THROWS(...)
    {
        return value();
    }

    [[nodiscard]] operator Type&() EXT_THROWS(...)
    {
        return value();
    }

    lazy_type<Type, GetObjectFunction>& operator=(const Type& other) noexcept
    {
        m_object = other;
        return *this;
    }

    lazy_type<Type, GetObjectFunction>& operator=(Type&& other) noexcept
    {
        m_object = std::move(other);
        return *this;
    }

    lazy_type<Type, GetObjectFunction>& operator=(lazy_type<Type, GetObjectFunction>&& other) noexcept
    {
        m_object = std::move(other.m_object);
        m_getterFunction = std::move(other.m_getterFunction);
        return *this;
    }
    lazy_type<Type, GetObjectFunction>& operator=(const lazy_type<Type, GetObjectFunction>& other) noexcept
    {
        m_object = other.m_object;
        m_getterFunction = other.m_getterFunction;
        return *this;
    }

protected:
    mutable std::optional<Type> m_object;
    mutable GetObjectFunction m_getterFunction;
};

/*
    * Allows to execute object creation only on first call, example

    struct Test
    {
        Test(bool,int) {}
        void Init() {}
    };
    ext::lazy_shared_ptr<Test> lazyPointer([]() { return std::make_shared<Test>(100, 500); });
    * Object not created here

    * Object will be created in operator-> and function executed
    lazyPointer->Init();
*/
template <typename Type>
struct lazy_shared_ptr : lazy_type<std::shared_ptr<Type>>
{
    lazy_shared_ptr(std::function<std::shared_ptr<Type>()>&& getterFunction)
        : lazy_type<std::shared_ptr<Type>>(std::move(getterFunction))
    {}

    [[nodiscard]] const Type* get() const EXT_THROWS(...)
    {
        return lazy_type<std::shared_ptr<Type>>::value().get();
    }

    [[nodiscard]] Type* get() EXT_THROWS(...)
    {
        return lazy_type<std::shared_ptr<Type>>::value().get();
    }

    [[nodiscard]] Type* operator->() EXT_THROWS(...)
    {
        return get();
    }

    [[nodiscard]] const Type* operator->() const EXT_THROWS(...)
    {
        return get();
    }
};

/*
    * Allows to execute object creation only on first call, example
    
    struct Test
    {
        void Init() {}
    };
    ext::lazy_weak_ptr<Test> lazyWeakPointer([]() -> std::weak_ptr { return ...; });
    * Object not created yet

    * Object created in operator-> and function executed
    lazyWeakPointer->Init();
*/
template <typename Type>
struct lazy_weak_ptr : lazy_type<std::weak_ptr<Type>>
{
    lazy_weak_ptr(std::function<std::weak_ptr<Type>()>&& getterFunction)
        : lazy_type<std::weak_ptr<Type>>(std::move(getterFunction))
    {}

    [[nodiscard]] const Type* get() const EXT_THROWS(...)
    {
        return lazy_type<std::weak_ptr<Type>>::value().lock().get();
    }

    [[nodiscard]] Type* get() EXT_THROWS(...)
    {
        return lazy_type<std::weak_ptr<Type>>::value().lock().get();
    }

    [[nodiscard]] Type* operator->() EXT_THROWS(...)
    {
        return get();
    }

    [[nodiscard]] const Type* operator->() const EXT_THROWS(...)
    {
        return get();
    }
};

} // namespace ext