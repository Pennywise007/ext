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

Example of recipient:
    struct Recipient : ext::events::ScopeSubscription<IEvent>
    {
        void Event(int val) override { std::cout << "Event"; }
    }
*/
#include <map>
#include <shared_mutex>
#include <set>

#include <type_traits>

#include "ext/core/check.h"
#include "ext/core/defines.h"
#include "ext/core/noncopyable.h"
#include "ext/core/singleton.h"
#include "ext/core/mpl.h"

#include "ext/thread/thread_pool.h"

namespace ext::events { class Dispatcher; }
namespace ext {

// Send sync event to all recipients, example: ext::SendEvent(&IEvent::Event, 10);
template <typename IEvent, typename Function, typename... Args>
void send_event(Function IEvent::* function, Args&&... eventArgs)
{
    get_service<events::Dispatcher>().SendEvent(function, std::forward<Args>(eventArgs)...);
}

// Send async event to all recipients, example: ext::SendEventAsync(&IEvent::Event, 10);
// Don't forget to sync the arguments you send
template <typename IEvent, typename Function, typename... Args>
void send_event_async(Function IEvent::* function, Args&&... eventArgs)
{
    get_service<events::Dispatcher>().SendEventAsync(function, std::forward<Args>(eventArgs)...);
}

} // namespace ext

namespace ext::events {

// base interface for all events
interface IBaseEvent
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
    void Subscribe(IEvent* recipient);
    template <typename IEvent>
    void Unsubscribe(IEvent* recipient);

    // Subscribing / unsubscribing the asynchronous event receiver, all event callbacks can be called from different threads
    template <typename IEvent>
    void SubscribeAsync(IEvent* recipient);
    template <typename IEvent>
    void UnsubscribeAsync(IEvent* recipient);

// sending events
public:
    // synchronous notification of subscribers about an event that has occurred
    template <typename IEvent, typename Function, typename... Args>
    void SendEvent(Function IEvent::* function, Args&&... eventArgs);
    // asynchronous notification of subscribers about an event
    template <typename IEvent, typename Function, typename... Args>
    void SendEventAsync(Function IEvent::* function, Args&&... eventArgs);

private:
    struct EventRecipients
    {
        std::set<IBaseEvent*> syncRecipients;
        std::set<IBaseEvent*> asyncRecipients;
    };

    typedef size_t EventId;
    std::map<EventId, EventRecipients> m_eventRecipients;
    std::shared_mutex m_recipientsMutex;

    thread_pool m_threadPool = { nullptr, 1 };
};

// Scope subscription manager
template <typename... IEvents>
struct ScopeSubscription : ext::NonCopyable, IEvents...
{
    explicit ScopeSubscription() { (..., ScopeSubscription::Subscribe<IEvents>()); }
    virtual ~ScopeSubscription() { (..., ScopeSubscription::Unsubscribe<IEvents>()); }

protected:
    template <typename IEvent>
    void Subscribe()
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Subscribing to an event not included in the event list");
        get_service<Dispatcher>().Subscribe(static_cast<IEvent*>(this));
    }
    template <typename IEvent>
    void Unsubscribe()
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Unsubscribing to an event not included in the event list");
        get_service<Dispatcher>().Unsubscribe(static_cast<IEvent*>(this));
    }
};

// Scope subscription manager
template <typename... IEvents>
struct ScopeAsyncSubscription : ext::NonCopyable, IEvents...
{
    explicit ScopeAsyncSubscription() { (..., ScopeAsyncSubscription::SubscribeAsync<IEvents>()); }
    virtual ~ScopeAsyncSubscription() { (..., ScopeAsyncSubscription::UnsubscribeAsync<IEvents>()); }

protected:
    template <typename IEvent>
    void SubscribeAsync()
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Subscribing to an event not included in the event list");
        get_service<Dispatcher>().SubscribeAsync(static_cast<IEvent*>(this));
    }
    template <typename IEvent>
    void UnsubscribeAsync()
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Unsubscribing to an event not included in the event list");
        get_service<Dispatcher>().SubscribeAsync(static_cast<IEvent*>(this));
    }
};

template <typename IEvent>
void Dispatcher::Subscribe(IEvent* recipient)
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::mutex> lock(m_recipientsMutex);
    EXT_DUMP_IF(!m_eventRecipients[typeid(IEvent).hash_code()].syncRecipients.insert(static_cast<IBaseEvent*>(recipient)).second)
        << EXT_TRACE_FUNCTION << "Already subscribed";
}

template <typename IEvent>
void Dispatcher::Unsubscribe(IEvent* recipient)
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::mutex> lock(m_recipientsMutex);

    if (auto eventIt = m_eventRecipients.find(typeid(IEvent).hash_code()); eventIt != m_eventRecipients.end())
    {
        if (const auto recipientIt = eventIt->second.syncRecipients.find(static_cast<IBaseEvent*>(recipient));
            recipientIt != eventIt->second.syncRecipients.end())
        {
            eventIt->second.syncRecipients.erase(recipientIt);
            if (eventIt->second.syncRecipients.empty() && eventIt->second.asyncRecipients.empty())
                m_eventRecipients.erase(eventIt);
        }
        else
            EXT_DUMP_IF(false) << EXT_TRACE_FUNCTION << "Recipient already unsubscribed";
    }
    else
        EXT_DUMP_IF(false) << EXT_TRACE_FUNCTION << "No one subscribed to event";
}

template <typename IEvent>
void Dispatcher::SubscribeAsync(IEvent* recipient)
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::mutex> lock(m_recipientsMutex);
    EXT_DUMP_IF(!m_eventRecipients[typeid(IEvent).hash_code()].asyncRecipients.insert(static_cast<IBaseEvent*>(recipient)).second)
        << EXT_TRACE_FUNCTION << "Already subscribed";
}

template <typename IEvent>
void Dispatcher::UnsubscribeAsync(IEvent* recipient)
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::mutex> lock(m_recipientsMutex);

    if (auto eventIt = m_eventRecipients.find(typeid(IEvent).hash_code()); eventIt != m_eventRecipients.end())
    {
        if (const auto recipientIt = eventIt->second.asyncRecipients.find(static_cast<IBaseEvent*>(recipient));
            recipientIt != eventIt->second.asyncRecipients.end())
        {
            eventIt->second.asyncRecipients.erase(recipientIt);
            if (eventIt->second.asyncRecipients.empty() && eventIt->second.syncRecipients.empty())
                m_eventRecipients.erase(eventIt);
        }
        else
            EXT_DUMP_IF(false) << EXT_TRACE_FUNCTION << "Recipient already unsubscribed";
    }
    else
        EXT_DUMP_IF(false) << EXT_TRACE_FUNCTION << "No one subscribed to event";
}

template <typename IEvent, typename Function, typename... Args>
void Dispatcher::SendEvent(Function IEvent::* function, Args&&... eventArgs)
{
    std::shared_lock<std::mutex> lock(m_recipientsMutex);

    auto it = m_eventRecipients.find(typeid(IEvent).hash_code());
    if (it != m_eventRecipients.end())
    {
        auto callFunction = [&function, &eventArgs..., mutex = &m_recipientsMutex](std::set<IBaseEvent*>& recipients)
        {
            for (size_t index = 0; index < recipients.size(); ++index)
            {
                IEvent* recipient = dynamic_cast<IEvent*>(*std::next(recipients.begin(), index));
                EXT_DUMP_IF(!recipient) << "How we skip this situation in subscribe?";

                if (!recipient)
                    continue;

                mutex->unlock_shared();

                // call function without mutex to allow unsubscription during event call
                (recipient->*function)(std::forward<Args>(eventArgs)...);

                mutex->lock_shared();
            }
        };

        callFunction(it->second.syncRecipients);
        callFunction(it->second.asyncRecipients);
    }
    else
        EXT_ASSERT(false) << "No subscribers on event";
}

template <typename IEvent, typename Function, typename... Args>
void Dispatcher::SendEventAsync(Function IEvent::* function, Args&&... eventArgs)
{
    m_threadPool.add_task([function, eventArgs...]()
                          {
                               get_service<Dispatcher>().SendEvent(function, eventArgs...);
                          });
}

} // namespace ext::events
