#include "gtest/gtest.h"

#include <ext/thread/event.h>
#include <ext/thread/thread.h>
#include <ext/thread/stop_token.h>

namespace {

std::atomic_bool ThreadExecuted(false);
std::atomic_bool ThreadInterrupted(false);
std::atomic_bool ThreadRun(false);

void thread_function(const std::function<void()>& function)
{
    ThreadRun = true;

    ThreadExecuted = false;
    ThreadInterrupted = false;
    try
    {
        function();

        ThreadExecuted = true;
    }
    catch (const ext::thread::thread_interrupted&)
    {
        ThreadInterrupted = true;
    }
}

void join_thread_and_check(ext::thread& thread, bool interrupted)
{
    thread.join();
    EXPECT_TRUE(ThreadRun);

    if (interrupted)
    {
        EXPECT_FALSE(ThreadExecuted);
        EXPECT_TRUE(ThreadInterrupted);
    }
    else
    {
        EXPECT_TRUE(ThreadExecuted);
        EXPECT_FALSE(ThreadInterrupted);
    }

    ThreadRun = false;
    ThreadExecuted = false;
    ThreadInterrupted = false;
}

} // namespace

TEST(thread_test, check_reference)
{
    int value = 0;
    ext::thread myThread([](int& val)
                         {
                            val = 5;
                         }, std::ref(value));
    myThread.join();
    EXPECT_EQ(value, 5);
}

TEST(thread_test, arguments_copying_test)
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

    ext::thread myThread(moveFunction, Counter{});
    myThread.join();

    auto copyFunction = [](const Counter& counter) {
        EXPECT_EQ(counter.m_moves, 0);
        EXPECT_EQ(counter.m_copies, 0);
    };
    Counter counter;
    myThread.run(copyFunction, std::cref(counter));
    myThread.join();
}

TEST(thread_test, non_copyable_arguments)
{
    auto function = [](std::unique_ptr<int>&& val) {
        EXPECT_EQ(*val, 100);
    };

    ext::thread myThread(function, std::make_unique<int>(100));
    myThread.join();
}

TEST(thread_test, test_passing_parameters)
{
    bool run = false;
    ext::thread myThread([&run](int value, std::string val)
                         {
                             EXPECT_EQ(12, value);
                             EXPECT_STREQ("test", val.c_str());
                             run = true;
                         }, 12, "test");
    myThread.join();
    EXPECT_TRUE(run);

    run = false;
    myThread.run([&run](int value, std::string val)
                 {
                     EXPECT_EQ(12, value);
                     EXPECT_STREQ("test", val.c_str());
                     run = true;
                 }, 12, "test");
    myThread.join();
    EXPECT_TRUE(run);
}

TEST(thread_test, check_interruption_requested)
{
    {
        ext::thread myThread(thread_function, []()
        {
            while (!ext::this_thread::interruption_requested())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        myThread.interrupt();
        join_thread_and_check(myThread, false);
    }
    {
        ext::thread myThread(thread_function, []()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            EXPECT_FALSE(ext::this_thread::interruption_requested());
        });
        join_thread_and_check(myThread, false);
    }
    {
        ext::thread myThread(thread_function, []()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            EXPECT_TRUE(ext::this_thread::interruption_requested());
        });
        myThread.interrupt();
        join_thread_and_check(myThread, false);
    }
}

TEST(thread_test, check_interruption_point)
{
    {
        ext::thread myThread(thread_function, []()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ext::this_thread::interruption_point();
        });
        myThread.interrupt();
        join_thread_and_check(myThread, true);
    }
    {
        ext::thread myThread(thread_function, []()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ext::this_thread::interruption_point();
        });
        join_thread_and_check(myThread, false);
    }
}

TEST(thread_test, check_detaching)
{
    ext::Event threadInterruptedAndDetached;
    std::atomic_bool interrupted = false;
    ext::thread myThread([&threadInterruptedAndDetached, &interrupted]()
    {
        EXPECT_TRUE(threadInterruptedAndDetached.Wait());
        try
        {
            ext::this_thread::interruption_point();
        }
        catch (const ext::thread::thread_interrupted&)
        {
            interrupted = true;
        }
    });
    myThread.interrupt();
    myThread.detach();
    threadInterruptedAndDetached.Set();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(interrupted);
}

TEST(thread_test, check_ordinary_sleep)
{
    ext::thread myThread(thread_function, []() { ext::this_thread::sleep_for(std::chrono::milliseconds(300)); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    myThread.interrupt();
    join_thread_and_check(myThread, false);
}

TEST(thread_test, check_interruption_sleeping)
{
    ext::Event threadStartedEvent;
    ext::thread myThread(thread_function, [&]()
                         {
                             threadStartedEvent.Set();
                             ext::this_thread::interruptible_sleep_for(std::chrono::seconds(1));
                         });
    EXPECT_TRUE(threadStartedEvent.Wait());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    myThread.interrupt();
    join_thread_and_check(myThread, true);
}

TEST(thread_test, check_run_and_interrupting_sleep)
{
    ext::thread myThread(thread_function, []() { 
        ext::this_thread::interruptible_sleep_for(std::chrono::milliseconds(100));
    });
    join_thread_and_check(myThread, false);

    myThread.run(thread_function, []() {
        ext::this_thread::interruptible_sleep_for(std::chrono::milliseconds(100));
    });
    join_thread_and_check(myThread, false);
}

TEST(thread_test, check_stop_token)
{
    ext::Event threadStartedEvent;
    ext::thread myThread(thread_function, [&threadStartedEvent]()
                         {
                             const auto token = ext::this_thread::get_stop_token();
                             while (!token.stop_requested())
                             {
                                 threadStartedEvent.Set();
                                 std::this_thread::yield();
                             }
                         });
    EXPECT_TRUE(threadStartedEvent.Wait());
    myThread.interrupt();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    join_thread_and_check(myThread, false);
}
