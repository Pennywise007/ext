#pragma once

/*
    Implementation of the event-driven singleton Dispatcher, serve to notify subscribers about events

Example of event interface
    struct IEvent : ext::events::IBaseEvent
    {
        virtual void Event(int val) = 0;
    };

Example of sending an event:
    ext::send_event(&IEvent::Event, 10);        // sending event and waiting for handling event by recepients synchroniously
    ext::send_event_async(&IEvent::Event, 10);  // send event and don`t wait for handling, continue execution

Example of recipient:
    struct Recipient
        : ext::events::ScopeSubscription<IEvent> // OR ext::events::ScopeAsyncSubscription<IEvent>
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
#include "ext/error/exception.h"

#include "ext/thread/thread_pool.h"

namespace ext::events { class Dispatcher; }
namespace ext {

// Sending event and waiting for handling event by recepients synchroniously
// Example: ext::send_event(&IEvent::Event, 10);
template <typename IEvent, typename Function, typename... Args>
void send_event(Function IEvent::* function, Args&&... eventArgs) EXT_THROWS(...)
{
    get_service<events::Dispatcher>().SendEvent(function, std::forward<Args>(eventArgs)...);
}

// Sending event and don`t wait for handling, continue execution, args should be copy constructible
// Don't forget to sync the arguments you send, arguments need to have a copy constructor
// Example: ext::send_event_async(&IEvent::Event, 10);
template <typename IEvent, typename Function, typename... Args>
void send_event_async(Function IEvent::* function, Args&&... eventArgs) EXT_NOEXCEPT
{
    get_service<events::Dispatcher>().SendEventAsync(function, std::forward<Args>(eventArgs)...);
}

namespace events {

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
    void Subscribe(IEvent* recipient) EXT_NOEXCEPT;
    template <typename IEvent>
    void Unsubscribe(IEvent* recipient) EXT_NOEXCEPT;
    // check if recipient subscribed
    template <typename IEvent>
    bool IsSubscribed(IEvent* recipient) const EXT_NOEXCEPT;

// sending events
public:
    // sending event and waiting for handling event by recepients synchroniously
    template <typename IEvent, typename Function, typename... Args>
    void SendEvent(Function IEvent::* function, Args&&... eventArgs) const EXT_THROWS(...);
    // sending event and don`t wait for handling, continue execution, args should be copy constructible
    template <typename IEvent, typename Function, typename... Args>
    void SendEventAsync(Function IEvent::* function, Args&&... eventArgs) const EXT_NOEXCEPT;

private:
    typedef std::set<IBaseEvent*> EventRecipients;
    typedef size_t EventId;

    std::map<EventId, EventRecipients> m_eventRecipients;
    mutable std::shared_mutex m_recipientsMutex;

    mutable thread_pool m_threadPool = { nullptr, 1 };
};

// Scope subscription manager
template <typename... IEvents>
struct ScopeSubscription : ext::NonCopyable, IEvents...
{
    explicit ScopeSubscription(const bool autoSubscription = true) EXT_NOEXCEPT
        : m_autoSubscription(autoSubscription)
    {
        if (m_autoSubscription)
            SubscribeAll();
    }
    virtual ~ScopeSubscription() EXT_NOEXCEPT { UnsubscribeAll(); }

protected:
    void SubscribeAll() const EXT_NOEXCEPT
    {
        (..., ScopeSubscription::Subscribe<IEvents>());
    }
    void UnsubscribeAll() const EXT_NOEXCEPT
    {
        (..., ScopeSubscription::Unsubscribe<IEvents>());
    }

    template <typename IEvent>
    void Subscribe() const EXT_NOEXCEPT
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Subscribing to an event not included in the event list");
        get_service<Dispatcher>().Subscribe(GetEventPointer<IEvent>());
    }
    template <typename IEvent>
    void Unsubscribe() const EXT_NOEXCEPT
    {
        static_assert(ext::mpl::contain_type_v<IEvent, IEvents...>, "Unsubscribing to an event not included in the event list");
        if (m_autoSubscription)
            get_service<Dispatcher>().Unsubscribe(GetEventPointer<IEvent>());
        else
        {
            auto* event = GetEventPointer<IEvent>();
            auto& dispatcher = get_service<Dispatcher>();
            if (dispatcher.IsSubscribed(event))
                dispatcher.Unsubscribe(event);
        }
    }

private:
    template <typename IEvent>
    IEvent* GetEventPointer() const EXT_NOEXCEPT
    {
        return const_cast<IEvent*>(static_cast<const IEvent*>(this));
    }
private:
    const bool m_autoSubscription;
};

template <typename IEvent>
void Dispatcher::Subscribe(IEvent* recipient) EXT_NOEXCEPT
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::shared_mutex> lock(m_recipientsMutex);
    EXT_DUMP_IF(!m_eventRecipients[typeid(IEvent).hash_code()].emplace(static_cast<IBaseEvent*>(recipient)).second)
        << EXT_TRACE_FUNCTION << "Already subscribed";
}

template <typename IEvent>
void Dispatcher::Unsubscribe(IEvent* recipient) EXT_NOEXCEPT
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::shared_mutex> lock(m_recipientsMutex);

    if (auto eventIt = m_eventRecipients.find(typeid(IEvent).hash_code()); eventIt != m_eventRecipients.end())
    {
        if (const auto recipientIt = eventIt->second.find(static_cast<IBaseEvent*>(recipient));
            recipientIt != eventIt->second.end())
        {
            eventIt->second.erase(recipientIt);
            if (eventIt->second.empty())
                m_eventRecipients.erase(eventIt);
        }
        else
            EXT_DUMP_IF(true) << EXT_TRACE_FUNCTION << "Recipient already unsubscribed";
    }
    else
        EXT_DUMP_IF(true) << EXT_TRACE_FUNCTION << "No one subscribed to event";
}

template<typename IEvent>
inline bool Dispatcher::IsSubscribed(IEvent* recipient) const EXT_NOEXCEPT
{
    static_assert(std::is_base_of_v<IBaseEvent, IEvent>, "Event must be inherited from IBaseEvent!");

    std::unique_lock<std::shared_mutex> lock(m_recipientsMutex);
    auto eventIt = m_eventRecipients.find(typeid(IEvent).hash_code());
    return eventIt != m_eventRecipients.end() &&
        eventIt->second.find(static_cast<IBaseEvent*>(recipient)) != eventIt->second.end();
}

template <typename IEvent, typename Function, typename... Args>
void Dispatcher::SendEvent(Function IEvent::* function, Args&&... eventArgs) const EXT_THROWS(...)
{
    std::shared_lock<std::shared_mutex> lock(m_recipientsMutex);

    auto it = m_eventRecipients.find(typeid(IEvent).hash_code());
    if (it != m_eventRecipients.end())
    {
        for (size_t index = 0; index < it->second.size() && !ext::this_thread::interruption_requested(); ++index)
        {
            IEvent* recipient = dynamic_cast<IEvent*>(*std::next(it->second.begin(), index));
            EXT_DUMP_IF(!recipient) << "How we skip this situation in subscribe?";

            if (!recipient)
                continue;

            m_recipientsMutex.unlock_shared();

            // call function without mutex to allow unsubscription during event call
            (recipient->*function)(std::forward<Args>(eventArgs)...);

            m_recipientsMutex.lock_shared();

            // during subscriptions\resubscriptions iterator may become invalid
            it = m_eventRecipients.find(typeid(IEvent).hash_code());
            if (it == m_eventRecipients.end())
                return;
        }
    }
    else
        EXT_ASSERT(false) << "No subscribers on event";
}

template <typename IEvent, typename Function, typename... Args>
void Dispatcher::SendEventAsync(Function IEvent::* function, Args&&... eventArgs) const EXT_NOEXCEPT
{
    m_threadPool.add_task([function, eventArgs...]()
                          {
                               try
                               {
                                   get_service<Dispatcher>().SendEvent(function, eventArgs...);
                               }
                               catch (...)
                               {
                                   ext::ManageException((L"Failure during asynch sending events to " + std::widen(typeid(IEvent).name())).c_str());
                               }
                          });
}

} // namespace events
} // namespace ext
