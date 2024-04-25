#pragma once

/*
    Implementation of the event-driven singleton Dispatcher, serve to notify subscribers about events

Example of event interface
    struct IEvent : ext::events::IBaseEvent
    {
        virtual void Event(int val) = 0;
    };

Example of sending an event:
    ext::send_event(&IEvent::Event, 10);

    // Behaviour is similar to the send_event, but event will be handled asynchronous without freezing caller thread and all exceptions will be ignored
    ext::send_event_async(&IEvent::Event, 10);

Example of recipient:
    struct Recipient
        : ext::events::ScopeSubscription<IEvent>
    {
        void Event(int val) override { std::cout << "Event"; }
    }
*/
#include <exception>
#include <functional>
#include <list>
#include <map>
#include <shared_mutex>

#include <type_traits>

#include <ext/core/defines.h>
#include <ext/core/mpl.h>
#include <ext/core/noncopyable.h>
#include <ext/core/singleton.h>
#include <ext/core/tracer.h>

#include <ext/error/exception.h>

#include <ext/scope/defer.h>

#include <ext/thread/invoker.h>
#include <ext/thread/thread_pool.h>

#include <ext/types/utils.h>

namespace ext {
/*
    Sending event and waiting for handling event by recipients.
    Event will be sent in the same order as recipients were registered

    Note: If ext::events::event_handled exception was raised during an event handling it will stop iterating over recipients.
    Note: If any other unhandled exceptions was raised during event processing it will throw it to the caller code

    Example:
        ext::send_event(&IEvent::Event, 10);
*/
template <typename IEvent, typename Function, typename... Args>
void send_event(Function IEvent::* function, Args&&... eventArgs) EXT_THROWS(...);

/*
    Sending event nd don`t wait for handling, continue execution
    Event will be sent in the same order as recipients were registered.

    Note: Code will be executed in a different thread, be aware about arguments synchronization
    Note: If ext::events::event_handled exception was raised during an event handling it will stop iterating over recipients.
    Note: If any other unhandled exceptions was raised during event processing it will store it in the returned future

    Example:
        ext::send_event_async(&IEvent::Event, 10);
*/
template <typename IEvent, typename Function, typename... Args>
std::future<void> send_event_async(Function IEvent::* function, Args&&... eventArgs) noexcept;

/*
    Executing callback for every event recipient
    Event will be sent in the same order as recipients were registered.

    Note: If ext::events::event_handled exception was raised during an event handling it will stop iterating over recipients.

    Example:
        ext::call_for_every_recipient([](IEvent* recipient) {
            ...
        });
*/
template <typename IEvent>
void call_for_every_recipient(const std::function<void(IEvent* recipient)>& callback) EXT_THROWS(...);

namespace events {

// special exception which should stop iterating over event recipients
struct event_handled : std::exception {};

// base interface for all events
struct IBaseEvent
{
    virtual ~IBaseEvent() = default;
};

// Event dispatcher class
class Dispatcher
{
    friend ext::Singleton<Dispatcher>;
public:
    // Subscribing / unsubscribing the synchronous event receiver
    template <typename IEvent>
    void Subscribe(IEvent* recipient) noexcept;
    template <typename IEvent>
    void Unsubscribe(IEvent* recipient, bool checkSubscription = true) noexcept;
    // check if recipient subscribed
    template <typename IEvent>
    bool IsSubscribed(IEvent* recipient) const noexcept;
    // Update priority for event subscriber - on raising event this subscriber will receive information first
    template <typename IEvent>
    void SetFirstPriority(IEvent* recipient) EXT_THROWS(ext::check::CheckFailedException);

// sending events
public:
    // @see send_event. Sending event and waiting for handling event by recipients synchronously
    template <typename IEvent, typename Function, typename... Args>
    void SendEvent(Function IEvent::* function, Args&&... eventArgs) const EXT_THROWS(...);

    // @see send_event_async. Sending event and don`t wait for handling, continue execution, args should be copy constructible
    template <typename IEvent, typename Function, typename... Args>
    std::future<void> SendEventAsync(Function IEvent::* function, Args&&... eventArgs) const noexcept;

    // @see call_for_every_recipient. Call callback for every recipient who subscribed on event
    template <typename IEvent>
    void ForEveryRecipient(const std::function<void(IEvent* recipient)>& callback) const EXT_THROWS(...);

private:
    typedef std::list<IBaseEvent*> EventRecipients;
    typedef size_t EventId;

    std::map<EventId, EventRecipients> m_eventRecipients;
    mutable std::shared_mutex m_recipientsMutex;
};

// Scope subscription manager
template <typename... IEvents>
struct ScopeSubscription : ext::NonCopyable, IEvents...
{
    explicit ScopeSubscription(bool autoSubscription = true) noexcept
    {
        if (autoSubscription)
            SubscribeAll();
    }
    virtual ~ScopeSubscription() noexcept { UnsubscribeAll(false); }

protected:
    void SubscribeAll() const noexcept
    {
        (..., ScopeSubscription::Subscribe<IEvents>());
    }
    void UnsubscribeAll(bool checkSubscription = true) const noexcept
    {
        (..., ScopeSubscription::Unsubscribe<IEvents>(checkSubscription));
    }

    template <typename IEvent>
    void Subscribe() const noexcept
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Subscribing to an event not included in the event list");
        get_singleton<Dispatcher>().Subscribe(GetEventPointer<IEvent>());
    }
    template <typename IEvent>
    void Unsubscribe(bool checkSubscription = true) const noexcept
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Unsubscribing to an event not included in the event list");
        get_singleton<Dispatcher>().Unsubscribe(GetEventPointer<IEvent>(), checkSubscription);
    }

    // Update priority for event subscriber - on raising event this subscriber will receive information first
    template <typename IEvent>
    void SetFirstPriority() const EXT_THROWS(ext::check::CheckFailedException)
    {
        get_singleton<Dispatcher>().SetFirstPriority(GetEventPointer<IEvent>());
    }

private:
    template <typename IEvent>
    IEvent* GetEventPointer() const noexcept
    {
        return const_cast<IEvent*>(static_cast<const IEvent*>(this));
    }
};

template <typename IEvent>
void Dispatcher::Subscribe(IEvent* recipient) noexcept
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::shared_mutex> lock(m_recipientsMutex);
    auto& eventRecipients = m_eventRecipients[typeid(IEvent).hash_code()];
    EXT_ASSERT(std::find(eventRecipients.begin(), eventRecipients.end(), static_cast<IBaseEvent*>(recipient)) == eventRecipients.end())
        << EXT_TRACE_FUNCTION << "Already subscribed";
    eventRecipients.emplace_back(static_cast<IBaseEvent*>(recipient));
}

template <typename IEvent>
void Dispatcher::Unsubscribe(IEvent* recipient, bool checkSubscription) noexcept
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::shared_mutex> lock(m_recipientsMutex);

    if (auto eventIt = m_eventRecipients.find(typeid(IEvent).hash_code()); eventIt != m_eventRecipients.end())
    {
        if (const auto recipientIt = std::find(eventIt->second.begin(), eventIt->second.end(), static_cast<IBaseEvent*>(recipient));
            recipientIt != eventIt->second.end())
        {
            eventIt->second.erase(recipientIt);
            if (eventIt->second.empty())
                m_eventRecipients.erase(eventIt);
        }
        else
            EXT_DUMP_IF(checkSubscription) << "Recipient already unsubscribed";
    }
    else
        EXT_DUMP_IF(checkSubscription) << "No one subscribed to event";
}

template<typename IEvent>
inline bool Dispatcher::IsSubscribed(IEvent* recipient) const noexcept
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::shared_mutex> lock(m_recipientsMutex);
    auto eventIt = m_eventRecipients.find(typeid(IEvent).hash_code());
    return eventIt != m_eventRecipients.end() &&
        std::find(eventIt->second.begin(), eventIt->second.end(), static_cast<IBaseEvent*>(recipient)) != eventIt->second.end();
}

template<typename IEvent>
inline void Dispatcher::SetFirstPriority(IEvent* recipient) EXT_THROWS(ext::check::CheckFailedException)
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::shared_mutex> lock(m_recipientsMutex);
    auto eventIt = m_eventRecipients.find(typeid(IEvent).hash_code());
    EXT_CHECK(eventIt != m_eventRecipients.end()) << "Event recipient not registered";
    const auto it = std::find(eventIt->second.begin(), eventIt->second.end(), static_cast<IBaseEvent*>(recipient));
    EXT_CHECK(it != eventIt->second.end()) << "Event recipient not registered";
    eventIt->second.erase(it);
    eventIt->second.push_front(static_cast<IBaseEvent*>(recipient));
}

template <typename IEvent, typename Function, typename... Args>
void Dispatcher::SendEvent(Function IEvent::* function, Args&&... eventArgs) const EXT_THROWS(...)
{
    ForEveryRecipient<IEvent>([&](IEvent* recipient)
    {
        (recipient->*function)(std::forward<Args>(eventArgs)...);
    });
}

template <typename IEvent, typename Function, typename... Args>
std::future<void> Dispatcher::SendEventAsync(Function IEvent::* function, Args&&... eventArgs) const noexcept
{
    auto functionToInvoke = [](Function IEvent::* function, std::decay_t<Args>&&... eventArgs) {
        get_singleton<Dispatcher>().SendEvent(function, std::forward<Args>(eventArgs)...);
    };

    ext::ThreadInvoker invoker(std::move(functionToInvoke), function, std::forward<Args>(eventArgs)...);
    
    static thread_pool thread_pool(1);
    return thread_pool.add_task([invoker = std::move(invoker)]() mutable { 
            invoker();
        }).second;
}

template <typename IEvent>
void Dispatcher::ForEveryRecipient(const std::function<void(IEvent* recipient)>& callback) const EXT_THROWS(...)
{
    std::shared_lock<std::shared_mutex> lock(m_recipientsMutex);

    auto it = m_eventRecipients.find(typeid(IEvent).hash_code());
    if (it != m_eventRecipients.end())
    {
        for (size_t index = 0; index < it->second.size(); ++index)
        {
            IEvent* recipient = dynamic_cast<IEvent*>(*std::next(it->second.begin(), index));
            EXT_DUMP_IF(!recipient) << "How we skip this situation in subscribe?";

            if (!recipient)
                continue;

            {
                // during callback call (un)subscribe functions might be called, don't lock it
                m_recipientsMutex.unlock_shared();
                EXT_DEFER(m_recipientsMutex.lock_shared());

                // call function without mutex to allow unsubscription during this call
                try
                {
                    callback(recipient);
                }
                catch (const ::ext::events::event_handled&)
                {
                    return;
                }
            }

            // during subscriptions\resubscriptions iterator may become invalid
            it = m_eventRecipients.find(typeid(IEvent).hash_code());
            if (it == m_eventRecipients.end())
                return;
        }
    }
    else
        EXT_TRACE_DBG() << EXT_TRACE_FUNCTION << "No subscribers on interface " << ext::type_name<IEvent>();
}

} // namespace events

template <typename IEvent, typename Function, typename... Args>
void send_event(Function IEvent::* function, Args&&... eventArgs) EXT_THROWS(...)
{
    get_singleton<events::Dispatcher>().SendEvent(function, std::forward<Args>(eventArgs)...);
}

template <typename IEvent, typename Function, typename... Args>
std::future<void> send_event_async(Function IEvent::* function, Args&&... eventArgs) noexcept
{
    return get_singleton<events::Dispatcher>().SendEventAsync(function, std::forward<Args>(eventArgs)...);
}

template <typename IEvent>
void call_for_every_recipient(const std::function<void(IEvent* recipient)>& callback) EXT_THROWS(...)
{
    return get_singleton<events::Dispatcher>().ForEveryRecipient(callback);
}

} // namespace ext
