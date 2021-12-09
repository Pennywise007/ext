#pragma once

#include <functional>
#include <sal.h>
#include <optional>
#include <type_traits>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>

/* Setup execution code on exit scope

Using:
* SSH_SCOPE_ON_EXIT({ foundedPos = currentPos; });
* SSH_SCOPE_ON_EXIT_F((&foundedPos, currentPos),
    {
        foundedPos = currentPos;
    });
*/
#define SSH_SCOPE_ON_EXIT(code) ::ext::scope::ExitScope SSH_PP_CAT(__on_scoped_exit, __COUNTER__)([&]() code);
#define SSH_SCOPE_ON_EXIT_F(capture, code) ::ext::scope::ExitScope SSH_PP_CAT(__on_scoped_exit, __COUNTER__)([REMOVE_PARENTHES(capture)]() code);

// Declare cleaner for object, it will be called after exit scope
#define SSH_SCOPE_ON_EXIT_FREE(object, function) \
    ::ext::scope::FreeObject SSH_PP_CAT(__on_scoped_exit_free, __COUNTER__)(object, function);

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
    ObjectHolder(_In_ FreeFunction&& freeObjectFunction,
                 _In_opt_ std::optional<ObjectType>&& objectInitialValue,
                 _In_opt_ std::optional<ObjectType>&& objectInvalidValue) SSH_NOEXCEPT
        : m_freeObjectFunction(std::move(freeObjectFunction))
        , m_object(std::move(objectInitialValue))
        , m_objectInvalidValue(std::move(objectInvalidValue))
    {}
    /// <summary>Declare object with free object function without initialization</summary>
    /// <param name="freeObjectFunction">Destroy object function</param>
    /// <param name="objectInvalidValue">If set, we will only destroy the object if it does not match this value</param>
    ObjectHolder(_In_ FreeFunction&& freeObjectFunction,
                 _In_opt_ std::optional<ObjectType>&& objectInvalidValue = std::nullopt) SSH_NOEXCEPT
        : m_freeObjectFunction(std::move(freeObjectFunction))
        , m_objectInvalidValue(std::move(objectInvalidValue))
    {}
    // Move constuctor
    ObjectHolder(_In_ ObjectHolder&& otherObjectHolder) SSH_NOEXCEPT
        : m_freeObjectFunction(std::move(otherObjectHolder.m_freeObjectFunction))
        , m_object(std::move(otherObjectHolder.m_object))
        , m_objectInvalidValue(std::move(otherObjectHolder.m_objectInvalidValue))
    {
        otherObjectHolder.m_freeObjectFunction = nullptr;
        otherObjectHolder.m_object.reset();
    }

    ~ObjectHolder() { DestroyObject(); }

    constexpr ObjectHolder& operator=(ObjectType&& object) SSH_NOEXCEPT
    {
        DestroyObject();
        m_object = std::move(object);
        return *this;
    }

    constexpr ObjectHolder& operator=(const ObjectType& object) SSH_NOEXCEPT
    {
        DestroyObject();
        m_object = object;
        return *this;
    }

    constexpr SSH_NODISCARD operator ObjectType&() { return m_object.value(); }
    constexpr SSH_NODISCARD operator const ObjectType&() const { return m_object.value(); }

    constexpr SSH_NODISCARD const ObjectType& value() const { return m_object.value(); }
    constexpr SSH_NODISCARD bool has_value() const SSH_NOEXCEPT
    {
        return m_object.has_value() && (!m_objectInvalidValue.has_value() || m_object.value() != m_objectInvalidValue.value());
    }
    constexpr SSH_NODISCARD bool operator!() const SSH_NOEXCEPT { return !has_value(); }
    constexpr SSH_NODISCARD operator bool() const SSH_NOEXCEPT { return has_value(); }

public:
    constexpr void DestroyObject() SSH_NOEXCEPT
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
