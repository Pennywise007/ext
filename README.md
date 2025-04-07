<div align="center">
  <img src="https://github.com/user-attachments/assets/c27e7ac4-7453-498a-90ea-dfab4a430a43" alt="Ext logo">
</div>

Developed and maintained a comprehensive header-only C++ library, EXT designed to enhance productivity and flexibility in mode C++17 and later standards.

# Build

<details><summary>Bazel build and run tests</summary>

```ps
bazel build //...
bazel test //...
```

</details>

<details><summary>CMake build and run tests</summary>

```ps
cmake -B build -DEXT_BUILD_TESTS=ON
cmake --build build --parallel
# On windows
.\build\tests\Debug\ext_tests.exe
# On linux
./build/tests/ext_tests
```

</details>

# Dependency injection

Usage simple with .Net [Microsoft.Extensions.DependencyInjection](https://www.nuget.org/packages/Microsoft.Extensions.DependencyInjection/)

<details><summary>Example</summary>

```c++

#include <ext/core/dependency_injection.h>

struct SomeInterface
{
    virtual ~SomeInterface() = default;
};

struct InterfaceImplementation : SomeInterface
{};

struct Object
{
    explicit Object(std::shared_ptr<SomeInterface> interface)
        : m_interface(std::move(interface))
    {}

    std::shared_ptr<SomeInterface> m_interface;
};

ext::ServiceCollection& serviceCollection = ext::get_singleton<ext::ServiceCollection>();
serviceCollection.RegisterScoped<InterfaceImplementationExample, InterfaceExample>();
// Register other classes
auto serviceProvider = serviceCollection.BuildServiceProvider();

std::shared_ptr<Object> object = ext::CreateObject<Object>(serviceProvider);
```

</details>

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/core/dependency_injection.h)
- [Tests and examples](https://github.com/Pennywise007/ext/blob/main/tests/core/dependency_injection_test.cpp)

# Reflection

Support of the compile time reflection in C++, getting object fields, functions

<details><summary>Object</summary>

```c++
struct TestStruct
{
    int intField;
    bool booleanField;
    std::string_view charArrayField;

    void existingFunction(int) {}
};

// Checking the brace constructor size(basically the fields count)
ext::reflection::brace_constructor_size<TestStruct> == 3;

// Fields iteration
constexpr auto kGlobalObj = TestStruct{ 100, true, "test"};
std::get<0>(ext::reflection::get_object_fields(kGlobalObj)) == 100
std::get<1>(ext::reflection::get_object_fields(kGlobalObj)) == true
std::get<2>(ext::reflection::get_object_fields(kGlobalObj)) == "test"

// Getting field names(C++20 or later)
#if C++20
ext::reflection::get_field_name<decltype(kGlobalObj), 0> == "intField"
ext::reflection::get_field_name<TestStruct, 1> == "booleanField"
ext::reflection::get_field_name<TestStruct, 2> == "charArrayField"
#endif

// Checking if object has some field
HAS_FIELD(TestStruct, booleanField) = true;
HAS_FIELD(TestStruct, unknown) = false;

// Checking if object has some function
HAS_FUNCTION(TestStruct, existingFunction) == true;
!HAS_FUNCTION(TestStruct, unknownFunction) == false;

// Real usage of the reflection
template <typename T>
void serializeObject(T& object)
{
    if constexpr (HAS_FUNCTION(T, onSerializationStart))
        object.onSerializationStart();
    // ...
    if constexpr (HAS_FUNCTION(T, onSerializationEnd))
        object.onSerializationEnd();
}
```

</details>

<details><summary>Enums</summary>

```c++
#include <ext/reflection/enum.h>

enum class TestEnum
{
    eEnumValue1,
    eEnumValue2,
    eEnumValue5 = 5,
};

// Enum to string
ext::reflection::enum_name<TestEnum(0)>() == "TestEnum::eEnumValue1";
ext::reflection::enum_name<TestEnum::eEnumValue5>() == "TestEnum::eEnumValue5";

// Enum size
ext::reflection::get_enum_size<TestEnum>() == 3;

// Getting enum value by index
ext::reflection::get_enum_value<TestEnum, 0>() == TestEnum::eEnumValue1
ext::reflection::get_enum_value<TestEnum, 1>() == TestEnum::eEnumValue2
ext::reflection::get_enum_value<TestEnum, 2>() == TestEnum::eEnumValue5

// Compilation time checks
switch (TestEnum)
{
case TestEnum::eEnumValue1: // ...
case TestEnum::eEnumValue2: // ...
case TestEnum::eEnumValue5: // ...
default: static_assert(ext::reflection::get_enum_size<TestEnum>() == 3, "Unhandled enum case state");
}

// Enum values iteration
for (TestEnum val : ext::reflection::get_enum_values<TestEnum>())
    // ...

EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(0));
EXPECT_TRUE(ext::reflection::is_enum_value<TestEnum>(TestEnum::eEnumValue2));
EXPECT_FALSE(ext::reflection::is_enum_value<TestEnum>(-1));
```

</details>

# Serialization

Serialization objects to/from json etc.

<details><summary>Example</summary>

```c++
#include <ext/serialization/iserializable.h>

using namespace ext::serializable;
using namespace ext::serializer;

#if C++20 // we use reflection to get fields info, no macro needed, to use base classes you need to use REGISTER_SERIALIZABLE_OBJECT
struct Settings
{
    struct User
    {
        std::int64_t id;
        std::string firstName;
        std::string userName;
    };
    
    std::wstring password;
    std::list<User> registeredUsers;
};

#else // not C++20

struct InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT();
    DECLARE_SERIALIZABLE_FIELD(long, value);
    DECLARE_SERIALIZABLE_FIELD(std::list<int>, valueList);
};

struct CustomValue : ISerializableValue {
// ISerializableValue
    [[nodiscard]] SerializableValue SerializeValue() const override { return std::to_wstring(val); }
    void DeserializeValue(const SerializableValue& value) override { val = std::wtoi(value); }
    int val = 10;
};

struct Setting : InternalStruct
{
    REGISTER_SERIALIZABLE_OBJECT(InternalStruct);

    DECLARE_SERIALIZABLE_FIELD(long, valueLong, 2);
    DECLARE_SERIALIZABLE_FIELD(int, valueInt);
    DECLARE_SERIALIZABLE_FIELD(std::vector<bool>, boolVector, { true, false });

    DECLARE_SERIALIZABLE_FIELD(CustomValue, value);
    DECLARE_SERIALIZABLE_FIELD(InternalStruct, internalStruct);

    // Instead of using macroses - use REGISTER_SERIALIZABLE_FIELD in constructor
    std::list<int> m_listOfParams;

    MyTestStruct()
    {
        REGISTER_SERIALIZABLE_FIELD(m_listOfParams); // or use DECLARE_SERIALIZABLE_FIELD macro
    }
};

#endif

Settings settings;

std::wstring json;
try {
    SerializeToJson(settings, json);
}
catch (...) {
    ext::ManageException(EXT_TRACE_FUNCTION);
}
...
try {
    DeserializeFromJson(settings, json);
}
catch (...) {
    ext::ManageException(EXT_TRACE_FUNCTION);
}

```

You can also declare this functions in your REGISTER_SERIALIZABLE_OBJECT object to get notified when (de)serialization was called:

// Called before object serialization
void OnSerializationStart() {}
// Called after object serialization
void OnSerializationEnd() {};

// Called before deserializing object, allow to change deserializable tree and avoid unexpected data, allows to add upgraders for outdated settings
// Also used to allocate collections elements
void OnDeserializationStart(SerializableNode& serializableTree) {}
// Called after collection deserialization
void OnDeserializationEnd() {};

</details>

- [Source](https://github.com/Pennywise007/ext/tree/main/include/ext/serialization)

Tests and examples:
- [C++ 20 tests and examples](https://github.com/Pennywise007/ext/blob/main/tests/serialization/serialization_c++20_test.cpp)
- [C++ 17 tests and examples](https://github.com/Pennywise007/ext/blob/main/tests/serialization/serialization_test.cpp)

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
- [Tests](https://github.com/Pennywise007/ext/blob/main/tests/thread/thread_test.cpp)

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
- [Tests](https://github.com/Pennywise007/ext/blob/main/tests/thread/thread_pool_test.cppp)

</details>


## And others

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
#include <ext/core/tracer.h>
ext::get_tracer().Enable();
```

Simple macroses:

- Default information trace `EXT_TRACE() << "My trace";`
- Debug information only for Debug build `EXT_TRACE_DBG() << EXT_TRACE_FUNCTION "called";`
- Error trace to cerr, mostly used in EXT_CHECK/EXT_EXPECT `EXT_TRACE_ERR() << EXT_TRACE_FUNCTION "called";`
- Can be called for scope call function check. Trace start and end scope with the given text `EXT_TRACE_SCOPE() << EXT_TRACE_FUNCTION << "Main function called with " << args;`

- [Source](https://github.com/Pennywise007/ext/blob/main/include/ext/core/tracer.h)

</details>

# Check code execution and handling errors

<details><summary>Allows to add simple checks inside executing code and manage exceptions</summary>

```c++
#include <ext/core/check.h>
```

**EXT_CHECK** - throws exception if expression is false

**EXT_CHECK**(bool_expression) << "Text";

```c++
if (!bool_expression)
	throw ::ext::check::CheckFailedException(std::source_location::current(), #bool_expression "Text");
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
	throw ::ext::check::CheckFailedException(std::source_location::current(), #bool_expression "Text"));
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
		std::throw_with_nested(ext::exception(std::source_location::current(), "Job failed")); 
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

# Compile time classes

<details><summary>Constexpr string</summary>

Allows to combine and check text in compile time.

```c++
#include <ext/constexpr/string.h>

constexpr ext::constexpr_string textFirst = "test";
constexpr ext::constexpr_string textSecond = "second";

constexpr auto TextCombination = textFirst + "_" + textSecond;
static_assert(TextCombination == "test_second");
```

In C++20 can be used to store text as a template argument:

```c++
    template <ext::constexpr_string name__>
    struct Object {
        constexpr std::string_view Name() const {
            return name__.str();
        }
        ...
    };

    Object<"object_name"> object;
    static_assert(object.Name() == std::string_view("object_name"));
```

[Source](https://github.com/Pennywise007/ext/blob/main/include/ext/constexpr/string.h)
</details>

<details><summary>Constexpr map</summary>

Compile time extension for strings, allow to combine and check text in compile time.

```c++
#include <ext/constexpr/map.h>

constexpr ext::constexpr_map my_map = {std::pair{11, 10}, {std::pair{22, 33}}};
static_assert(my_map.size() == 2);

static_assert(10 == my_map.get_value(11));
static_assert(33 == my_map.get_value(22));
```

[Source](https://github.com/Pennywise007/ext/blob/main/include/ext/constexpr/map.h)
</details>

# Std extensions

## Memory

- [lazy_shared_ptr](https://github.com/Pennywise007/ext/blob/main/include/ext/std/memory.h#L56C8-L56C23) - allow to create a shared memory which will be created only on first call

## Strings

- [Trim all](https://github.com/Pennywise007/ext/blob/main/include/ext/std/string.h#L19)
- [String/wstring converters](https://github.com/Pennywise007/ext/blob/main/include/ext/std/string.h#L36)
- [String format](https://github.com/Pennywise007/ext/blob/main/include/ext/std/string.h#L116C27-L116C41)

## Filesystem

- [Full exe path](https://github.com/Pennywise007/ext/blob/main/include/ext/std/filesystem.h#L17C44-L17C61)

# Other

- [Call once (GO analog)](https://github.com/Pennywise007/ext/blob/main/include/ext/utils/call_once.h#L23)
- [Thread safe singleton with lifetime check](https://github.com/Pennywise007/ext/blob/main/include/ext/core/singleton.h)
- [Extension for tuples/variants and types array](https://github.com/Pennywise007/ext/blob/main/include/ext/core/mpl.h)
- [Auto setter on scope change](https://github.com/Pennywise007/ext/blob/main/include/ext/scope/auto_setter.h)
- [Defer (GO analog)](https://github.com/Pennywise007/ext/blob/main/include/ext/scope/defer.h)
- [Object holder](https://github.com/Pennywise007/ext/blob/main/include/ext/scope/on_exit.h#L70)
