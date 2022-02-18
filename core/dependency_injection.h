#pragma once

#include <any>
#include <guiddef.h>
#include <map>
#include <memory>
#include <mutex>
#include <type_traits>

#include <ext/core/check.h>
#include <ext/core/singleton.h>
#include <ext/core/defines.h>

#include <ext/detail/constructor_traits.h>

namespace ext {

struct ServiceProvider;
typedef std::shared_ptr<const ServiceProvider> ServiceProviderPtr;

struct ServiceCollection;

template <typename Type>
EXT_NODISCARD std::shared_ptr<Type> Create(ServiceProviderPtr serviceProvider);

namespace dependency_injection {

#undef GetObject

struct IObject
{
    virtual ~IObject() = default;

    virtual std::any GetObject(const ServiceProviderPtr&) = 0;
};

template <typename Interface, typename Object>
struct SingletonObjectGetter final : IObject, ext::NonCopyable
{
    std::any GetObject(const ServiceProviderPtr& serviceProvider) override
    {
        if (!m_object)
            m_object = ext::Create<Object>(serviceProvider);
        return m_object;
    }

private:
    std::shared_ptr<Interface> m_object;
};

template <typename Interface, typename Object>
struct TransientObjectGetter final : IObject
{
    std::any GetObject(const ServiceProviderPtr& serviceProvider) override
    {
        return std::make_any<std::shared_ptr<Interface>>(ext::Create<Object>(serviceProvider));
    }
};

typedef size_t InterfaceId;
typedef std::multimap<InterfaceId, std::shared_ptr<IObject>> InterfaceMap;

} // namespace dependency_injection

namespace di = dependency_injection;

struct ServiceProvider : std::enable_shared_from_this<ServiceProvider>, ext::NonCopyable
{
    template <typename Interface>
    EXT_NODISCARD std::shared_ptr<Interface> GetService() const
    {
        if (auto it = m_objects.find(typeid(Interface).hash_code()); it != m_objects.end())
        {
            try
            {
                EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "getting " << typeid(Interface).name() << " interface";
                const auto serviceProvider = shared_from_this();
                const std::any object = it->second->GetObject(serviceProvider);
                try
                {
                    return std::any_cast<std::shared_ptr<Interface>>(object);
                }
                catch (const std::bad_any_cast&)
                {
                    EXT_ASSERT(false);
                    EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "failed to get " << typeid(Interface).name()
                        << ", internal error, can`t get it from " << object.type().name();
                }
            }
            catch (...)
            {
                EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "failed to get " << typeid(Interface).name() << " interface, failed to create object";
                throw;
            }
        }
        EXT_TRACE() << EXT_TRACE_FUNCTION << "failed to get " << typeid(Interface).name() << " interface, not registered";
        throw std::exception((std::string("Failed to get interface ") + typeid(Interface).name() + ", interface, not registered").c_str());
    }

    template <typename Interface>
    EXT_NODISCARD std::shared_ptr<Interface> TryGetService() const EXT_NOEXCEPT
    try
    {
        return GetService<Interface>();
    }
    catch (...)
    {
        return nullptr;
    }

    template <typename Interface>
    EXT_NODISCARD std::list<std::shared_ptr<Interface>> GetServices() const EXT_NOEXCEPT
    {
        auto objectCreators = m_objects.equal_range(typeid(Interface).hash_code());
        if (objectCreators.first == objectCreators.second)
            return {};

        std::list<std::shared_ptr<Interface>> result;
        const auto serviceProvider = shared_from_this();
        for (auto it = objectCreators.first; it != objectCreators.second; ++ it)
        {
            try
            {
                const std::any object = it->second->GetObject(serviceProvider);
                try
                {
                    result.emplace_back(std::any_cast<std::shared_ptr<Interface>>(object));
                }
                catch (const std::bad_any_cast&)
                {
                    EXT_ASSERT(false);
                    EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "failed to get " << typeid(Interface).name()
                        << ", internal error, can`t get it from " << object.type().name();
                }
            }
            catch (...)
            {
                EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "failed to get " << typeid(Interface).name() << " interface, failed to create object";
            }
        }

        return result;
    }

private:
    friend struct ServiceCollection;
    explicit ServiceProvider(di::InterfaceMap&& objectsMap) EXT_NOEXCEPT : m_objects(std::move(objectsMap)) {}

    const di::InterfaceMap m_objects;
};

struct ServiceCollection : ext::NonCopyable
{
    template <typename Interface, typename Class>
    void AddTransient()
    {
        static_assert(std::is_base_of_v<Interface, Class>, "Inheritance error");
        std::shared_ptr<di::IObject> object = std::make_shared<di::TransientObjectGetter<Interface, Class>>();

        std::lock_guard lock(m_objectsMutex);
        m_objects.emplace(typeid(Interface).hash_code(), std::move(object));
    }

    template <typename Interface, typename Class>
    void AddScoped()
    {
        static_assert(std::is_base_of_v<Interface, Class>, "Inheritance error");
        std::shared_ptr<di::IObject> object = std::make_shared<di::TransientObjectGetter<Interface, Class>>();

        std::lock_guard lock(m_objectsMutex);
        m_scopedObjects.emplace(typeid(Interface).hash_code(), std::move(object));
    }

    template <typename Interface, typename Class>
    void AddSingleton()
    {
        static_assert(std::is_base_of_v<Interface, Class>, "Inheritance error");
        std::shared_ptr<di::IObject> object = std::make_shared<di::SingletonObjectGetter<Interface, Class>>();

        std::lock_guard lock(m_objectsMutex);
        m_objects.emplace(typeid(Interface).hash_code(), std::move(object));
    }

    ServiceProviderPtr BuildServiceProvider()
    {
        di::InterfaceMap commonMap;
        {
            std::lock_guard lock(m_objectsMutex);
            commonMap = m_objects;
            commonMap.insert(m_scopedObjects.begin(), m_scopedObjects.end());
            m_scopedObjects.clear();
        }
        return std::shared_ptr<const ServiceProvider>(new const ServiceProvider(std::move(commonMap)));
    }

private:
    std::mutex m_objectsMutex;
    di::InterfaceMap m_objects;
    di::InterfaceMap m_scopedObjects;
};

namespace dependency_injection {

struct any_interface_provider
{
    explicit any_interface_provider(ServiceProviderPtr&& serviceProvider)
        : m_serviceProvider(std::move(serviceProvider))
    {
        EXT_REQUIRE(!!m_serviceProvider);
    }

    template <typename Interface>
    operator std::shared_ptr<Interface>() const
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return m_serviceProvider->GetService<Interface>();
    }

    operator const ServiceProviderPtr&() const EXT_NOEXCEPT
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return m_serviceProvider;
    }

private:
    const ServiceProviderPtr m_serviceProvider;
};

template <typename Type, size_t... I>
EXT_NODISCARD std::shared_ptr<Type> CreateWithProviders(any_interface_provider& provider, std::index_sequence<I...>)
{
    return std::make_shared<Type>(((void)I, provider)...);
}
} // namespace dependency_injection

template <typename Type>
EXT_NODISCARD std::shared_ptr<Type> Create(ServiceProviderPtr serviceProvider)
{
    di::any_interface_provider provider(std::move(serviceProvider));
    return di::CreateWithProviders<Type>(provider, std::make_index_sequence<ext::detail::constructor_size<Type>>{});
}

} // namespace ext