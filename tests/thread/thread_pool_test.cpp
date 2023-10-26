#include "gtest/gtest.h"

#include <ext/thread/event.h>
#include <ext/thread/thread_pool.h>

TEST(thread_pool_test, add_task)
{
    std::atomic_uint onDoneCalls = 0;

    ext::thread_pool threadPool([&](const ext::thread_pool::TaskId&)
    {
        ++onDoneCalls;
    });

    std::atomic_uint taskExecuted = 0;
    threadPool.add_task([&](){
        ++taskExecuted;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    threadPool.wait_for_tasks();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_EQ(onDoneCalls, 1);
    EXPECT_EQ(taskExecuted, 1);
}

TEST(thread_pool_test, check_adding_and_executing_tasks)
{
    std::mutex listMutex;
    std::set<ext::thread_pool::TaskId> taskList;

    std::atomic_int runTaskCount = 0;

    ext::thread_pool threadPool([&taskList, &listMutex](const ext::thread_pool::TaskId& taskId)
    {
        std::scoped_lock lock(listMutex);
        EXPECT_NE(taskList.find(taskId), taskList.end()) << "Unknown task id";
        taskList.erase(taskId);
    });

    {
        std::scoped_lock lock(listMutex);
        const auto maxThreads = std::thread::hardware_concurrency();
        for (auto i = maxThreads; i != 0; --i)
        {
            EXPECT_TRUE(taskList.emplace(threadPool.add_task([&runTaskCount]()
            {
                ++runTaskCount;
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                --runTaskCount;
            }).first).second) << "Duplicate of task id";
        }
        EXPECT_EQ(taskList.size(), maxThreads);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_EQ(runTaskCount, maxThreads);
    }

    threadPool.wait_for_tasks();
    EXPECT_TRUE(taskList.empty()) << "not all tasks finish callbacks called";
    EXPECT_EQ(runTaskCount, 0) << "not all task executed";
}

TEST(thread_pool_test, parallel_execution)
{
    std::atomic_bool firstExecuting = false, secondExecuted = false;
    std::atomic_uint onDoneCalls = 0;

    ext::thread_pool threadPool([&onDoneCalls](const ext::thread_pool::TaskId& taskId)
    {
        ++onDoneCalls;
    }, 2);

    threadPool.add_task([&firstExecuting]() {
        firstExecuting = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        firstExecuting = false;
    });
    threadPool.add_task([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        EXPECT_TRUE(firstExecuting);
        secondExecuted = true;
    });
    threadPool.wait_for_tasks();

    EXPECT_TRUE(secondExecuted);
    EXPECT_FALSE(firstExecuting);
    EXPECT_EQ(onDoneCalls, 2);
}

TEST(thread_pool_test, waiting_for_tasks)
{
    ext::Event threadStart, doneStart;
    std::atomic_bool doneCalled = false, functionExecuted = false, erasedFunctionExecuted = false;
    ext::thread_pool threadPool([&](const ext::thread_pool::TaskId& taskId)
    {
        doneStart.RaiseAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        doneCalled = true;
    }, 1);

    threadPool.add_task([&]() {
        threadStart.RaiseAll();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        functionExecuted = true;
    });

    const auto erasedTask = [&erasedFunctionExecuted]() {
        erasedFunctionExecuted = true;
    };

    // erased immediately
    threadPool.erase_task(threadPool.add_task(erasedTask).first);

    threadStart.Wait();
    EXPECT_FALSE(functionExecuted);
    EXPECT_FALSE(doneCalled);
    // erased during first task execution
    threadPool.erase_task(threadPool.add_task(erasedTask).first);

    // erased during done callback execution
    doneStart.Wait();
    EXPECT_TRUE(functionExecuted);
    EXPECT_FALSE(doneCalled);
    threadPool.erase_task(threadPool.add_task(erasedTask).first);

    threadPool.wait_for_tasks();
    EXPECT_TRUE(functionExecuted);
    EXPECT_TRUE(doneCalled);
    EXPECT_FALSE(erasedFunctionExecuted);
}

TEST(thread_pool_test, check_removing_tasks_with_post_processing_delay)
{
    std::atomic_uint executedTasksCount = 0;
    ext::thread_pool threadPool([&executedTasksCount](const ext::thread_pool::TaskId& taskId)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++executedTasksCount;
    }, 1);

    for (int i = 0; i < 5; ++i)
    {
        const auto taskId = threadPool.add_task([](){}).first;
        if (i != 0)
            threadPool.erase_task(taskId);
    }
    threadPool.wait_for_tasks();
    EXPECT_EQ(executedTasksCount, 1);
}

TEST(thread_pool_test, check_removing_tasks_with_processing_delay)
{
    std::atomic_uint executedTasksCount = 0;
    ext::thread_pool threadPool([&executedTasksCount](const ext::thread_pool::TaskId& taskId)
    {
        ++executedTasksCount;
    }, 1);

    for (int i = 0; i < 5; ++i)
    {
        const auto taskId = threadPool.add_task([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }).first;
        if (i != 0)
            threadPool.erase_task(taskId);
    }
    threadPool.wait_for_tasks();
    EXPECT_EQ(executedTasksCount, 1);
}

TEST(thread_pool_test, interrupt_and_rerun)
{
    std::atomic_uint executedTasksCount = 0;
    ext::thread_pool threadPool([&executedTasksCount](auto)
                                {
                                    ++executedTasksCount;
                                }, 1);
    ext::Event threadStarted;
    const auto threadFunction = [&threadStarted]()
    {
        const auto token = ext::this_thread::get_stop_token();
        while (!token.stop_requested())
        {
            threadStarted.RaiseOne();
            ext::this_thread::yield();
        }
    };

    threadPool.add_task(threadFunction);

    EXPECT_TRUE(threadStarted.Wait());
    threadPool.interrupt_and_remove_all_tasks();
    EXPECT_EQ(1, executedTasksCount);

    threadStarted.Reset();
    threadPool.add_task(threadFunction);

    EXPECT_TRUE(threadStarted.Wait());
    threadPool.interrupt_and_remove_all_tasks();
    EXPECT_EQ(2, executedTasksCount);
}

TEST(thread_pool_test, arguments_copying_test)
{
    struct Counter {
        Counter() = default;
        Counter(Counter&& v) { m_moves = v.m_moves + 1; };
        Counter(const Counter& v) { m_copies = v.m_copies + 1; }

        Counter& operator=(const Counter& v) = delete;
        Counter& operator=(Counter&& v) = delete;

        size_t m_moves = 0;
        size_t m_copies = 0;
    };

    auto moveFunction = [](Counter&& counter) {
        EXPECT_EQ(counter.m_moves, 1);
        EXPECT_EQ(counter.m_copies, 0);
    };

    ext::thread_pool threadPool(1);
    threadPool.add_task(moveFunction, Counter{});
    threadPool.wait_for_tasks();

    auto copyFunction = [](const Counter& counter) {
        EXPECT_EQ(counter.m_moves, 1);
        EXPECT_EQ(counter.m_copies, 0);
    };
    threadPool.add_task(copyFunction, Counter{});
    threadPool.wait_for_tasks();
}

TEST(thread_pool_test, task_execution_priority)
{
    ext::thread_pool threadPool(1);

    std::atomic_int executedThreads = 0;
    auto task = [&executedThreads](int expectedSequence) {
        EXPECT_EQ(++executedThreads, expectedSequence);
        ext::this_thread::sleep_for(std::chrono::milliseconds(100));
    };

    threadPool.add_task(task, 1);
    threadPool.add_task(task, 3);

    // wait till thread pool gets a first task to execute
    ext::this_thread::sleep_for(std::chrono::milliseconds(50));
    threadPool.add_high_priority_task(task, 2);

    threadPool.wait_for_tasks();
    EXPECT_EQ(executedThreads, 3);
}

TEST(thread_pool_test, task_execution_high_priority)
{
    ext::thread_pool threadPool(1);

    std::atomic_int executedThreads = 0;
    auto task = [&executedThreads](int expectedSequence) {
        EXPECT_EQ(++executedThreads, expectedSequence);
        ext::this_thread::sleep_for(std::chrono::milliseconds(50));
    };

    threadPool.add_high_priority_task(task, 1);
    threadPool.add_high_priority_task(task, 2);
    threadPool.add_task(task, 4);
    threadPool.add_high_priority_task(task, 3);

    threadPool.wait_for_tasks();
    EXPECT_EQ(executedThreads, 4);
}
