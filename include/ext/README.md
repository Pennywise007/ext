# ext library
Header only C++17 extensions library

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

ext::ServiceCollection& serviceCollection = ext::get_service<ext::ServiceCollection>();
serviceCollection.RegisterScoped<InterfaceImplementationExample, InterfaceExample>();

const std::shared_ptr<CreatedObjectExample> object = ext::CreateObject<CreatedObjectExample>(serviceCollection.BuildServiceProvider());

```

</details>

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/core/dependency_injection.h)
- [Tests and examples](https://github.com/Pennywise007/ext/blob/main/test/core/dependency_injection_test.cpp)

# Serialization
Serialization objects to/from text, xml
<details><summary>Example</summary>

```c++

#include <ext/serialization/iserializable.h>

using namespace ext::serializable;

struct InternalStruct : SerializableObject<InternalStruct, "Pretty name">
{
    DECLARE_SERIALIZABLE_FIELD((long) value);
    DECLARE_SERIALIZABLE_FIELD((std::list<int>) valueList);
};

struct TestStruct : SerializableObject<TestStruct>
{
    REGISTER_SERIALIZABLE_BASE(InternalStruct);

    DECLARE_SERIALIZABLE_FIELD(long, valueLong, 2);
    DECLARE_SERIALIZABLE_FIELD(int, valueInt);
    DECLARE_SERIALIZABLE_FIELD(std::vector<bool>, boolVector, { true, false });

    DECLARE_SERIALIZABLE_FIELD(CustomField, field);
    DECLARE_SERIALIZABLE_FIELD(InternalStruct, internalStruct);

    std::list<int> m_listOfParams;

    MyTestStruct()
    {
        REGISTER_SERIALIZABLE_FIELD(m_listOfParams); // or use DECLARE_SERIALIZABLE macro

        Executor::DeserializeObject(Factory::TextDeserializer(L"C:\\Test.xml"), testStruct);
    }

    ~MyTestStruct()
    {
        Executor::SerializeObject(Factory::TextSerializer(L"C:\\Test.xml"), testStruct);
    }
};

```
</details>

- [Source](https://github.com/Pennywise007/ext/tree/main/include/ext/serialization)
- [Tests and examples](https://github.com/Pennywise007/ext/blob/main/test/serialization/serialization_test.cpp)

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

# Threading
<details><summary>Interruptible thread(boost/thread analog)</summary>

```c++
#include <ext/thread/thread.h>

ext::thread myThread(thread_function, []()
{
	while (!ext::this_thread::interruption_requested())
	{
		try
		{
			...
		}
		catch (const ext::thread::thread_interrupted&)
		{
			break;
		}
	}
});

myThread.interrupt();
EXPECT_TRUE(myThread.interrupted());
```

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/thread.h)
- [Tests](https://github.com/Pennywise007/ext/blob/main/test/thread/thread_test.cpp)

</details>

<details><summary>Thread pool</summary>

```c++
#include <ext/thread/thread_pool.h>

std::set<ext::task::TaskId, ext::task::TaskIdComparer> taskList;
ext::thread_pool threadPool([&taskList, &listMutex](const ext::task::TaskId& taskId)
{
	taskList.erase(taskId);
});

const auto maxThreads = std::thread::hardware_concurrency();
for (auto i = maxThreads; i != 0; --i)
{
	taskList.emplace(threadPool.add_task([]()
	{
		...
	}));
}
threadPool.wait_for_tasks();
```

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/thread_pool.h)
- [Tests](https://github.com/Pennywise007/ext/blob/main/test/thread/thread_pool_test.cppp)

</details>


## And others:

- [Task scheduler](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/scheduler.h)
- [Main thread methods invoker(for GUI and other synchronized actions)](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/invoker.h)
- [Event](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/event.h)
- [Tick timer, allow to synchronize sth(for example animations)](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/tick.h)
- [Wait group(GO analog)](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/wait_group.h)
- [Channel(GO analog)](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/channel.h)

```c++
ext::Channel<int> channel;

std::thread([&]()
    {
        for (auto val : channel) {
            ...
        }
    });
channel.add(1);
channel.add(10);
channel.close();
```
- [C++17 stop token](https://github.com/Pennywise007/ext/blob/main/include/ext/utils/stop_token_details.h)
```c++
ext::stop_source source;
ext::thread myThread([stop_token = source.get_token()]()
{
    while (!stop_token.stop_requested())
    {
        ...
    }
});

source.request_stop();
myThread.join();
```
# Tracer
<details><summary>Show traces with defferent levels and time stamps in cout/cerr/output/trace file</summary>

```c++
#include <ext/traces/tracer.h>
ext::get_tracer()->EnableTraces(true);
```

Simple macroses:
Default information trace

`	EXT_TRACE() << "My trace";`

Debug information only for Debug build

`	EXT_TRACE_DBG() << EXT_TRACE_FUNCTION "called";`
	
Error trace to cerr, mostly used in EXT_CHECK/EXT_EXPECT

`	EXT_TRACE_ERR() << EXT_TRACE_FUNCTION "called";`
	
Can be called for scope call function check. Trace start and end scope with the given text

`	EXT_TRACE_SCOPE() << EXT_TRACE_FUNCTION << "Main function called with " << args;`

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/trace/itracer.h)

</details>

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

# Managing exceptions
<details><summary>Allow to simplify managing exceptions and output error text</summary>

```c++
#include <ext/error/exception.h>

try
{ 
	EXT_EXPECT(is_ok()) << "Something wrong!";
}
catch (...)
{	
	try
	{
		std::throw_with_nested(ext::exception(EXT_SRC_LOCATION, "Job failed")); 
	}
	catch (...)
	{
		::MessageBox(NULL, ext::ManageExceptionText("Big bang"));
	}
}
```

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/error/exception.h)

</details>

# Dumper and debugging

<details><summary>Allow to catch unhandled exceptions and generate dump file</summary>

Declare unhandled exceptions handler(called automatic on calling ext::dump::create_dump())
```c++
#include <ext/error/dump_writer.h>

void main()
{
	EXT_DUMP_DECLARE_HANDLER();
	...
}
```
	
If you need to catch error inside you code you add check:
```c++
EXT_DUMP_IF(is_something_wrong());
``` 
In this case if debugger presents - it will be stopped here, otherwise generate dump file and **continue** execution, @see DEBUG_BREAK_OR_CREATE_DUMP.
Dump generation and debug break in case with EXT_DUMP_IF generates only once to avoid spam.

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/error/dump_writer.h)

</details>

# Std extensions
## Memory:
- [lazy_shared_ptr](https://github.com/Pennywise007/ext/blob/main/include/ext/std/memory.h#L56C8-L56C23) - allow to create a shared memory which will be created only on first call

## Strings:
- [Trim all](https://github.com/Pennywise007/ext/blob/main/include/ext/std/string.h#L19)
- [String/wstring converters](https://github.com/Pennywise007/ext/blob/main/include/ext/std/string.h#L36)
- [String format](https://github.com/Pennywise007/ext/blob/main/include/ext/std/string.h#L116C27-L116C41)

## Filesystem:
- [Full exe path](https://github.com/Pennywise007/ext/blob/main/include/ext/std/filesystem.h#L17C44-L17C61)

# Other
- [Call once(GO analog)](https://github.com/Pennywise007/ext/blob/main/include/ext/utils/call_once.h#L23)
- [Thread safe singleton with lifetime check](https://github.com/Pennywise007/ext/blob/main/include/ext/core/singleton.h)
- [Extension for tuples/variants and types array](https://github.com/Pennywise007/ext/blob/main/include/ext/core/mpl.h)
- [Auto setter on scope change](https://github.com/Pennywise007/ext/blob/main/include/ext/scope/auto_setter.h)
- [Defer (GO analog)](https://github.com/Pennywise007/ext/blob/main/include/ext/scope/defer.h)
- [Object holder](https://github.com/Pennywise007/ext/blob/main/include/ext/scope/on_exit.h#L70)