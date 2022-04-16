#pragma once
/*
 * C++17 header only dependency injection realization
 * Usage simple with .Net Microsoft.Extensions.DependencyInjection
 *
 * Example:

struct IInterfaceExample
{
    virtual ~IInterfaceExample() = default;
};

struct InterfaceImplementationExample : IInterfaceExample
{};

struct CreatedObjectExample : ext::ServiceProviderHolder
{
    explicit CreatedObjectExample(std::shared_ptr<IInterfaceExample> interfaceShared, std::lazy_interface<IInterfaceExample> interfaceLazy, ext::ServiceProvider::Ptr&& serviceProvider)
        : ServiceProviderHolder(std::move(serviceProvider))
        , m_interfaceShared(std::move(interfaceShared))
        , m_interfaceLazyOne(std::move(interfaceLazy))
        , m_interfaceLazyTwo(ServiceProviderHolder::m_serviceProvider)
    {}

    std::shared_ptr<IRandomInterface> GetRandomInterface() const
    {
        return ServiceProviderHolder::GetInterface<IRandomInterface>();
    }

    std::shared_ptr<IRandomInterface> GetRandomInterfaceOption2() const
    {
        return ext::GetInterface<IRandomInterface>(ServiceProviderHolder::m_serviceProvider);
    }

    std::shared_ptr<IInterfaceExample> m_interfaceShared;
    ext::lazy_interface<IInterfaceExample> m_interfaceLazyOne;
    ext::lazy_interface<IInterfaceExample> m_interfaceLazyTwo;
};

ext::ServiceCollection& serviceCollection = ext::get_service<ext::ServiceCollection>();
serviceCollection.RegisterScoped<InterfaceImplementationExample, IInterfaceExample>();

const std::shared_ptr<CreatedObjectExample> object = ext::CreateObject<CreatedObjectExample>(serviceCollection.BuildServiceProvider());

 */

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
#include <ext/core/mpl.h>
#include <ext/core/singleton.h>

#include <ext/std/memory.h>
#include <ext/utils/constructor_traits.h>

#pragma push_macro("GetObject")
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

// Services provider for GetInterface and CreateInterface functions, can be created from ServiceCollection::BuildServiceProvider
struct ServiceProvider : ext::enable_shared_from_this<ServiceProvider>, ext::NonCopyable
{
    typedef std::shared_ptr<ServiceProvider> Ptr;

    // Getting interface from registered interface collection, returns last registered implementation of interface
    // Returns shared ptr on object if everything ok
    // If interface not registered in service provider throws di::not_registered exception
    // If interface implementation can`t be created because of construction exception - throws it
    template <typename Interface>
    EXT_NODISCARD std::shared_ptr<Interface> GetInterface() const EXT_THROWS(di::not_registered, ...);

    // Getting interface from registered interface collection, returns last registered implementation of interface
    // Returns shared ptr on object if everything ok
    // Returns nullptr if any error/exception occurred
    template <typename Interface>
    EXT_NODISCARD std::shared_ptr<Interface> TryGetInterface() const EXT_NOEXCEPT;

    // Getting all interface realizations from registered interface collection
    // Returns collection of shared ptr on object if everything ok
    // If interface not registered in service provider throws di::not_registered exception
    // If any interface implementation can`t be created because of construction exception - throws it
    template <typename Interface>
    EXT_NODISCARD std::list<std::shared_ptr<Interface>> GetInterfaces() const EXT_THROWS(di::not_registered, ...);

    // Checking if interface registered in provider
    template <typename Interface>
    EXT_NODISCARD bool IsRegistered() const EXT_NOEXCEPT;

    // Creating new provider scope, all objects registered as @ServiceCollection::RegisterScoped will be recreated
    EXT_NODISCARD Ptr CreateScope() EXT_NOEXCEPT;

    // Remove all created scoped\singleton objects from all created and exist ServiceProviders
    // Allows to destoy singletones and scoped objects if service provider is the last owner
    void Reset() EXT_NOEXCEPT;

private:
    // allow construction from ServiceCollection
    friend struct ServiceCollection;

    struct IObject;
    struct IObjectWrapper;

    // Interface to object fabrics map
    typedef size_t InterfaceId;
    typedef std::map<InterfaceId, std::list<std::shared_ptr<IObject>>> InterfaceMap;
    const InterfaceMap m_registeredObjects;

    std::mutex m_createdScopesMutex;
    std::list<std::weak_ptr<ServiceProvider>> m_createdScopes;

private:
    explicit ServiceProvider(InterfaceMap&& objectsMap) EXT_NOEXCEPT;

    // Common function for creating new service provider scope
    EXT_NODISCARD static Ptr CreateScope(const InterfaceMap& registeredObjects,
                                         std::mutex& createdScopesMutex,
                                         std::list<std::weak_ptr<ServiceProvider>>& createdScopes) EXT_NOEXCEPT;
    // update all object wrappers, in new scope
    static void UpdateObjectWrappersInNewScope(const ServiceProvider::InterfaceMap& parentScope,
                                               ServiceProvider::InterfaceMap& newScope) EXT_NOEXCEPT;
};

// Service collection singleton, allow to register interfaces and their implementations
// Can be obtained by ext::get_service<ext::ServiceCollection>() or ServiceCollection::Instance()
struct ServiceCollection
{
    // Registering a class and the interfaces it implements
    // Function ServiceProvider::GetInterface will create a new object every time it called
    template <typename Class, typename... Interfaces>
    void RegisterTransient();

    // Registering a class and the interfaces it implements
    // Function ServiceProvider::GetInterface will create object on first call and will return it for each next call inside each ServiceProvider
    template <typename Class, typename... Interfaces>
    void RegisterScoped();

    // Registering a class and the interfaces it implements
    // Function ServiceProvider::GetInterface will create object on first call and will return it for each next call for all ServiceProviders
    template <typename Class, typename... Interfaces>
    void RegisterSingleton();

    // Checking is interface registered inside collection
    template <typename Interface>
    EXT_NODISCARD bool IsRegistered() const EXT_NOEXCEPT;

    // Unregistering interface from collection
    template <typename Interface>
    bool Unregister() EXT_NOEXCEPT;

    // Unregistering object with interfaces implementation from collection
    template <typename Class, typename... Interfaces>
    void UnregisterObject() EXT_NOEXCEPT;
    
    // Remove all information
    void UnregisterAll() EXT_NOEXCEPT;

    // Creating new ServiceProvider with all registered objects information
    EXT_NODISCARD ServiceProvider::Ptr BuildServiceProvider();

    // Remove all created scoped\singleton objects from all created and exist ServiceProviders
    // Allows singletons and scoped objects to be destroyed if the service provider is the last owner
    void ResetObjects() EXT_NOEXCEPT;

private:
    // Internal registration class in InterfaceMap
    template <typename Interface>
    void RegisterObject(const std::function<std::shared_ptr<ServiceProvider::IObject>()>& registerObject);

    ~ServiceCollection() EXT_NOEXCEPT;

private:
    friend ext::Singleton<ServiceCollection>;

    template <typename Object, typename Interface>
    struct SingletonObject;
    template <typename Object, typename Interface>
    struct TransientObject;
    template <typename Object, typename Interface>
    struct ScopedObject;
    template <typename Object, typename Interface, typename InterfaceWrapped>
    struct WrapperObject;

    mutable std::shared_mutex m_registeredObjectsMutex;
    ServiceProvider::InterfaceMap m_registeredObjects;

    mutable std::mutex m_creaedServiceProvidersMutex;
    std::list<std::weak_ptr<ServiceProvider>> m_createdServiceProviders;
};

// Creating object, allow to construct object and provide it with all necessary interfaces
// If any required interface not registered inside ServiceProvider - throws di::not_registered
// If object can`t be created because of constructor exceptions - throws it
template <typename Object>
EXT_NODISCARD std::shared_ptr<Object> CreateObject(ServiceProvider::Ptr serviceProvider) EXT_THROWS(di::not_registered, ...);

// Getting interface implementation from ServiceProvider
// If any required interface not registered inside ServiceProvider - throws di::not_registered
// If object can`t be created because of constructor exceptions - throws it
template <typename Interface>
EXT_NODISCARD std::shared_ptr<Interface> GetInterface(const ServiceProvider::Ptr& serviceProvider) EXT_THROWS(di::not_registered, ...);

// Help interface to simplify getting interfaces inside classes, holds ServiceProvider pointer and allow to easy GetInterface
struct ServiceProviderHolder
{
    explicit ServiceProviderHolder(ServiceProvider::Ptr serviceProvider) EXT_NOEXCEPT
        : m_serviceProvider(std::move(serviceProvider))
    {}

    template <typename Interface>
    EXT_NODISCARD std::shared_ptr<Interface> GetInterface() const EXT_THROWS(di::not_registered, ...)
    {
        return ext::GetInterface<Interface>(m_serviceProvider);
    }

    template <typename Object>
    EXT_NODISCARD std::shared_ptr<Object> CreateObject() const EXT_THROWS(di::not_registered, ...)
    {
        return ext::CreateObject<Object>(m_serviceProvider);
    }

    const ServiceProvider::Ptr m_serviceProvider;
};

// Lazy weak interface pointer, holds std::weak_ptr on Interface
// Allows to delay interface getting and execute it only on first call: get() or operator->
// !Can throw exception on first call
template <typename Interface>
struct lazy_weak_interface : ext::lazy_weak_ptr<Interface>
{
    lazy_weak_interface(const ServiceProvider::Ptr& serviceProvider)
        : ext::lazy_weak_ptr<Interface>([serviceProvider]() -> std::weak_ptr<Interface>
          {
              return ext::GetInterface<Interface>(serviceProvider);
          })
    {}
};

// Lazy interface pointer, holds std::shared_ptr on Interface
// Allows to delay interface getting and execute it only on first call: get() or operator->
// !Can throw exception on first call
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

// Lazy object pointer, holds std::shared_ptr on Object
// Allows to create object on first call to object functions - get() or operator->
// !Can throw exception on first call
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

#pragma region Implementation

namespace dependency_injection {

// Inheritance checker helper, allow to output interface and class name in build log
template<typename Class, typename Interface>
struct check_inheritance
{
    constexpr static void Check() { static_assert(std::is_base_of_v<Interface, Class>, "Class must implements Interface"); }
};

// Allow to create any object and provide it with all necessary interfaces
struct any_interface_provider
{
    explicit any_interface_provider(ServiceProvider::Ptr&& serviceProvider) EXT_NOEXCEPT
        : m_serviceProvider(std::move(serviceProvider))
    {}

    operator ServiceProvider::Ptr() const EXT_NOEXCEPT
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return m_serviceProvider;
    }

    template <typename Interface>
    operator std::shared_ptr<Interface>() const
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return m_serviceProvider->GetInterface<Interface>();
    }

    template <typename Interface>
    operator std::weak_ptr<Interface>() const
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

    template <typename Interface>
    operator lazy_weak_interface<Interface>() const
    {
        EXT_REQUIRE(!!m_serviceProvider);
        return lazy_weak_interface<Interface>(m_serviceProvider);
    }

private:
    const ServiceProvider::Ptr m_serviceProvider;
};

// Creating shared pointer on any object and provide it with all necessary interfaces
template <typename Type, size_t... Index>
EXT_NODISCARD std::shared_ptr<Type> CreateWithProviders(any_interface_provider& provider, const std::index_sequence<Index...>&) EXT_THROWS(di::not_registered, ...)
{
    return std::make_shared<Type>(((void)Index, provider)...);
}

} // namespace dependency_injection

// Creating object, allow to construct object and provide it with all necessary interfaces
// If any required interface not registered inside ServiceProvider - throws di::not_registered
// If object can`t be created because of constructor exceptions - throws it
template <typename Type>
EXT_NODISCARD std::shared_ptr<Type> CreateObject(ServiceProvider::Ptr serviceProvider) EXT_THROWS(di::not_registered, ...)
{
    EXT_EXPECT(!!serviceProvider);
    di::any_interface_provider provider(std::move(serviceProvider));
    return di::CreateWithProviders<Type>(provider, std::make_index_sequence<ext::detail::constructor_size<Type>>{});
}

// Getting interface implementation from ServiceProvider
// If any required interface not registered inside ServiceProvider - throws di::not_registered
// If object can`t be created because of constructor exceptions - throws it
template <typename Interface>
EXT_NODISCARD std::shared_ptr<Interface> GetInterface(const ServiceProvider::Ptr& serviceProvider) EXT_THROWS(di::not_registered, ...)
{
    EXT_EXPECT(!!serviceProvider);
    return serviceProvider->GetInterface<Interface>();
}

struct ServiceProvider::IObject : ext::NonCopyable
{
    explicit IObject(const char* name) EXT_NOEXCEPT : m_objectName(name) {}
    virtual ~IObject() = default;

    // Get class hash
    EXT_NODISCARD virtual size_t GetHash() const EXT_NOEXCEPT = 0;
    // Get class name, mostly for debug
    EXT_NODISCARD virtual const char* GetName() const EXT_NOEXCEPT { return m_objectName; }
    // Get object, if object not exist - creating it
    EXT_NODISCARD virtual std::any GetObject(ServiceProvider::Ptr&&) = 0;
    // On call CreateScope or BuildServiceProvider - creating new IObject
    EXT_NODISCARD virtual std::shared_ptr<IObject> CreateScopedObject() EXT_NOEXCEPT = 0;
    // Reset object if exists
    virtual void Reset() EXT_NOEXCEPT = 0;

private:
    const char* m_objectName;
};

struct ServiceProvider::IObjectWrapper : IObject
{
    explicit IObjectWrapper(const char* name) EXT_NOEXCEPT : IObject(name) {}

    // Get wrapped object reference
    EXT_NODISCARD virtual std::shared_ptr<IObject>& GetWrappedObject() EXT_NOEXCEPT = 0;
    // Get wrapped interface hash
    EXT_NODISCARD virtual size_t GetWrappedInterfaceHash() const EXT_NOEXCEPT = 0;
};

template <typename Interface>
EXT_NODISCARD std::shared_ptr<Interface> ServiceProvider::GetInterface() const EXT_THROWS(di::not_registered, ...)
{
    if (auto it = m_registeredObjects.find(typeid(Interface).hash_code()); it != m_registeredObjects.end() && !it->second.empty())
    {
        try
        {
            EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "getting " << typeid(Interface).name() << " interface";
            const std::any object = it->second.back()->GetObject(std::const_pointer_cast<ServiceProvider>(shared_from_this()));
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

inline ServiceProvider::Ptr ServiceProvider::CreateScope() EXT_NOEXCEPT
{
    return CreateScope(m_registeredObjects, m_createdScopesMutex, m_createdScopes);
}

inline void ServiceProvider::Reset() EXT_NOEXCEPT
{
    InterfaceMap registeredObjects;
    for (auto&& [interfaceId, objects] : m_registeredObjects)
    {
        for (auto& object : objects)
        {
            object->Reset();
        }
    }

    std::unique_lock<std::mutex> lock(m_createdScopesMutex);
    for (auto& scopeServiceProvider : m_createdScopes)
    {
        const auto provider = scopeServiceProvider.lock();
        if (provider)
            provider->Reset();
    }
    m_createdScopes.clear();
}

inline ServiceProvider::ServiceProvider(InterfaceMap&& objectsMap) EXT_NOEXCEPT
    : m_registeredObjects(std::move(objectsMap))
{}

inline EXT_NODISCARD ServiceProvider::Ptr ServiceProvider::CreateScope(const InterfaceMap& registeredObjects,
                                                                       std::mutex& createdScopesMutex,
                                                                       std::list<std::weak_ptr<ServiceProvider>>& createdScopes) EXT_NOEXCEPT
{
    ServiceProvider::InterfaceMap newScopeObjects;
    for (auto&& [interfaceId, objects] : registeredObjects)
    {
        auto& interfaceScopeList = newScopeObjects[interfaceId];
        std::transform(objects.cbegin(), objects.cend(), std::back_inserter(interfaceScopeList),
            [](const std::shared_ptr<ServiceProvider::IObject>& object)
            {
                return object->CreateScopedObject();
            });
    }

    // update all wrappers, in new scope
    ServiceProvider::UpdateObjectWrappersInNewScope(registeredObjects, newScopeObjects);

    const ServiceProvider::Ptr serviceProvider(new ServiceProvider(std::move(newScopeObjects)));
    {
        // memorize new service provider and clean deleted
        std::unique_lock<std::mutex> lock(createdScopesMutex);
        createdScopes.erase(std::remove_if(createdScopes.begin(), createdScopes.end(),
            [](const std::weak_ptr<ServiceProvider>& pointer)
            {
                return pointer.expired();
            }), createdScopes.end());
        createdScopes.emplace_back(serviceProvider);
    }
    return serviceProvider;
}

inline void ServiceProvider::UpdateObjectWrappersInNewScope(const ServiceProvider::InterfaceMap& parentScope,
                                                            ServiceProvider::InterfaceMap& newScope) EXT_NOEXCEPT
{
    std::list<std::shared_ptr<IObjectWrapper>> handledWrappers;
    for (auto&& [interfaceId, objectsList] : newScope)
    {
        for (auto& object : objectsList)
        {
            const auto objectWrapper = std::dynamic_pointer_cast<IObjectWrapper>(object);
            if (!!objectWrapper)
                handledWrappers.push_back(objectWrapper);
        }
    }

    while (!handledWrappers.empty())
    {
        std::shared_ptr<IObjectWrapper> objectWrapper = handledWrappers.front();
        handledWrappers.pop_front();

        const size_t wrappedInterfaceHash = objectWrapper->GetWrappedInterfaceHash();
        // wrapped interface might be unregistered search for the other wrappers in old scope
        std::shared_ptr<ServiceProvider::IObject>& wrappedObject = objectWrapper->GetWrappedObject();

        // search for wrapped interface in scope
        const auto objectInParentScopeIt = parentScope.find(wrappedInterfaceHash);
        if (objectInParentScopeIt != parentScope.end())
        {
            const auto it = std::find_if(objectInParentScopeIt->second.cbegin(), objectInParentScopeIt->second.cend(),
                [&wrappedObject](const std::shared_ptr<IObject>& object)
                {
                    return object == wrappedObject;
                });
            if (it != objectInParentScopeIt->second.cend())
            {
                const size_t indexInParentScope = std::distance(objectInParentScopeIt->second.cbegin(), it);
                EXT_ASSERT(newScope[wrappedInterfaceHash].size() > indexInParentScope) << "Scopes must be the same size";
                wrappedObject = *std::next(newScope[wrappedInterfaceHash].begin(), indexInParentScope);
                continue;
            }
        }

        // wrapped interface might be unregistered, search for the other wrappers in old scope
        const std::shared_ptr<ServiceProvider::IObject> objectInParentScope = wrappedObject;
        wrappedObject = wrappedObject->CreateScopedObject();

        for (auto it = handledWrappers.begin(); it != handledWrappers.end();)
        {
            auto& otherWrapperObject = it->get()->GetWrappedObject();
            if (otherWrapperObject == objectInParentScope)
            {
                otherWrapperObject = wrappedObject;
                it = handledWrappers.erase(it);
            }
            else
                ++it;
        }
    }
}

template <typename Object, typename Interface>
struct ServiceCollection::SingletonObject : ServiceProvider::IObject,
    ext::enable_shared_from_this<ServiceCollection::SingletonObject<Object, Interface>, ServiceProvider::IObject>
{
    explicit SingletonObject() EXT_NOEXCEPT : IObject(typeid(Object).name()) {}
    EXT_NODISCARD size_t GetHash() const EXT_NOEXCEPT override final { return typeid(Object).hash_code(); }
    EXT_NODISCARD std::any GetObject(ServiceProvider::Ptr&& serviceProvider) override final
    {
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            if (m_object)
                return std::make_any<std::shared_ptr<Interface>>(m_object);
        }

        std::unique_lock<std::shared_mutex> lock(m_mutex);
        if (!m_object)
            m_object = std::static_pointer_cast<Interface>(ext::CreateObject<Object>(std::move(serviceProvider)));
        return std::make_any<std::shared_ptr<Interface>>(m_object);
    }
    virtual EXT_NODISCARD std::shared_ptr<ServiceProvider::IObject> CreateScopedObject() EXT_NOEXCEPT override
    {
        // only one instance on whole scopes
        return ext::enable_shared_from_this<ServiceCollection::SingletonObject<Object, Interface>, ServiceProvider::IObject>::shared_from_this();
    }
    void Reset() EXT_NOEXCEPT override final { std::unique_lock<std::shared_mutex> lock(m_mutex); m_object = nullptr; }

private:
    std::shared_mutex m_mutex;
    std::shared_ptr<Interface> m_object;
};

template <typename Object, typename Interface>
struct ServiceCollection::ScopedObject final : ServiceCollection::SingletonObject<Object, Interface>
{
    EXT_NODISCARD std::shared_ptr<ServiceProvider::IObject> CreateScopedObject() EXT_NOEXCEPT override final
    {
        // new instance for each scope
        return std::make_shared<ScopedObject<Object, Interface>>();
    }
};

template <typename Object, typename Interface>
struct ServiceCollection::TransientObject final : ServiceProvider::IObject
{
    explicit TransientObject() EXT_NOEXCEPT : IObject(typeid(Object).name()) {}

    EXT_NODISCARD size_t GetHash() const EXT_NOEXCEPT override final { return typeid(Object).hash_code(); }
    EXT_NODISCARD std::any GetObject(ServiceProvider::Ptr&& serviceProvider) override final
    {
        return std::make_any<std::shared_ptr<Interface>>(std::static_pointer_cast<Interface>(ext::CreateObject<Object>(std::move(serviceProvider))));
    }
    EXT_NODISCARD std::shared_ptr<ServiceProvider::IObject> CreateScopedObject() EXT_NOEXCEPT override final
    {
        // new instance every time
        return std::make_shared<TransientObject<Object, Interface>>();
    }
    void Reset() EXT_NOEXCEPT override final {}
};

template <typename Object, typename Interface, typename InterfaceWrapped>
struct ServiceCollection::WrapperObject final : ServiceProvider::IObjectWrapper,
    ext::enable_shared_from_this<ServiceCollection::WrapperObject<Object, Interface, InterfaceWrapped>, ServiceProvider::IObject>
{
    explicit WrapperObject(const std::shared_ptr<ServiceProvider::IObject>& object) EXT_NOEXCEPT
        : IObjectWrapper(typeid(Object).name())
        , m_object(object)
    {}

    EXT_NODISCARD size_t GetHash() const EXT_NOEXCEPT override final { return typeid(Object).hash_code(); }
    EXT_NODISCARD std::any GetObject(ServiceProvider::Ptr&& serviceProvider) override final
    {
        std::shared_ptr<Interface> interfacePtr;
        try
        {
            const auto object = m_object->GetObject(std::move(serviceProvider));
            const std::shared_ptr<InterfaceWrapped> wrappedInterface = std::any_cast<std::shared_ptr<InterfaceWrapped>>(object);
            interfacePtr = std::dynamic_pointer_cast<Interface>(wrappedInterface);
        }
        catch (const std::bad_any_cast&)
        {
            EXT_ASSERT(false);
            EXT_TRACE_ERR() << EXT_TRACE_FUNCTION << "failed to get " << typeid(InterfaceWrapped).name()
                << ", internal error, can`t get it from " << typeid(Object).name();
            throw;
        }
        EXT_EXPECT(!!interfacePtr) << "Failed to convert "  << typeid(InterfaceWrapped).name() << " to "
            << typeid(Interface).name() << " inherited from" << typeid(Object).name();
        return std::make_any<std::shared_ptr<Interface>>(interfacePtr);
    }
    EXT_NODISCARD std::shared_ptr<ServiceProvider::IObject> CreateScopedObject() EXT_NOEXCEPT override final
    {
        return std::make_shared<ServiceCollection::WrapperObject<Object, Interface, InterfaceWrapped>>(m_object);
    }
    void Reset() EXT_NOEXCEPT override final { m_object->Reset(); }

    EXT_NODISCARD std::shared_ptr<IObject>& GetWrappedObject() EXT_NOEXCEPT override final { return m_object; }
    EXT_NODISCARD size_t GetWrappedInterfaceHash() const EXT_NOEXCEPT override final { return typeid(InterfaceWrapped).hash_code(); }

private:
    std::shared_ptr<ServiceProvider::IObject> m_object;
};

template <typename Class, typename... Interfaces>
void ServiceCollection::RegisterTransient()
{
    std::unique_lock lock(m_registeredObjectsMutex);
    ext::mpl::ForEach<ext::mpl::list<Interfaces...>>::Call([&](auto* type)
    {
        using Interface = std::remove_pointer_t<decltype(type)>;
        di::check_inheritance<Class, Interface>::Check();
        RegisterObject<Interface>([]()
        {
            return std::make_shared<TransientObject<Class, Interface>>();;
        });
    });
}

template <typename Class, typename... Interfaces>
void ServiceCollection::RegisterScoped()
{
    using ObjectBasedInterface = ext::mpl::get_type_t<0, Interfaces...>;
    const std::shared_ptr<ScopedObject<Class, ObjectBasedInterface>> basedObject = std::make_shared<ScopedObject<Class, ObjectBasedInterface>>();

    std::unique_lock lock(m_registeredObjectsMutex);
    ext::mpl::ForEach<ext::mpl::list<Interfaces...>>::Call([&](auto* type)
    {
        using Interface = std::remove_pointer_t<decltype(type)>;
        di::check_inheritance<Class, Interface>::Check();
        RegisterObject<Interface>([&basedObject]()
        {
            if constexpr (std::is_same_v<Interface, ObjectBasedInterface>)
                return basedObject;
            else
                return std::make_shared<WrapperObject<Class, Interface, ObjectBasedInterface>>(basedObject);
        });
    });
}

template <typename Class, typename... Interfaces>
void ServiceCollection::RegisterSingleton()
{
    using ObjectBasedInterface = ext::mpl::get_type_t<0, Interfaces...>;
    const std::shared_ptr<SingletonObject<Class, ObjectBasedInterface>> basedObject = std::make_shared<SingletonObject<Class, ObjectBasedInterface>>();

    std::unique_lock lock(m_registeredObjectsMutex);
    ext::mpl::ForEach<ext::mpl::list<Interfaces...>>::Call([&](auto* type)
    {
        using Interface = std::remove_pointer_t<decltype(type)>;
        di::check_inheritance<Class, Interface>::Check();
        RegisterObject<Interface>([&basedObject]()
        {
            if constexpr (std::is_same_v<Interface, ObjectBasedInterface>)
                return basedObject;
            else
                return std::make_shared<WrapperObject<Class, Interface, ObjectBasedInterface>>(basedObject);
        });
    });
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

template <typename Class, typename... Interfaces>
void ServiceCollection::UnregisterObject() EXT_NOEXCEPT
{
    std::unique_lock lock(m_registeredObjectsMutex);
    ext::mpl::ForEach<ext::mpl::list<Interfaces...>>::Call([&, hash = typeid(Class).hash_code()](auto* type)
    {
        using Interface = std::remove_pointer_t<decltype(type)>;
        di::check_inheritance<Class, Interface>::Check();
        auto interfaceIt = m_registeredObjects.find(typeid(Interface).hash_code());
        if (interfaceIt != m_registeredObjects.end())
        {
            const auto eraseIt = std::remove_if(interfaceIt->second.begin(), interfaceIt->second.end(),
                [&hash](const std::shared_ptr<ServiceProvider::IObject>& object)
                {
                    return object->GetHash() == hash;
                });

            interfaceIt->second.erase(eraseIt, interfaceIt->second.end());
            if (interfaceIt->second.empty())
                m_registeredObjects.erase(interfaceIt);
        }
    });
}

inline void ServiceCollection::UnregisterAll() EXT_NOEXCEPT
{
    std::unique_lock lock(m_registeredObjectsMutex);
    m_registeredObjects.clear();
}

template <typename Interface>
void ServiceCollection::RegisterObject(const std::function<std::shared_ptr<ServiceProvider::IObject>()>& registerObject)
{
    EXT_ASSERT(!m_registeredObjectsMutex.try_lock());
    m_registeredObjects[typeid(Interface).hash_code()].emplace_back(registerObject());
}

inline ServiceProvider::Ptr ServiceCollection::BuildServiceProvider()
{
    std::shared_lock lock(m_registeredObjectsMutex);
    return ServiceProvider::CreateScope(m_registeredObjects, m_creaedServiceProvidersMutex, m_createdServiceProviders);
}

inline ext::ServiceCollection::~ServiceCollection() EXT_NOEXCEPT
{
    ResetObjects();
}

inline void ServiceCollection::ResetObjects() EXT_NOEXCEPT
{
    std::unique_lock<std::mutex> lock(m_creaedServiceProvidersMutex);
    for (auto& serviceProvider : m_createdServiceProviders)
    {
        const auto provider = serviceProvider.lock();
        if (provider)
            provider->Reset();
    }
    m_createdServiceProviders.clear();
}

#pragma endregion Implementation

} // namespace ext

#pragma pop_macro("GetObject")