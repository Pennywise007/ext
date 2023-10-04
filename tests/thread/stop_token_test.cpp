#include "gtest/gtest.h"

#include <ext/thread/event.h>
#include <ext/thread/thread.h>
#include <ext/thread/stop_token.h>

TEST(stop_token_test, check_stop)
{
    ext::stop_source source;

    ext::Event threadStartedEvent;

    ext::thread myThread([&, stop_token = source.get_token()]()
    {
        while (!stop_token.stop_requested())
        {
            threadStartedEvent.Set();
        }
    });

    EXPECT_TRUE(threadStartedEvent.Wait());
    EXPECT_TRUE(source.request_stop());

    myThread.join();
}

TEST(stop_token_test, check_callback)
{
    ext::stop_source source;

    ext::Event threadStartedEvent;

    std::atomic_bool callbackCalled = false;
    ext::thread myThread([&, stop_token = source.get_token()]()
    {
        ext::stop_callback callback(stop_token, [&]() { callbackCalled = true; });

        while (!stop_token.stop_requested())
        {
            threadStartedEvent.Set();
        }
    });

    EXPECT_TRUE(threadStartedEvent.Wait());
    EXPECT_FALSE(callbackCalled);
    EXPECT_TRUE(source.request_stop());
    myThread.join();
    EXPECT_TRUE(callbackCalled);
}

TEST(stop_token_test, check_callback_not_called)
{
    ext::Event threadStartedEvent, stopThreadEvent;

    std::atomic_bool callbackCalled = false;
    {
        ext::stop_source source;
        ext::thread myThread([&, stop_token = source.get_token()]()
        {
            ext::stop_callback callback(stop_token, [&]() { callbackCalled = true; });

            threadStartedEvent.Set();
            EXPECT_TRUE(stopThreadEvent.Wait());
        });

        EXPECT_TRUE(threadStartedEvent.Wait());
        EXPECT_FALSE(callbackCalled);
        stopThreadEvent.Set();
        myThread.join();
    }
    EXPECT_FALSE(callbackCalled);
}