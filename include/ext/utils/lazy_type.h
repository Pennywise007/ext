#pragma once

#include <optional>

#include <ext/core/check.h>
#include <ext/core/defines.h>

namespace ext {

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

} // namespace ext