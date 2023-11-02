// checking dependency injection creating objects

#include "gtest/gtest.h"

#include <ext/core/defines.h>
#include <ext/core/dependency_injection.h>
#include <ext/core/singleton.h>

struct Interface1
{
    virtual ~Interface1() = default;
};

struct Interface1Impl : Interface1
{};

struct Interface1Impl2 : Interface1
{};

struct Interface2
{
    virtual ~Interface2() = default;
};

struct Interface2Impl : Interface2
{};

struct Interface12Impl : Interface1, Interface2
{};

struct CreatedObject
{
    explicit CreatedObject(std::shared_ptr<Interface1> i1, std::shared_ptr<Interface2> i2)
        : m_i1(std::move(i1))
        , m_i2(std::move(i2))
    {}

    std::shared_ptr<Interface1> m_i1;
    std::shared_ptr<Interface2> m_i2;
};

struct CreatedObject2 : ext::ServiceProviderHolder
{
    explicit CreatedObject2(std::shared_ptr<Interface1> shared, ext::lazy_interface<Interface1> lazy, ext::ServiceProvider::Ptr&& serviceProvider)
        : ServiceProviderHolder(std::move(serviceProvider))
        , m_shared(std::move(shared))
        , m_lazy(std::move(lazy))
        , m_lazy2(ServiceProviderHolder::m_serviceProvider)
    {}

    std::shared_ptr<Interface2> GetRandomInterface() const
    {
        return ServiceProviderHolder::GetInterface<Interface2>();
    }

    std::shared_ptr<Interface2> GetRandomInterfaceOption2() const
    {
        return ::ext::GetInterface<Interface2>(ServiceProviderHolder::m_serviceProvider);
    }

    std::shared_ptr<Interface1> m_shared;
    ext::lazy_interface<Interface1> m_lazy;
    ext::lazy_interface<Interface1> m_lazy2;
};

struct dependency_injection_fixture : testing::Test
{
    void SetUp() override
    {
        m_serviceCollection.UnregisterAll();
    }

    ext::ServiceCollection& m_serviceCollection = ext::get_service<ext::ServiceCollection>();
};

TEST_F(dependency_injection_fixture, creating_without_registration)
{
    ASSERT_THROW(auto const _ = ext::CreateObject<CreatedObject>(m_serviceCollection.BuildServiceProvider()), ext::di::not_registered);
}

TEST_F(dependency_injection_fixture, creating_with_registration)
{
    m_serviceCollection.RegisterScoped<Interface1Impl, Interface1>();
    m_serviceCollection.RegisterScoped<Interface2Impl, Interface2>();

    const std::shared_ptr<CreatedObject> object = ext::CreateObject<CreatedObject>(m_serviceCollection.BuildServiceProvider());
    ASSERT_NE(nullptr, object);
    EXPECT_NE(nullptr, object->m_i1);
    EXPECT_NE(nullptr, object->m_i2);

    const std::shared_ptr<CreatedObject2> object2 = ext::CreateObject<CreatedObject2>(m_serviceCollection.BuildServiceProvider());
    EXPECT_NE(nullptr, object2);
    EXPECT_NE(nullptr, object2->GetRandomInterface());
    EXPECT_NE(nullptr, object2->GetRandomInterfaceOption2());
}

TEST_F(dependency_injection_fixture, creating_scope)
{
    m_serviceCollection.RegisterScoped<Interface1Impl, Interface1>();
    m_serviceCollection.RegisterScoped<Interface2Impl, Interface2>();

    EXPECT_NE(m_serviceCollection.BuildServiceProvider(), m_serviceCollection.BuildServiceProvider()->CreateScope());
    EXPECT_NE(nullptr, m_serviceCollection.BuildServiceProvider()->CreateScope());
}

TEST_F(dependency_injection_fixture, check_singleton)
{
    m_serviceCollection.RegisterSingleton<Interface1Impl, Interface1>();
    const auto serviceProvider = m_serviceCollection.BuildServiceProvider();

    const std::shared_ptr<Interface1> interface1 = ext::GetInterface<Interface1>(serviceProvider);
    const std::shared_ptr<Interface1> interface2 = ext::GetInterface<Interface1>(serviceProvider);
    const std::shared_ptr<Interface1Impl> object = ext::CreateObject<Interface1Impl>(serviceProvider);
    EXPECT_NE(nullptr, interface1);
    EXPECT_NE(nullptr, interface2);
    EXPECT_NE(nullptr, object);
    EXPECT_EQ(interface1, interface2);
    EXPECT_NE(interface1, object);

    EXPECT_EQ(interface1, ext::GetInterface<Interface1>(m_serviceCollection.BuildServiceProvider()));
    EXPECT_EQ(interface1, ext::GetInterface<Interface1>(serviceProvider->CreateScope()));

    ASSERT_THROW(auto const _ = ext::GetInterface<Interface2>(serviceProvider), ext::di::not_registered);

    {
        m_serviceCollection.RegisterScoped<Interface12Impl, Interface1, Interface2>();
        const std::shared_ptr<Interface1> interface1 = ext::GetInterface<Interface1>(serviceProvider);
        const std::shared_ptr<Interface1> interface2 = ext::GetInterface<Interface1>(serviceProvider);
        const std::shared_ptr<Interface1Impl> object = ext::CreateObject<Interface1Impl>(serviceProvider);
    }
}

TEST_F(dependency_injection_fixture, check_singleton_multiple_interface_registration)
{
    m_serviceCollection.RegisterSingleton<Interface12Impl, Interface1, Interface2>();
    const auto serviceProvider = m_serviceCollection.BuildServiceProvider();

    std::shared_ptr<Interface1> interface1 = ext::GetInterface<Interface1>(serviceProvider);
    const std::shared_ptr<Interface2> interface2 = ext::GetInterface<Interface2>(serviceProvider);

    EXPECT_NE(nullptr, interface1);
    EXPECT_NE(nullptr, interface2);
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface12Impl>(interface1));
    EXPECT_EQ(std::dynamic_pointer_cast<Interface12Impl>(interface1), std::dynamic_pointer_cast<Interface12Impl>(interface2));

    m_serviceCollection.RegisterSingleton<Interface1Impl, Interface1>();
    interface1 = ext::GetInterface<Interface1>(m_serviceCollection.BuildServiceProvider());
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface1Impl>(interface1));
}

TEST_F(dependency_injection_fixture, check_scoped)
{
    m_serviceCollection.RegisterScoped<Interface12Impl, Interface1, Interface2>();
    const auto serviceProviderScope1 = m_serviceCollection.BuildServiceProvider();

    const std::shared_ptr<Interface12Impl> interface1InScope1 = std::dynamic_pointer_cast<Interface12Impl>(ext::GetInterface<Interface1>(serviceProviderScope1));
    const std::shared_ptr<Interface12Impl> interface2InScope1 = std::dynamic_pointer_cast<Interface12Impl>(ext::GetInterface<Interface2>(serviceProviderScope1));
    const std::shared_ptr<Interface12Impl> objectInScope1 = ext::CreateObject<Interface12Impl>(serviceProviderScope1);
    EXPECT_NE(nullptr, interface1InScope1);
    EXPECT_NE(nullptr, interface2InScope1);
    EXPECT_NE(nullptr, objectInScope1);
    EXPECT_EQ(interface1InScope1, interface2InScope1);
    EXPECT_NE(interface1InScope1, objectInScope1);

    auto checkScope = [&](const ext::ServiceProvider::Ptr& serviceProvider)
    {
        const std::shared_ptr<Interface12Impl> interface1InScope2 = std::dynamic_pointer_cast<Interface12Impl>(ext::GetInterface<Interface1>(serviceProvider));
        const std::shared_ptr<Interface12Impl> interface2InScope2 = std::dynamic_pointer_cast<Interface12Impl>(ext::GetInterface<Interface2>(serviceProvider));
        const std::shared_ptr<Interface12Impl> objectInScope2 = ext::CreateObject<Interface12Impl>(serviceProvider);
        EXPECT_NE(nullptr, interface1InScope2);
        EXPECT_NE(nullptr, interface2InScope2);
        EXPECT_NE(nullptr, objectInScope2);
        EXPECT_EQ(interface1InScope2, interface2InScope2);
        EXPECT_NE(interface1InScope2, objectInScope2);

        EXPECT_NE(interface1InScope1, interface2InScope2) << "Object should be different in different scopes";
    };
    checkScope(m_serviceCollection.BuildServiceProvider());
    checkScope(serviceProviderScope1->CreateScope());

    m_serviceCollection.RegisterSingleton<Interface12Impl, Interface1, Interface2>();
    const auto serviceProviderSingleton = m_serviceCollection.BuildServiceProvider();
    const std::shared_ptr<Interface12Impl> baseSingleton = std::dynamic_pointer_cast<Interface12Impl>(ext::GetInterface<Interface1>(serviceProviderSingleton));
    EXPECT_NE(nullptr, baseSingleton);

    const std::shared_ptr<Interface12Impl> newServiceProviderSingleton = std::dynamic_pointer_cast<Interface12Impl>(ext::GetInterface<Interface1>(m_serviceCollection.BuildServiceProvider()));
    EXPECT_EQ(newServiceProviderSingleton, baseSingleton);
    const std::shared_ptr<Interface12Impl> newScopeSingleton = std::dynamic_pointer_cast<Interface12Impl>(ext::GetInterface<Interface2>(serviceProviderSingleton->CreateScope()));
    EXPECT_EQ(newScopeSingleton, baseSingleton);
    const std::shared_ptr<Interface12Impl> objectInNewScope = ext::CreateObject<Interface12Impl>(serviceProviderSingleton->CreateScope());
    EXPECT_NE(objectInNewScope, baseSingleton);
}

TEST_F(dependency_injection_fixture, check_scoped_with_unregistration)
{
    struct Interface3
    {
        virtual ~Interface3() = default;
    };

    struct Interface123Impl : Interface1, Interface2, Interface3
    {
    };

    m_serviceCollection.RegisterScoped<Interface123Impl, Interface1, Interface2, Interface3>();
    m_serviceCollection.Unregister<Interface1>();

    const auto serviceProviderScope1 = m_serviceCollection.BuildServiceProvider();

    const std::shared_ptr<Interface123Impl> interface1InScope1 = std::dynamic_pointer_cast<Interface123Impl>(ext::GetInterface<Interface2>(serviceProviderScope1));
    const std::shared_ptr<Interface123Impl> interface2InScope1 = std::dynamic_pointer_cast<Interface123Impl>(ext::GetInterface<Interface3>(serviceProviderScope1));

    EXPECT_NE(nullptr, interface1InScope1);
    EXPECT_EQ(interface1InScope1, interface2InScope1);
    EXPECT_THROW(auto const _ = ext::GetInterface<Interface1>(serviceProviderScope1), ext::dependency_injection::not_registered);

    {
        const auto newServiceProvider = m_serviceCollection.BuildServiceProvider();
        const std::shared_ptr<Interface123Impl> interface1InScope1 = std::dynamic_pointer_cast<Interface123Impl>(ext::GetInterface<Interface2>(newServiceProvider));
        const std::shared_ptr<Interface123Impl> interface2InScope1 = std::dynamic_pointer_cast<Interface123Impl>(ext::GetInterface<Interface3>(newServiceProvider));
        EXPECT_NE(nullptr, interface1InScope1);
        EXPECT_EQ(interface1InScope1, interface2InScope1);
    }

    {
        const auto newScope = serviceProviderScope1->CreateScope();
        const std::shared_ptr<Interface123Impl> interface1InScope1 = std::dynamic_pointer_cast<Interface123Impl>(ext::GetInterface<Interface2>(newScope));
        const std::shared_ptr<Interface123Impl> interface2InScope1 = std::dynamic_pointer_cast<Interface123Impl>(ext::GetInterface<Interface3>(newScope));
        EXPECT_NE(nullptr, interface1InScope1);
        EXPECT_EQ(interface1InScope1, interface2InScope1);
    }
}

TEST_F(dependency_injection_fixture, check_scoped_multiple_interface_registration)
{
    m_serviceCollection.RegisterScoped<Interface12Impl, Interface1, Interface2>();
    auto serviceProvider = m_serviceCollection.BuildServiceProvider();

    std::shared_ptr<Interface1> interface1 = ext::GetInterface<Interface1>(serviceProvider);
    std::shared_ptr<Interface2> interface2 = ext::GetInterface<Interface2>(serviceProvider);

    EXPECT_NE(nullptr, interface1);
    EXPECT_NE(nullptr, interface2);
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface12Impl>(interface1));
    EXPECT_EQ(std::dynamic_pointer_cast<Interface12Impl>(interface1), std::dynamic_pointer_cast<Interface12Impl>(interface1));

    m_serviceCollection.RegisterScoped<Interface12Impl, Interface1, Interface2>();
    m_serviceCollection.RegisterScoped<Interface1Impl, Interface1>();

    serviceProvider = m_serviceCollection.BuildServiceProvider();
    interface1 = ext::GetInterface<Interface1>(serviceProvider);
    interface2 = ext::GetInterface<Interface2>(serviceProvider);
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface1Impl>(interface1));
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface12Impl>(interface2));
}

TEST_F(dependency_injection_fixture, check_transient)
{
    m_serviceCollection.RegisterTransient<Interface1Impl, Interface1>();
    const auto serviceProviderScope1 = m_serviceCollection.BuildServiceProvider();

    const std::shared_ptr<Interface1> interface1InScope1 = ext::GetInterface<Interface1>(serviceProviderScope1);
    const std::shared_ptr<Interface1> interface2InScope1 = ext::GetInterface<Interface1>(serviceProviderScope1);
    const std::shared_ptr<Interface1Impl> objectInScope1 = ext::CreateObject<Interface1Impl>(serviceProviderScope1);
    EXPECT_NE(nullptr, interface1InScope1);
    EXPECT_NE(nullptr, interface2InScope1);
    EXPECT_NE(nullptr, objectInScope1);
    EXPECT_NE(interface1InScope1, interface2InScope1);
    EXPECT_NE(interface1InScope1, objectInScope1);

    const auto serviceProviderScope2 = m_serviceCollection.BuildServiceProvider();

    const std::shared_ptr<Interface1> interface1InScope2 = ext::GetInterface<Interface1>(serviceProviderScope2);
    const std::shared_ptr<Interface1> interface2InScope2 = ext::GetInterface<Interface1>(serviceProviderScope2);
    const std::shared_ptr<Interface1Impl> objectInScope2 = ext::CreateObject<Interface1Impl>(serviceProviderScope2);
    EXPECT_NE(nullptr, interface1InScope2);
    EXPECT_NE(nullptr, interface2InScope2);
    EXPECT_NE(nullptr, objectInScope2);
    EXPECT_NE(interface1InScope2, interface2InScope2);
    EXPECT_NE(interface1InScope2, objectInScope2);
    EXPECT_NE(interface1InScope1, interface1InScope2);
    EXPECT_NE(objectInScope1, objectInScope2);

    ASSERT_THROW(auto const _ = ext::GetInterface<Interface2>(serviceProviderScope1), ext::di::not_registered);
    ASSERT_THROW(auto const _ = ext::GetInterface<Interface2>(serviceProviderScope2), ext::di::not_registered);
}

TEST_F(dependency_injection_fixture, check_transient_multiple_interface_registration)
{
    m_serviceCollection.RegisterTransient<Interface12Impl, Interface1, Interface2>();
    const auto serviceProvider = m_serviceCollection.BuildServiceProvider();

    std::shared_ptr<Interface1> interface1 = ext::GetInterface<Interface1>(serviceProvider);
    const std::shared_ptr<Interface2> interface2 = ext::GetInterface<Interface2>(serviceProvider);

    EXPECT_NE(nullptr, interface1);
    EXPECT_NE(nullptr, interface2);
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface12Impl>(interface1));
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface12Impl>(interface2));
    EXPECT_NE(std::dynamic_pointer_cast<Interface12Impl>(interface1), std::dynamic_pointer_cast<Interface12Impl>(interface2));

    m_serviceCollection.RegisterTransient<Interface1Impl, Interface1>();
    interface1 = ext::GetInterface<Interface1>(m_serviceCollection.BuildServiceProvider());
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface1Impl>(interface1));
}

TEST_F(dependency_injection_fixture, check_multi_registration)
{
    m_serviceCollection.RegisterTransient<Interface1Impl, Interface1>();
    m_serviceCollection.RegisterTransient<Interface1Impl2, Interface1>();
    auto serviceProvider = m_serviceCollection.BuildServiceProvider();

    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface1Impl2>(ext::GetInterface<Interface1>(serviceProvider)));

    m_serviceCollection.RegisterSingleton<Interface1Impl, Interface1>();
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface1Impl2>(ext::GetInterface<Interface1>(serviceProvider)));

    serviceProvider = m_serviceCollection.BuildServiceProvider();
    const auto interfaceObj1 = ext::GetInterface<Interface1>(serviceProvider);
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<Interface1Impl>(interfaceObj1));
    const auto interfaceObj2 = ext::GetInterface<Interface1>(serviceProvider);
    EXPECT_EQ(interfaceObj1, interfaceObj2);
}

int LazyObjectCounter = 0;
TEST_F(dependency_injection_fixture, lazy_getter_test)
{
    LazyObjectCounter = 0;
    struct LazyObjectTester
    {
        LazyObjectTester(ext::lazy_interface<Interface1> lazyInterface)
            : m_lazy(std::move(lazyInterface))
        {
            ++LazyObjectCounter;
        }

        void UseInterface()
        {
            EXT_UNUSED(m_lazy.get());
        }
        ext::lazy_interface<Interface1> m_lazy;
    };

    struct LazyInterface1 : Interface1
    {
        LazyInterface1()
        {
            ++LazyObjectCounter;
        }
    };

    m_serviceCollection.RegisterTransient<LazyInterface1, Interface1>();
    ext::lazy_object<LazyObjectTester> objectCounter(m_serviceCollection.BuildServiceProvider());
    EXPECT_EQ(0, LazyObjectCounter);
    EXT_UNUSED(objectCounter.get()); // for create
    EXPECT_EQ(1, LazyObjectCounter);
    objectCounter->UseInterface();
    EXPECT_EQ(2, LazyObjectCounter);
}

TEST_F(dependency_injection_fixture, check_resetting_objects)
{
    struct TestObject
    {
        TestObject(ext::ServiceProvider::Ptr&& serviceProvider)
            : m_serviceProvider(std::move(serviceProvider))
        {
            ++LazyObjectCounter;
        }
        ~TestObject()
        {
            --LazyObjectCounter;
        }

        ext::ServiceProvider::Ptr m_serviceProvider;
    };

    LazyObjectCounter = 0;
    m_serviceCollection.RegisterScoped<TestObject, TestObject>();

    auto serviceProvider = m_serviceCollection.BuildServiceProvider();
    std::shared_ptr<TestObject> objectCounter = ext::GetInterface<TestObject>(serviceProvider);

    EXPECT_EQ(1, LazyObjectCounter);
    objectCounter = nullptr;
    EXPECT_EQ(1, LazyObjectCounter);
    serviceProvider->Reset();
    EXPECT_EQ(0, LazyObjectCounter) << "After resetting scope object must be destroyed becaouse object refs = 0";

    objectCounter = ext::GetInterface<TestObject>(serviceProvider);
    EXPECT_EQ(1, LazyObjectCounter);
    serviceProvider = nullptr;
    objectCounter = nullptr;
    EXPECT_EQ(1, LazyObjectCounter);
    m_serviceCollection.ResetObjects();
    EXPECT_EQ(0, LazyObjectCounter) << "After resetting collection all scopes must be destroyed becaouse object refs = 0";
}

TEST_F(dependency_injection_fixture, cyclic_dependencies)
{
    struct Object1 : Interface1 { 
        Object1(std::shared_ptr<Interface2>) {}
    };
    struct Object2 : Interface2 {
        Object2(std::shared_ptr<Interface1>) {}
    };

    m_serviceCollection.RegisterScoped<Object1, Interface1>();
    m_serviceCollection.RegisterScoped<Object2, Interface2>();

    EXPECT_THROW(static_cast<void>(ext::GetInterface<Interface1>(m_serviceCollection.BuildServiceProvider())),
                 ext::di::cyclic_dependency);

    m_serviceCollection.RegisterTransient<Object2, Interface2>();
    EXPECT_THROW(static_cast<void>(ext::GetInterface<Interface1>(m_serviceCollection.BuildServiceProvider())),
                 ext::di::cyclic_dependency);

    m_serviceCollection.RegisterSingleton<Object2, Interface2>();
    EXPECT_THROW(static_cast<void>(ext::GetInterface<Interface1>(m_serviceCollection.BuildServiceProvider())),
                 ext::di::cyclic_dependency);
}

TEST_F(dependency_injection_fixture, cyclic_dependencies_lazy_objects)
{
    struct Object1 : Interface1 {
        Object1(std::shared_ptr<Interface2>) {}
    };
    struct ObjectLazyInterface : Interface2 {
        ObjectLazyInterface(ext::lazy_interface<Interface1> obj) { (void)obj.get(); }
    };
    struct ObjectLazyObject : Interface2 {
        ObjectLazyObject(ext::lazy_object<Object1> obj) { (void)obj.get(); }
    };
    struct ObjectLazyWeakInterface : Interface2 {
        ObjectLazyWeakInterface(ext::lazy_weak_interface<Interface1> obj) { (void)obj.get(); }
    };
    struct GetObjectAfterConstructor : Interface2 {
        GetObjectAfterConstructor(ext::lazy_weak_interface<Interface1> obj) : m_obj(obj) {}
        ext::lazy_weak_interface<Interface1> m_obj;
    };

    m_serviceCollection.RegisterScoped<Object1, Interface1>();
    m_serviceCollection.RegisterScoped<ObjectLazyInterface, Interface2>();
    EXPECT_THROW(static_cast<void>(ext::CreateObject<ObjectLazyInterface>(m_serviceCollection.BuildServiceProvider())),
                 ext::di::cyclic_dependency);
    m_serviceCollection.RegisterScoped<ObjectLazyObject, Interface2>();
    EXPECT_THROW(static_cast<void>(ext::CreateObject<ObjectLazyObject>(m_serviceCollection.BuildServiceProvider())),
                 ext::di::cyclic_dependency);
    m_serviceCollection.RegisterScoped<ObjectLazyWeakInterface, Interface2>();
    EXPECT_THROW(static_cast<void>(ext::CreateObject<ObjectLazyWeakInterface>(m_serviceCollection.BuildServiceProvider())),
                 ext::di::cyclic_dependency);

    m_serviceCollection.RegisterScoped<GetObjectAfterConstructor, Interface2>();
    auto object = ext::CreateObject<GetObjectAfterConstructor>(m_serviceCollection.BuildServiceProvider());
    EXPECT_NO_THROW(static_cast<void>(object->m_obj.get()));
}

TEST_F(dependency_injection_fixture, getting_lazy_registered_objects)
{
    static int counter1 = 0, counter2 = 0;
    struct Object1 : Interface1 {
        Object1() { ++counter1; }
    };
    struct Object2 : Interface1 {
        Object2() { ++counter2; }
    };
    struct Object3 : Interface1 {
        Object3() { throw std::runtime_error("Test"); }
    };

    m_serviceCollection.RegisterTransient<Object1, Interface1>();
    m_serviceCollection.RegisterTransient<Object2, Interface1>();
    m_serviceCollection.RegisterTransient<Object3, Interface1>();

    const auto objects = m_serviceCollection.BuildServiceProvider()->GetLazyInterfaces<Interface1>();
    EXPECT_EQ(0, counter1);
    EXPECT_EQ(0, counter2);
    EXPECT_EQ(3, objects.size());

    const auto getObject = [&objects](int index) {
        return std::next(objects.begin(), index)->value();
    };

    const std::shared_ptr<Object1> object1 = std::static_pointer_cast<Object1>(getObject(0));
    EXPECT_NE(nullptr, object1);
    const std::shared_ptr<Object2> object2 = std::static_pointer_cast<Object2>(getObject(1));
    EXPECT_NE(nullptr, object2);
    EXPECT_EQ(1, counter1);
    EXPECT_EQ(1, counter2);

    EXPECT_THROW(static_cast<void>(getObject(2)), std::runtime_error);
}