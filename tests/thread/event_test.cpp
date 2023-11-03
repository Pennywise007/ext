#include "gtest/gtest.h"

#include <atomic>
#include <algorithm>
#include <functional>
#include <thread>
#include <list>

#include <ext/thread/event.h>

TEST(event_test, check_rising)
{
    ext::Event event;
    std::thread myThread([&event]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        event.RaiseOne();
    });
    EXPECT_TRUE(event.Wait(std::chrono::milliseconds(200)));

    std::thread myThread2([&event]()
    {
        event.RaiseOne();
    });

    myThread2.join();
    EXPECT_TRUE(event.Wait(std::chrono::seconds(0)));

    myThread.join();
}

TEST(event_test, check_wait_after_set)
{
    ext::Event event;
    event.RaiseOne();

    EXPECT_TRUE(event.Wait());
}

TEST(event_test, check_wait_after_set_in_another_thread)
{
    ext::Event event;
    std::thread myThread([&event]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        event.RaiseOne();
    });
    EXPECT_TRUE(event.Wait(std::chrono::milliseconds(150)));
    myThread.join();
}

TEST(event_test, check_timeout)
{
    ext::Event event;
    EXPECT_FALSE(event.Wait(std::chrono::milliseconds(10)));
}

TEST(event_test, check_reset)
{
    ext::Event event;
    event.RaiseOne();
    EXPECT_TRUE(event.Raised());
    EXPECT_TRUE(event.Wait(std::chrono::milliseconds(0)));
    EXPECT_FALSE(event.Raised());
    EXPECT_FALSE(event.Wait(std::chrono::milliseconds(0)));

    event.RaiseOne();
    EXPECT_TRUE(event.Raised());
    event.Reset();
    EXPECT_FALSE(event.Raised());

    EXPECT_FALSE(event.Wait(std::chrono::milliseconds(10)));

    event.RaiseOne();
    EXPECT_TRUE(event.Wait());
}

TEST(event_test, multy_notification)
{
    ext::Event event;
    std::atomic_int doneThreads = 0;

    auto func = [&]() {
        event.Wait();
        ++doneThreads;
    };

    size_t threads_count = 25;
    std::list<std::thread> threads;
    for (size_t i = 0; i < threads_count; ++i) {
        threads.emplace_back(func);
    }

    event.RaiseOne();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(1, doneThreads);

    event.RaiseAll();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(threads_count, doneThreads);

    std::for_each(threads.begin(), threads.end(), std::mem_fn(&std::thread::join));
}
