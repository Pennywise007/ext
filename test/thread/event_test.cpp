#include "gtest/gtest.h"

#include <ext/thread/event.h>

TEST(event_test, check_rising)
{
    ext::Event event;
    std::thread myThread([&event]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        event.Set();
    });
    EXPECT_TRUE(event.Wait(std::chrono::seconds(2)));

    std::thread myThread2([&event]()
    {
        event.Set();
    });

    myThread2.join();
    EXPECT_TRUE(event.Wait(std::chrono::seconds(0)));

    myThread.join();
}

TEST(event_test, check_wait_after_set)
{
    ext::Event event;
    event.Set();

    EXPECT_TRUE(event.Wait());
}

TEST(event_test, check_wait_after_set_in_another_thread)
{
    ext::Event event;
    std::thread myThread([&event]()
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        event.Set();
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
    event.Set();
    
    EXPECT_TRUE(event.Wait());
    EXPECT_TRUE(event.Raised());

    EXPECT_TRUE(event.Wait());

    event.Reset();
    EXPECT_FALSE(event.Raised());

    EXPECT_FALSE(event.Wait(std::chrono::milliseconds(10)));

    event.Set();
    EXPECT_TRUE(event.Wait());
}
