#include "gtest/gtest.h"

#include <thread>

#include <ext/scope/defer.h>
#include <ext/thread/channel.h>
#include <ext/thread/wait_group.h>

TEST(channel_test, check_channel_set_get)
{
    ext::Channel<int> channel;
    channel.add(1);
    const auto res = channel.get();
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(1, res.value());
}

TEST(channel_test, check_channel_loop)
{
    ext::Channel<int> channel(3);
    channel.add(1);
    channel.add(2);
    channel.add(3);

    channel.close();
    int element = 1;
    for (int val : channel) {
        EXPECT_EQ(element++, val);
    }
}

TEST(channel_test, check_channel_closing)
{
    ext::Channel<int> channel(3);
    channel.add(10);

    bool working = true;
    auto thread = std::thread([&]()
        {
            auto calls = 0;
            for (auto val : channel) {
                EXPECT_EQ(10, val);
                calls++;
            }
            EXPECT_EQ(5, calls);
            working = false;
        });

    channel.add(10);
    channel.add(10);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    EXPECT_TRUE(working);

    channel.add(10);
    channel.add(10);
    channel.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(working);

    thread.join();
}

TEST(channel_test, check_multithreading)
{
    ext::Channel<int> channel;

    ext::WaitGroup wg;
    wg.add();
    auto thread = std::thread([&]()
        {
            EXT_DEFER({ wg.done(); });

            EXPECT_EQ(1, *channel.get());
            EXPECT_EQ(2, *channel.get());
            EXPECT_EQ(3, *channel.get());
        });

    channel.add(1);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    channel.add(2);
    channel.add(3);

    wg.done();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    thread.join();
}

TEST(channel_test, check_throwing_exceptions)
{
    ext::Channel<int> channel;
    auto iter = channel.end();
    EXPECT_THROW(++iter, std::bad_function_call);

    channel.close();
    EXPECT_THROW(channel.add(), std::bad_function_call);
}

