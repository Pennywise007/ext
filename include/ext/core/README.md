# Check code execution and handling errors
<details><summary>Allows to add simple checks inside executing code and manage ezxceptions</summary>

```c++
#include <ext/core/check.h>
```
**EXT_CHECK** - throws exception if expression is false

**EXT_CHECK**(bool_expression) << "Text";
```c++
if (!bool_expression)
	throw ::ext::check::CheckFailedException(EXT_SRC_LOCATION, #bool_expression "Text"));
```

**EXT_EXPECT** - if expression is false:
- Only on first failure: debug break if debugger presents, create dump otherwise
- throws exception

**EXT_EXPECT**(bool_expression) << "Text";
```c++
if (!bool_expression)
{
	if (IsDebuggerPresent())                                            
		DebugBreak();                                                   
	else                                                                
		EXT_DUMP_CREATE();
	throw ::ext::check::CheckFailedException(EXT_SRC_LOCATION, #bool_expression "Text"));
}
```

**EXT_ASSERT / EXT_REQUIRE** - if expression is false in debug mode. Only on first failure: debug break if debugger presents, create dump otherwise

**EXT_ASSERT**(bool_expression) << "Text";

```c++
#ifdef _DEBUG
if (!bool_expression)
{
	if (IsDebuggerPresent())                                            
		DebugBreak();                                                   
	else                                                                
		EXT_DUMP_CREATE();
}
#endif
```

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/core/check.h)
</details>

# Dependency injection
Usage simple with .Net [Microsoft.Extensions.DependencyInjection](https://www.nuget.org/packages/Microsoft.Extensions.DependencyInjection/)
<details><summary>Example</summary>

```c++

#include <ext/core/dependency_injection.h>


struct InterfaceExample
{
    virtual ~InterfaceExample() = default;
};

struct InterfaceImplementationExample : InterfaceExample
{};

struct CreatedObjectExample : ext::ServiceProviderHolder
{
    explicit CreatedObjectExample(std::shared_ptr<InterfaceExample> interfaceShared, std::lazy_interface<InterfaceExample> interfaceLazy, ext::ServiceProvider::Ptr&& serviceProvider)
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

    std::shared_ptr<InterfaceExample> m_interfaceShared;
    ext::lazy_interface<InterfaceExample> m_interfaceLazyOne;
    ext::lazy_interface<InterfaceExample> m_interfaceLazyTwo;
};

ext::ServiceCollection& serviceCollection = ext::get_singleton<ext::ServiceCollection>();
serviceCollection.RegisterScoped<InterfaceImplementationExample, InterfaceExample>();

const std::shared_ptr<CreatedObjectExample> object = ext::CreateObject<CreatedObjectExample>(serviceCollection.BuildServiceProvider());

```

</details>

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/core/dependency_injection.h)
- [Tests and examples](https://github.com/Pennywise007/ext/blob/main/test/core/dependency_injection_test.cpp)

# Event dispatcher
Allow to register events and notify subscribers
<details><summary>Example</summary>

```c++
#include <ext/core/dispatcher.h>

// Example of event interface
struct IEvent : ext::events::IBaseEvent
{
	virtual void Event(int val) = 0;
};

// Example of sending an event:
ext::send_event(&IEvent::Event, 10);

// Example of recipient:
struct Recipient : ext::events::ScopeSubscription<IEvent>
{
	void Event(int val) override { std::cout << "Event"; }
}
```

</details>

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/core/dispatcher.h)

# Tracer

Show traces with defferent levels and time stamps in cout/cerr/output/trace file

<details><summary>Details</summary>
Don`t forget enable traces via 

```C++
#include <ext/core/tracer.h>
ext::get_tracer().Enable(true);
```
Simple macroses:
Default information trace

	EXT_TRACE() << "My trace"; 

Debug information only for Debug build

	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION "called";
	
Error trace to cerr, mostly used in EXT_CHECK/EXT_EXPECT

	EXT_TRACE_ERR() << EXT_TRACE_FUNCTION "called";
	
Can be called for scope call function check. Trace start and end scope with the given text

	EXT_TRACE_SCOPE() << EXT_TRACE_FUNCTION << "Main called with " << args;

</details>

# Other

- [Thread safe singleton with lifetime check](https://github.com/Pennywise007/ext/blob/main/include/ext/core/singleton.h)
- [Extension for tuples/variants and types array](https://github.com/Pennywise007/ext/blob/main/include/ext/core/mpl.h)