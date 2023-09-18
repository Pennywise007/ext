#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <atomic>

#include <ext/core/dispatcher.h>

using namespace testing;

constexpr int kSyncCounter = 404;
constexpr int kAsyncCounter = 403;

struct Counter {
    explicit Counter() = default;
    explicit Counter(int value) : value_(value) {}
    Counter(Counter&& other)
     : value_(other.value_)
     , moves_(other.moves_ + 1)
     , copies_(other.copies_)
    {
        other.value_ = -303;
    }
    Counter(const Counter& other)
     : value_(other.value_)
     , moves_(other.moves_)
     , copies_(other.copies_ + 1)
    {}
    Counter& operator=(const Counter&) = delete;
    Counter& operator=(Counter&&) = delete;

    int value_;
    int moves_ = 0;
    int copies_ = 0;
};

struct IEvent : ext::events::IBaseEvent
{
    virtual void Event(int val) = 0;
    virtual void EventWithValueObject(Counter counter) = 0;
    virtual void EventWithRefObject(Counter& counter) = 0;
    virtual void EventWithMoveObject(Counter&& counter) = 0;
    virtual void EventWithCRefObject(const Counter& counter) = 0;
};

struct RecipientMock : ext::events::ScopeSubscription<IEvent>
{
    using Base = ext::events::ScopeSubscription<IEvent>;

    MOCK_METHOD(void, Event, (int val), (override));
    MOCK_METHOD(void, EventWithValueObject, (Counter counter), (override));
    MOCK_METHOD(void, EventWithRefObject, (Counter& counter), (override));
    MOCK_METHOD(void, EventWithMoveObject, (Counter&& counter), (override));
    MOCK_METHOD(void, EventWithCRefObject, (const Counter& counter), (override));

    using Base::Subscribe;
    using Base::Unsubscribe;
    using Base::SetFirstPriority;
};

TEST(dispatcher_test, auto_subscription_and_event_call)
{
    StrictMock<RecipientMock> mock;
    EXPECT_CALL(mock, Event(1));

    ext::send_event(&IEvent::Event, 1);
}

TEST(dispatcher_test, check_unsubscription)
{
    StrictMock<RecipientMock> mock;

    mock.Unsubscribe<IEvent>();
    ext::send_event(&IEvent::Event, -1);

    mock.Subscribe<IEvent>();
    EXPECT_CALL(mock, Event(2));
    ext::send_event(&IEvent::Event, 2);
}

TEST(dispatcher_test, call_order)
{
    StrictMock<RecipientMock> mock1;
    StrictMock<RecipientMock> mock2;
    StrictMock<RecipientMock> mock3;

    {
        InSequence sequence;
        EXPECT_CALL(mock1, Event(3));
        EXPECT_CALL(mock2, Event(3));
        EXPECT_CALL(mock3, Event(3));
    }

    ext::send_event(&IEvent::Event, 3);
    
    mock2.SetFirstPriority<IEvent>();
    mock3.SetFirstPriority<IEvent>();

    {
        InSequence sequence;
        EXPECT_CALL(mock3, Event(4));
        EXPECT_CALL(mock2, Event(4));
        EXPECT_CALL(mock1, Event(4));
    }
    ext::send_event(&IEvent::Event, 4);
}

TEST(dispatcher_test, interrupt_on_exception)
{
    StrictMock<RecipientMock> mock1;
    StrictMock<RecipientMock> mock2;
    StrictMock<RecipientMock> mock3;

    struct custom_exception : std::exception {};
    
    EXPECT_CALL(mock1, Event(-1));
    EXPECT_CALL(mock2, Event(-1)).WillOnce(Invoke([](auto&&) {
        throw custom_exception();
    }));

    EXPECT_THROW(ext::send_event(&IEvent::Event, -1), custom_exception);
} 

TEST(dispatcher_test, unsubscribe_during_event_call)
{
    StrictMock<RecipientMock> mock1;
    StrictMock<RecipientMock> mock2;
    StrictMock<RecipientMock> mock3;

    EXPECT_CALL(mock1, Event(7));
    EXPECT_CALL(mock2, Event(7)).WillOnce(Invoke([&](auto&&) {
        mock3.Unsubscribe<IEvent>();
    }));

    ext::send_event(&IEvent::Event, 7);
} 

TEST(dispatcher_test, subscribe_during_event_call)
{
    StrictMock<RecipientMock> mock1;
    StrictMock<RecipientMock> mock2;
    StrictMock<RecipientMock> mock3;
    mock3.Unsubscribe<IEvent>();

    EXPECT_CALL(mock1, Event(8));
    EXPECT_CALL(mock2, Event(8)).WillOnce(Invoke([&](auto&&) {
        mock3.Subscribe<IEvent>();
    }));
    EXPECT_CALL(mock3, Event(8));

    ext::send_event(&IEvent::Event, 8);
} 

TEST(dispatcher_test, async_call)
{
    StrictMock<RecipientMock> mock;
    EXPECT_CALL(mock, Event(6));

    ext::send_event_async(&IEvent::Event, 6);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

TEST(dispatcher_test, async_throwing_exception)
{
    StrictMock<RecipientMock> mock;
    EXPECT_CALL(mock, Event(_)).WillOnce(Invoke([](auto&&)
    {
        throw std::exception();
    }));

    auto cRefRes = ext::send_event_async(&IEvent::Event, -1);
    EXPECT_THROW(cRefRes.get(), std::exception);
}

TEST(dispatcher_test, arguments_copying_counter_reference)
{
    StrictMock<RecipientMock> mock;
    EXPECT_CALL(mock, EventWithRefObject(_)).Times(2).WillRepeatedly(Invoke([](Counter& counter)
    {
        EXPECT_EQ(counter.copies_, 0);
        EXPECT_EQ(counter.moves_, 0);
        counter.value_ = 101;
    }));

    Counter counterSync(kSyncCounter), counterAsync(kAsyncCounter);
    ext::send_event(&IEvent::EventWithRefObject, std::ref(counterSync));
    EXPECT_EQ(counterSync.value_, 101);

    ext::send_event_async(&IEvent::EventWithRefObject, std::ref(counterAsync)).get();
    EXPECT_EQ(counterAsync.value_, 101);
}

TEST(dispatcher_test, arguments_copying_counter_rvalue)
{
    StrictMock<RecipientMock> mock;
    EXPECT_CALL(mock, EventWithMoveObject(_)).Times(2).WillRepeatedly(Invoke([](Counter&& counter)
    {
        EXPECT_EQ(counter.copies_, 0);
        EXPECT_EQ(counter.moves_, counter.value_ == kSyncCounter ? 0 : 2);
    }));

    Counter counterSync(kSyncCounter), counterAsync(kAsyncCounter);
    ext::send_event(&IEvent::EventWithMoveObject, std::move(counterSync));
    ext::send_event_async(&IEvent::EventWithMoveObject, std::move(counterAsync)).get();
}

struct Recipient : RecipientMock
{
    void EventWithValueObject(Counter counter) override {
        if (counter.value_ == kSyncCounter)
        {
            EXPECT_EQ(counter.copies_, 1);
            EXPECT_EQ(counter.moves_, 0);
        }
        else if (counter.value_ == kAsyncCounter)
        {
            EXPECT_EQ(counter.copies_, 2);
            EXPECT_EQ(counter.moves_, 1);
        }
        else
            FAIL() << "Unexpected value " << counter.value_;
        ++calls;
    }
    size_t calls = 0;
};

TEST(dispatcher_test, arguments_copying_counter_value)
{
    Recipient recipient1, recipient2;

    Counter counterSync(kSyncCounter), counterAsync(kAsyncCounter);
    ext::send_event(&IEvent::EventWithValueObject, counterSync);
    ext::send_event_async(&IEvent::EventWithValueObject, counterAsync).get();
    EXPECT_EQ(counterSync.value_, kSyncCounter);
    EXPECT_EQ(counterAsync.value_, kAsyncCounter);

    EXPECT_EQ(recipient1.calls, 2);
    EXPECT_EQ(recipient2.calls, 2);
}

TEST(dispatcher_test, arguments_copying_counter_const_reference)
{
    StrictMock<RecipientMock> mock;
    EXPECT_CALL(mock, EventWithCRefObject(_)).Times(2).WillRepeatedly(Invoke([](const Counter& counter)
    {
        EXPECT_EQ(counter.copies_, 0);
        EXPECT_EQ(counter.moves_, 0);
    }));

    Counter counterSync(kSyncCounter), counterAsync(kAsyncCounter);
    ext::send_event(&IEvent::EventWithCRefObject, std::cref(counterSync));
    ext::send_event_async(&IEvent::EventWithCRefObject, std::cref(counterAsync)).get();
}
