#pragma once

#include <functional>
#include <optional>
#include <type_traits>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>

/* Setup execution code on exit scope

Using:
* EXT_SCOPE_ON_EXIT({ foundedPos = currentPos; });
* EXT_SCOPE_ON_EXIT_F((&foundedPos, currentPos),
    {
        foundedPos = currentPos;
    });
*/
#define EXT_SCOPE_ON_EXIT(code) ::ext::scope::ExitScope EXT_PP_CAT(__on_scoped_exit, __COUNTER__)([&]() code);
#define EXT_SCOPE_ON_EXIT_F(capture, code) ::ext::scope::ExitScope EXT_PP_CAT(__on_scoped_exit, __COUNTER__)([REMOVE_PARENTHESES(capture)]() code);

// Declare cleaner for object, it will be called after exit scope
#define EXT_SCOPE_ON_EXIT_FREE(object, function) \
    ::ext::scope::FreeObject EXT_PP_CAT(__on_scoped_exit_free, __COUNTER__)(object, function);

namespace ext::scope {

/* Realization of on exit scope functions executions */
struct ExitScope : ::ext::NonCopyable
{
    explicit ExitScope(std::function<void(void)>&& callback) : m_callback(std::move(callback)) {}
    ~ExitScope() { m_callback(); }

private:
    std::function<void(void)> m_callback;
};

/* Realization of object cleaner on exit scope */
template<typename ObjectType, typename FreeObjectFunction = void(*)(ObjectType&)>
struct FreeObject : ::ext::NonCopyable
{
    /// <summary>Declare object cleaner function</summary>
    /// <param name="object">object reference</param>
    /// <param name="freeObjectFunction">free function, will be executed in destructor</param>
    FreeObject(ObjectType& object, FreeObjectFunction&& freeObjectFunction)
        : m_object(object), m_freeObjectFunction(std::move(freeObjectFunction))
    {}
    ~FreeObject() { m_freeObjectFunction(m_object); }

private:
    ObjectType& m_object;
    FreeObjectFunction m_freeObjectFunction;
};

/*
* Realization of object holder with cleaner function
*
* Usage:
* ext::scope::ObjectHolder<HANDLE> hFile(&CloseHandle, INVALID_HANDLE_VALUE);
* ext::scope::ObjectHolder<HANDLE, decltype(&::CloseHandle)> handle = { &::CloseHandle, INVALID_HANDLE_VALUE };
*
* hFile(fileHandler, &CloseHandle, INVALID_HANDLE_VALUE)
* hFile(INVALID_HANDLE_VALUE, [](HANDLE& file)
        {
            if (file != INVALID_HANDLE_VALUE)
               CloseHandle(file);
        })
*/
template<typename ObjectType, typename FreeFunction = void(*)(ObjectType&)>
struct ObjectHolder : ::ext::NonCopyable
{
    /// <summary>Declare object with free object function </summary>
    /// <param name="freeObjectFunction">Destroy object function</param>
    /// <param name="objectInitialValue">Object initial value</param>
    /// <param name="objectInvalidValue">If set, we will only destroy the object if it does not match this value</param>
    ObjectHolder(FreeFunction&& freeObjectFunction,
                 std::optional<ObjectType>&& objectInitialValue,
                 std::optional<ObjectType>&& objectInvalidValue) noexcept
        : m_freeObjectFunction(std::move(freeObjectFunction))
        , m_object(std::move(objectInitialValue))
        , m_objectInvalidValue(std::move(objectInvalidValue))
    {}
    /// <summary>Declare object with free object function without initialization</summary>
    /// <param name="freeObjectFunction">Destroy object function</param>
    /// <param name="objectInvalidValue">If set, we will only destroy the object if it does not match this value</param>
    ObjectHolder(FreeFunction&& freeObjectFunction,
                 std::optional<ObjectType>&& objectInvalidValue = std::nullopt) noexcept
        : m_freeObjectFunction(std::move(freeObjectFunction))
        , m_objectInvalidValue(std::move(objectInvalidValue))
    {}
    // Move constructor
    ObjectHolder(ObjectHolder&& otherObjectHolder) noexcept
        : m_freeObjectFunction(std::move(otherObjectHolder.m_freeObjectFunction))
        , m_object(std::move(otherObjectHolder.m_object))
        , m_objectInvalidValue(std::move(otherObjectHolder.m_objectInvalidValue))
    {
        otherObjectHolder.m_freeObjectFunction = nullptr;
        otherObjectHolder.m_object.reset();
    }

    ~ObjectHolder() { DestroyObject(); }

    constexpr ObjectHolder& operator=(ObjectType&& object) noexcept
    {
        DestroyObject();
        m_object = std::move(object);
        return *this;
    }

    constexpr ObjectHolder& operator=(const ObjectType& object) noexcept
    {
        DestroyObject();
        m_object = object;
        return *this;
    }

    [[nodiscard]] constexpr operator ObjectType&() { return m_object.value(); }
    [[nodiscard]] constexpr operator const ObjectType&() const { return m_object.value(); }

    [[nodiscard]] constexpr const ObjectType& value() const { return m_object.value(); }
    [[nodiscard]] constexpr bool has_value() const noexcept
    {
        return m_object.has_value() && (!m_objectInvalidValue.has_value() || m_object.value() != m_objectInvalidValue.value());
    }

    [[nodiscard]] constexpr bool operator!() const noexcept { return !has_value(); }
    [[nodiscard]] constexpr operator bool() const noexcept { return has_value(); }

public:
    constexpr void DestroyObject() noexcept
    {
        if (m_freeObjectFunction != nullptr && has_value())
            m_freeObjectFunction(m_object.value());

        m_object.reset();
    }

private:
    FreeFunction m_freeObjectFunction;
    std::optional<ObjectType> m_object;
    std::optional<ObjectType> m_objectInvalidValue;
};

} // namespace ext::scope
