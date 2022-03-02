# ext library
Header only C++17 extensions library

Test project [ext_test](https://github.com/Pennywise007/ext_test) 

```c++
test
```

```c#
test2
```

```c#
	test2
```

	```c#
		test3
	```

# Dependency injection
Usage simple with .Net [Microsoft.Extensions.DependencyInjection](https://www.nuget.org/packages/Microsoft.Extensions.DependencyInjection/)
<details><summary>Example</summary>
    ```c++

    #include <ext/core/dependency_injection.h>
    
    
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
    
    ```

</details>
- [Source](https://github.com/Pennywise007/ext/blob/main/core/dependency_injection.h)
- [Tests](https://github.com/Pennywise007/ext_test/blob/main/ext_test/Tests/DependencyInjection.cpp)

# Serialization
Serialization struct to/from text, xml and others(in progress)
<details><summary>Example</summary>
```c++

    #include <ext/serialization/iserializable.h>
    
    using namespace ext::serializable;
    
    struct InternalStruct : SerializableObject<InternalStruct, L"Pretty name">
    {
        DECLARE_SERIALIZABLE((long) value);
        DECLARE_SERIALIZABLE((std::list<int>) valueList);
    };
    
    struct TestStruct :  SerializableObject<TestStruct>
    {
        REGISTER_SERIALIZABLE_BASE(InternalStruct);
    
        DECLARE_SERIALIZABLE((long) valueLong, 2);
        DECLARE_SERIALIZABLE((int) valueInt);
        DECLARE_SERIALIZABLE((std::vector<bool>) boolVector, { true, false });
    
        DECLARE_SERIALIZABLE((CustomField) field);
        DECLARE_SERIALIZABLE((InternalStruct) internalStruct);
    
        std::list<int> m_listOfParams;
    
        MyTestStruct()
        {
            REGISTER_SERIALIZABLE_OBJECT(m_listOfParams); // or use DECLARE_SERIALIZABLE macro
    
            Executor::DeserializeObject(Fabric::TextDeserializer(L"C:\\Test.xml"), testStruct);
        }
    
        ~MyTestStruct()
        {
            Executor::SerializeObject(Fabric::TextSerializer(L"C:\\Test.xml"), testStruct);
        }
    };

```
</details>
- [Source](https://github.com/Pennywise007/ext/tree/main/serialization)
- [Tests](https://github.com/Pennywise007/ext_test/blob/main/ext_test/Tests/Serialization.cpp)

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
- [Source](https://github.com/Pennywise007/ext/blob/main/core/dispatcher.h)

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
- [Source](https://github.com/Pennywise007/ext/blob/main/thread/thread.h)
- [Tests](https://github.com/Pennywise007/ext_test/blob/main/ext_test/Tests/Threads.cpp)
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
- [Source](https://github.com/Pennywise007/ext/blob/main/thread/thread_pool.h)
- [Tests](https://github.com/Pennywise007/ext_test/blob/main/ext_test/Tests/ThreadPool.cpp)
</details>

And others:

- [Task scheduller](https://github.com/Pennywise007/ext/blob/main/thread/scheduller.h)
- [Main thread methods invoker(for GUI and other synchronized actions)](https://github.com/Pennywise007/ext/blob/main/thread/invoker.h)
- [Events](https://github.com/Pennywise007/ext/blob/main/thread/event.h)
- [Tick timer, allow to synchronize sth(for example animations)](https://github.com/Pennywise007/ext/blob/main/thread/tick.h)

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

- [Source](https://github.com/Pennywise007/ext/tree/main/trace)
</details>

# Check code execution and handling errors
<details><summary>Allow to add simple checks inside executing code and manage ezxceptions</summary>

```c++
    #include <ext/core/check.h>
```
**EXT_CHECK** - throws exception if expression is false
EXT_CHECK(bool_expression) << "Text";
```c++
    if (!bool_expression)
    	throw ::ext::check::CheckFailedException(EXT_SRC_LOCATION, #bool_expression "Text"));
```

**EXT_EXPECT** - if expression is false:
- Only on first failure: debug break if debugger presents, create dump otherwise
- throws exception

EXT_EXPECT(bool_expression) << "Text";
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
EXT_ASSERT(bool_expression) << "Text";
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
- [Source](https://github.com/Pennywise007/ext/blob/main/core/check.h)
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

- [Source](https://github.com/Pennywise007/ext/blob/main/error/exception.h)
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

- [Source](https://github.com/Pennywise007/ext/blob/main/error/dump_writer.h)
</details>