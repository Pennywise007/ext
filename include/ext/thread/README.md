# Thread and synchronization utilities
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
- [Wait group(GO analog)](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/wait_group.h)
- [Tick timer, allow to synchronize sth(for example animations)](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/tick.h)
- [Task scheduler](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/scheduler.h)
- [Main thread methods invoker(for GUI and other synchronized actions)](https://github.com/Pennywise007/ext/blob/main/include/ext/thread/invoker.h)