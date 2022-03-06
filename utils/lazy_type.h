#pragma once

#include <optional>

#include <ext/core/check.h>
#include <ext/core/defines.h>

namespace ext {

template <typename Type, typename GetObjectFunction = std::function<Type()>>
struct lazy_type
{
    lazy_type(GetObjectFunction&& getterFunction) EXT_NOEXCEPT
        : m_getterFunction(std::move(getterFunction))
    {
        EXT_REQUIRE(!!m_getterFunction);
    }
    lazy_type(lazy_type<Type, GetObjectFunction>&& other) EXT_NOEXCEPT
        : m_object(std::move(other.m_object))
        , m_getterFunction(std::move(other.m_getterFunction))
    {}
    lazy_type(const lazy_type<Type, GetObjectFunction>& other)
        : m_object(other.m_object)
        , m_getterFunction(other.m_getterFunction)
    {}
    virtual ~lazy_type() = default;

    EXT_NODISCARD const Type& value() const EXT_THROWS(...)
    {
        if (!m_object.has_value())
        {
            EXT_EXPECT(!!m_getterFunction);
            m_object.emplace(m_getterFunction());
            m_getterFunction = nullptr;
        }
        return m_object.value();
    }

    EXT_NODISCARD Type& value() EXT_THROWS(...)
    {
        if (!m_object.has_value())
        {
            EXT_EXPECT(!!m_getterFunction);
            m_object.emplace(m_getterFunction());
            m_getterFunction = nullptr;
        }
        return m_object.value();
    }

    EXT_NODISCARD operator const Type&() const EXT_THROWS(...)
    {
        return value();
    }

    EXT_NODISCARD operator Type&() EXT_THROWS(...)
    {
        return value();
    }

    lazy_type<Type, GetObjectFunction>& operator=(const Type& other) EXT_NOEXCEPT
    {
        m_object = other;
        return *this;
    }

    lazy_type<Type, GetObjectFunction>& operator=(Type&& other) EXT_NOEXCEPT
    {
        m_object = std::move(other);
        return *this;
    }

    lazy_type<Type, GetObjectFunction>& operator=(lazy_type<Type, GetObjectFunction>&& other) EXT_NOEXCEPT
    {
        m_object = std::move(other.m_object);
        m_getterFunction = std::move(other.m_getterFunction);
        return *this;
    }
    lazy_type<Type, GetObjectFunction>& operator=(const lazy_type<Type, GetObjectFunction>& other) EXT_NOEXCEPT
    {
        m_object = other.m_object;
        m_getterFunction = other.m_getterFunction;
        return *this;
    }

protected:
    mutable std::optional<Type> m_object;
    GetObjectFunction m_getterFunction;
};

} // namespace ext