#pragma once

#include <any>
#include <exception>
#include <map>
#include <list>
#include <memory>
#include <shared_mutex>
#include <string>
#include <type_traits>

#include <ext/core/check.h>
#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>
#include <ext/core/singleton.h>

#include <ext/detail/constructor_traits.h>

#include <ext/utils/memory.h>
#include <ext/utils/lazy_type.h>

#undef GetObject

namespace ext {
namespace dependency_injection {
// exception from DI container, occurs when the container cannot find the class/interface(not registered)
struct not_registered : std::exception
{
    explicit not_registered(const std::string& error) EXT_NOEXCEPT : std::exception(error.c_str()) {}
};

} // namespace dependency_injection;

namespace di = dependency_injection;

struct ServiceProvider : ext::enable_shared_from_this<const ServiceProvider>, ext::NonCopyable
{
    typedef std::shared_ptr<const ServiceProvider> Ptr;

    template <typename Interface>
    EXT_NODISCARD std::shared_ptr<Interface> GetInterface() const EXT_THROWS(di::not_registered, ...);

    template <typename Interface>
    EXT_NODISCARD std::shared_ptr<Interface> TryGetInterface() const EXT_NOEXCEPT;

    template <typename Interface>
    EXT_NODISCARD std::list<std::shared_ptr<Interface>> GetInterfaces() const EXT_THROWS(di::not_registered, ...);

    template <typename Interface>
    EXT_NODISCARD bool IsRegistered() const EXT_NOEXCEPT;

    EXT_NODISCARD Ptr CreateScope() const EXT_NOEXCEPT;

private:
    friend struct ServiceCollection;

    struct IObject;

    typedef size_t InterfaceId;
    typedef std::map<InterfaceId, std::list<std::shared_ptr<IObject>>> InterfaceMap;

private:
    explicit ServiceProvider(InterfaceMap&& objectsMap) EXT_NOEXCEPT;

    const InterfaceMap m_registeredObjects;
};

struct ServiceCollection
{
    template <typename Interface, typename Class>
    void RegisterTransient();

    template <typename Interface, typename Class>
    void RegisterScoped();

    template <typename Interface, typename Class>
    void RegisterSingleton();

    template <typename Interface>
    EXT_NODISCARD bool IsRegistered() const EXT_NOEXCEPT;

    template <typename Interface>
    bool Unregister() EXT_NOEXCEPT;

    void Clear() EXT_NOEXCEPT;

    EXT_NODISCARD ServiceProvider::Ptr BuildServiceProvider() const;

private:
    friend ext::Singleton<ServiceCollection>;

    template <typename Interface, typename Object>
    struct SingletonObject;
    template <typename Interface, typename Object>
    struct TransientObject;
    template <typename Interface, typename Object>
    struct ScopedObject;

    mutable std::shared_mutex m_registeredObjectsMutex;
    ServiceProvider::InterfaceMap m_registeredObjects;
};

template <typename Object>
EXT_NODISCARD std::shared_ptr<Object> CreateObject(ServiceProvider::Ptr serviceProvider = nullptr) EXT_THROWS(di::not_registered, ...);

template <typename Interface>
EXT_NODISCARD std::shared_ptr<Interface> GetInterface(ServiceProvider::Ptr serviceProvider = nullptr) EXT_THROWS(di::not_registered, ...);

struct ServiceProviderHolder
{
    ServiceProviderHolder(const ServiceProvider::Ptr& serviceProvider)
        : m_serviceProvider(serviceProvider)
    {}

    template <typename Interface>
    std::shared_ptr<Interface> GetInterface()
    {
        return ext::GetInterface<Interface>(m_serviceProvider);
    }

    ServiceProvider::Ptr m_serviceProvider;
};

template <typename Interface>
struct lazy_interface : ext::lazy_shared_ptr<Interface>
{
    lazy_interface(const ServiceProvider::Ptr& serviceProvider)
        : ext::lazy_shared_ptr<Interface>([serviceProvider]()
          {
              return ext::GetInterface<Interface>(serviceProvider);
          })
    {}
};

template <typename Object>
struct lazy_object : ext::lazy_shared_ptr<Object>
{
    lazy_object(const ServiceProvider::Ptr& serviceProvider)
        :  ext::lazy_shared_ptr<Object>([serviceProvider]()
          {
              return ext::CreateObject<Object>(serviceProvider);
          })
    {}
};

namespace dependency_injection {

struct any_interface_provider
{
    explicit any_interface_provider(ServiceProvider::Ptr&& serviceProvider) EXT_NOEXCEPT
        : m_serviceProvider(std::move(serviceProvider))
    {}

    template <typename Interface>
    operator std::shared_ptr<Interface>() const
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return m_serviceProvider->GetInterface<Interface>();
    }

    template <typename Interface>
    operator lazy_interface<Interface>() const
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return lazy_interface<Interface>(m_serviceProvider);
    }

    template <typename Object>
    operator lazy_object<Object>() const
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return lazy_object<Object>(m_serviceProvider);
    }

    operator const ServiceProvider::Ptr&() const EXT_NOEXCEPT
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return m_serviceProvider;
    }

private:
    const ServiceProvider::Ptr m_serviceProvider;
};

template <typename Type, size_t... Index>
EXT_NODISCARD std::shared_ptr<Type> CreateWithProviders(any_interface_provider& provider, const std::index_sequence<Index...>&) EXT_THROWS(di::not_registered, ...)
{
    return std::make_shared<Type>(((void)Index, provider)...);
}

} // namespace dependency_injection

template <typename Type>
EXT_NODISCARD std::shared_ptr<Type> CreateObject(ServiceProvider::Ptr serviceProvider) EXT_THROWS(di::not_registered, ...)
{
    di::any_interface_provider provider(std::move(serviceProvider));
    return di::CreateWithProviders<Type>(provider, std::make_index_sequence<ext::detail::constructor_size<Type>>{});
}

template <typename Interface>
EXT_NODISCARD std::shared_ptr<Interface> GetInterface(ServiceProvider::Ptr serviceProvider) EXT_THROWS(di::not_registered, ...)
{
    return serviceProvider->GetInterface<Interface>();
}

struct ServiceProvider::IObject : ext::NonCopyable
{
    explicit IObject(const char* name) EXT_NOEXCEPT : m_objectName(name) {}
    virtual ~IObject() = default;

    EXT_NODISCARD virtual const char* GetName() const EXT_NOEXCEPT { return m_objectName; }
    EXT_NODISCARD virtual std::any GetObject(const ServiceProvider::Ptr&) = 0;
    EXT_NODISCARD virtual std::shared_ptr<IObject> CopyObject() EXT_NOEXCEPT = 0;

private:
    const char* m_objectName;
};

template <typename Interface>
EXT_NODISCARD std::shared_ptr<Interface> ServiceProvider::GetInterface() const EXT_THROWS(di::not_registered, ...)
{
    if (auto it = m_registeredObjects.find(typeid(Interface).hash_code()); it != m_registeredObjects.end() && !it->second.empty())
    {
        try
        {
            EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "getting " << typeid(Interface).name() << " interface";
            const std::any object = it->second.back()->GetObject(shared_from_this());
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
            EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "failed to get " << typeid(Interface).name() <<
                " interface, failed to create object " << it->second.back()->GetName();
            throw;
        }
    }
    std::string error = std::string("failed to get ") + typeid(Interface).name() + " interface, not registered";
    EXT_TRACE() << EXT_TRACE_FUNCTION << error;
    throw di::not_registered(error);
}

template <typename Interface>
EXT_NODISCARD std::shared_ptr<Interface> ServiceProvider::TryGetInterface() const EXT_NOEXCEPT
try
{
    return GetInterface<Interface>();
}
catch (...)
{
    return nullptr;
}

template <typename Interface>
EXT_NODISCARD std::list<std::shared_ptr<Interface>> ServiceProvider::GetInterfaces() const EXT_THROWS(di::not_registered, ...)
{
    if (auto it = m_registeredObjects.find(typeid(Interface).hash_code()); it != m_registeredObjects.end() && !it->second.empty())
    {
        std::list<std::shared_ptr<Interface>> result;
        const auto pointer = shared_from_this();
        for (auto& objectProvider : it->second)
        {
            try
            {
                EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "getting " << typeid(Interface).name() << " interface";
                const std::any object = objectProvider->GetObject(pointer);
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
                EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "failed to get " << typeid(Interface).name() <<
                    " interface, failed to create object " << objectProvider->GetName();
                throw;
            }
        }
        return result;
    }

    std::string error = std::string("failed to get realiztions for ") + typeid(Interface).name() + " interface, not registered";
    EXT_TRACE() << EXT_TRACE_FUNCTION << error;
    throw di::not_registered(error);
}

template <typename Interface>
EXT_NODISCARD bool ServiceProvider::IsRegistered() const EXT_NOEXCEPT
{
    return (m_registeredObjects.find(typeid(Interface).hash_code()) != m_registeredObjects.end());
}

inline ServiceProvider::Ptr ServiceProvider::CreateScope() const EXT_NOEXCEPT
{
    InterfaceMap registeredObjects;
    for (auto&& [interfaceId, objects] : m_registeredObjects)
    {
        auto& interfaceScopeList = registeredObjects[interfaceId];
        std::transform(objects.cbegin(), objects.cend(), std::back_inserter(interfaceScopeList),
                       [](const std::shared_ptr<IObject>& object)
                       {
                           return object->CopyObject();
                       });
    }
    return ServiceProvider::Ptr(new const ServiceProvider(std::move(registeredObjects)));
}

inline ServiceProvider::ServiceProvider(InterfaceMap&& objectsMap) EXT_NOEXCEPT
    : m_registeredObjects(std::move(objectsMap))
{}

template <typename Interface, typename Object>
struct ServiceCollection::SingletonObject : ServiceProvider::IObject,
    ext::enable_shared_from_this<ServiceCollection::SingletonObject<Interface, Object>, ServiceProvider::IObject>
{
    explicit SingletonObject() EXT_NOEXCEPT : IObject(typeid(Object).name()) {}
    EXT_NODISCARD std::any GetObject(const ServiceProvider::Ptr& serviceProvider) override
    {
        if (!m_object)
            m_object = ext::CreateObject<Object>(serviceProvider);
        return m_object;
    }

    virtual EXT_NODISCARD std::shared_ptr<ServiceProvider::IObject> CopyObject() EXT_NOEXCEPT override
    {
        return shared_from_this();
    }

private:
    std::shared_ptr<Interface> m_object;
};

template <typename Interface, typename Object>
struct ServiceCollection::TransientObject final : ServiceProvider::IObject
{
    explicit TransientObject() EXT_NOEXCEPT : IObject(typeid(Object).name()) {}

    EXT_NODISCARD std::any GetObject(const ServiceProvider::Ptr& serviceProvider) override
    {
        return std::make_any<std::shared_ptr<Interface>>(ext::CreateObject<Object>(serviceProvider));
    }

    EXT_NODISCARD std::shared_ptr<ServiceProvider::IObject> CopyObject() EXT_NOEXCEPT override
    {
        return std::make_unique<TransientObject<Interface, Object>>();
    }
};

template <typename Interface, typename Object>
struct ServiceCollection::ScopedObject final: ServiceCollection::SingletonObject<Interface, Object>
{
    EXT_NODISCARD std::shared_ptr<ServiceProvider::IObject> CopyObject() EXT_NOEXCEPT override
    {
        return std::make_shared<ScopedObject<Interface, Object>>();
    }

private:
    std::shared_ptr<Interface> m_object;
};

template <typename Interface, typename Class>
void ServiceCollection::RegisterTransient()
{
    static_assert(std::is_base_of_v<Interface, Class>, "Inheritance error");
    std::shared_ptr<ServiceProvider::IObject> object = std::make_shared<TransientObject<Interface, Class>>();

    std::unique_lock lock(m_registeredObjectsMutex);
    m_registeredObjects[typeid(Interface).hash_code()].emplace_back(std::move(object));
}

template <typename Interface, typename Class>
void ServiceCollection::RegisterScoped()
{
    static_assert(std::is_base_of_v<Interface, Class>, "Inheritance error");
    std::shared_ptr<ServiceProvider::IObject> object = std::make_shared<ScopedObject<Interface, Class>>();

    std::unique_lock lock(m_registeredObjectsMutex);
    m_registeredObjects[typeid(Interface).hash_code()].emplace_back(std::move(object));
}

template <typename Interface, typename Class>
void ServiceCollection::RegisterSingleton()
{
    static_assert(std::is_base_of_v<Interface, Class>, "Inheritance error");
    std::shared_ptr<ServiceProvider::IObject> object = std::make_unique<SingletonObject<Interface, Class>>();

    std::unique_lock lock(m_registeredObjectsMutex);
    m_registeredObjects[typeid(Interface).hash_code()].emplace_back(std::move(object));
}

template <typename Interface>
bool ServiceCollection::IsRegistered() const EXT_NOEXCEPT
{
    std::shared_lock lock(m_registeredObjectsMutex);
    return m_registeredObjects.find(typeid(Interface).hash_code()) != m_registeredObjects.end();
}

template <typename Interface>
bool ServiceCollection::Unregister() EXT_NOEXCEPT
{
    std::unique_lock lock(m_registeredObjectsMutex);
    return m_registeredObjects.erase(typeid(Interface).hash_code()) != 0;
}

inline ServiceProvider::Ptr ServiceCollection::BuildServiceProvider() const
{
    ServiceProvider::InterfaceMap registeredObjects;
    {
        std::shared_lock lock(m_registeredObjectsMutex);
        for (auto&&[interfaceId, objects] : m_registeredObjects)
        {
            auto& interfaceScopeList = registeredObjects[interfaceId];
            std::transform(objects.cbegin(), objects.cend(), std::back_inserter(interfaceScopeList),
                           [](const std::shared_ptr<ServiceProvider::IObject>& object)
                           {
                               return object->CopyObject();
                           });
        }
    }
    return ServiceProvider::Ptr(new const ServiceProvider(std::move(registeredObjects)));
}

inline void ServiceCollection::Clear() EXT_NOEXCEPT
{
    std::unique_lock lock(m_registeredObjectsMutex);
    m_registeredObjects.clear();
}

} // namespace ext