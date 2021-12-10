#pragma once

/*
    Service TickService

    Implementation of a class with a timer and the ability to connect timer tick handlers,
    You can set different intervals for the timer and specific identifiers for each timer.
    Available to set invoked UI timer and async

    Implemented a helper class CTickHandlerImpl that monitors timers and simplifies working with them
*/

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <windef.h>
#include <WinUser.h>

#include <ext/core/check.h>
#include <ext/core/singleton.h>

#include <ext/thread/invoker.h>

#include <ext/scope/auto_setter.h>

namespace ext::tick {

// tick parameter, passed to the tick handler
typedef LONG_PTR TickParam;
// type of clock used in the service
typedef std::chrono::steady_clock tick_clock;

// Interface for tick handlers, see TickSubscriber
struct ITickHandler
{
    virtual ~ITickHandler() = default;

    /// <summary>Called when the timer ticks</summary>
    /// <param name="tickParam">Parameter passed during subscription.</param>
    /// <returns>True - continue ticks, False - interrupt this timer.</returns>
    EXT_NODISCARD virtual bool OnTick(::ext::tick::TickParam tickParam) EXT_NOEXCEPT = 0;
};

// Tick service implementation, creates Invoker and Async timers for sending information about ticks to subscribers
// You can set the tick interval (it will work with kDefTickInterval precision)
// see TickSubscriber
class TickService
{
    friend ext::Singleton<TickService>;
public://---------------------------------------------------------------------//
    TickService() = default;

    // default tick interval
    inline static const std::chrono::milliseconds kDefTickInterval = std::chrono::milliseconds(200);

public:
    /// <summary>Add tick handler, set invoked==true if OnTick must be called from Invoker thread, async call othervise.</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickInterval">Tick interval for this parameter.</param>
    /// <param name="tickParam">Parameter passed to the handler on tick, allows you to identify the timer
    /// or pass information to the handler</param>
    template <bool invoked = true>
    void Subscribe(ITickHandler* handler, tick_clock::duration tickInterval = kDefTickInterval,
                   const TickParam& tickParam = 0)
    {
        if constexpr (invoked)
            m_invokedTimer.AddHandlerTimer(handler, tickInterval, tickParam);
        else
            m_asyncTimer.AddHandlerTimer(handler, tickInterval, tickParam);
    }

    /// <summary>Remove tick handler, set invoked==true if you want to remove invoked timer, false to remove async</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickParam">Tick parameter, if null - delete all handler timers.</param>
    template <bool invoked = true>
    void UnsubscribeHandler(ITickHandler* handler, const std::optional<TickParam>& tickParam = std::nullopt)
    {
        if constexpr (invoked)
            m_invokedTimer.RemoveHandler(handler, tickParam);
        else
            m_asyncTimer.RemoveHandler(handler, tickParam);
    }

    /// <summary>Checking if this handler has a timer with the given parameter</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickParam">Tick parameter.</param>
    EXT_NODISCARD bool IsTimerExist(ITickHandler* handler, const TickParam& tickParam)
    {
        return m_asyncTimer.IsHandlerExist(handler, tickParam) ||
               m_invokedTimer.IsHandlerExist(handler, tickParam);
    }

private:
    struct TimerBase
    {
        EXT_NODISCARD bool IsHandlerExist(ITickHandler* handler, const TickParam& tickParam)
        {
            std::scoped_lock lock(m_handlersMutex);
            const auto handlers = m_handlers.equal_range(handler);
            for (auto it = handlers.first; it != handlers.second; ++it)
            {
                if (it->second.tickParam == tickParam)
                    return true;
            }
            return false;
        }

        void AddHandlerTimer(ITickHandler* handler, tick_clock::duration tickInterval, TickParam tickParam)
        {
            std::scoped_lock lock(m_handlersMutex);
            m_handlers.emplace(handler, tickParam, tickInterval);
            m_handlersChanged = true;
            CheckTimerNecessary();
        }

        void RemoveHandler(ITickHandler* handler, const std::optional<TickParam>& tickParam)
        {
            std::scoped_lock lock(m_handlersMutex);
            const auto previousCount = m_handlers.size();
            if (tickParam.has_value())
            {
                const auto handlers = m_handlers.equal_range(handler);
                for (auto it = handlers.first; it != handlers.second; ++it)
                {
                    if (it->second.tickParam == tickParam)
                        m_handlers.erase(it);
                }
            }
            else
                m_handlers.erase(handler);

            m_handlersChanged =  previousCount != m_handlers.size();
            if (m_handlersChanged)
                CheckTimerNecessary();
        }

    protected:
        void OnTickTimer()
        {
            bool changed = false;
            std::scoped_lock lock(m_handlersMutex);
            for (size_t index = 0; index < m_handlers.size();)
            {
                auto handler = std::next(m_handlers.begin(), index);

                // если настало врем€ тика - сообщаем обработчику
                if (tick_clock::now() - handler->second.lastTickTime >= handler->second.tickInterval)
                {
                    const TickParam tickParam = handler->second.tickParam;
                    ITickHandler* handlerPointer = handler->first;

                    ext::scope::AutoSet changedAutoSet(m_handlersChanged, false, false);

                    // While tick subscriptions may happened
                    m_handlersMutex.unlock();
                    const bool result = handlerPointer->OnTick(tickParam);
                    m_handlersMutex.lock();

                    if (m_handlersChanged)
                    {
                        handler = m_handlers.end();
                        const auto handlers = m_handlers.equal_range(handlerPointer);
                        for (auto it = handlers.first; it != handlers.second; ++it)
                        {
                            if (it->second.tickParam == tickParam)
                            {
                                handler = it;
                                break;
                            }
                        }

                        changed |= true;
                    }

                    if (handler != m_handlers.end())
                    {
                        if (result)
                            handler->second.lastTickTime = tick_clock::now();
                        else
                        {
                            m_handlers.erase(handler);
                            continue;
                        }
                    }
                }

                ++index;
            }

            if (changed)
                CheckTimerNecessary();
        }

        virtual void CheckTimerNecessary() {}

    protected:
        struct TickHandlerInfo
        {
            TickParam tickParam = 0;
            tick_clock::duration tickInterval = kDefTickInterval;
            tick_clock::time_point lastTickTime = tick_clock::now();

            TickHandlerInfo(TickParam&& param, tick_clock::duration&& interval) : tickParam(param), tickInterval(interval)
            {}
        };

        typedef std::multimap<ITickHandler*, TickHandlerInfo> TickHandlersMap;

        bool m_handlersChanged = false;
        std::mutex m_handlersMutex;
        std::multimap<ITickHandler*, TickHandlerInfo> m_handlers;
    };

    struct InvokedTimer : TimerBase
    {
        InvokedTimer() = default;
        ~InvokedTimer() { StopTimer(); }

    private:
        void StartTimer()
        {
            EXT_ASSERT(m_timerId.has_value());
            ext::InvokeMethod([timerId = &m_timerId]()
            {
                timerId->emplace(::SetTimer(nullptr, NULL, (UINT)kDefTickInterval.count(),
                                            [](HWND, UINT, UINT_PTR, DWORD)
                                            {
                                                get_service<TickService>().m_invokedTimer.OnTickTimer();
                                            }));
            });
            EXT_ASSERT(m_timerId == 0);
        }

        void StopTimer()
        {
            if (m_timerId.has_value())
            {
                ::KillTimer(nullptr, *m_timerId);
                m_timerId.reset();
            }
        }

        void CheckTimerNecessary() override
        {
            // if handlers exist - timer should work
            if (m_handlers.empty() == m_timerId.has_value())
                m_handlers.empty() ? StopTimer() : StartTimer();
        }

    private:
        std::optional<UINT_PTR> m_timerId;
    } m_invokedTimer;

    struct AsyncTimer : TimerBase
    {
        AsyncTimer()
            : m_tickThread([&]()
            {
                while (m_work)
                {
                    OnTickTimer();
                    std::this_thread::sleep_for(kDefTickInterval);
                }
            })
        {}
        ~AsyncTimer() { m_work = false; m_tickThread.join(); }

    private:
        std::atomic_bool m_work = true;
        std::thread m_tickThread;

    } m_asyncTimer;
};

// Implementation of the base class of the tick handler, simplify tick subscription
struct TickSubscriber : public ITickHandler
{
    TickSubscriber() = default;
    virtual ~TickSubscriber() { UnsubscribeTimer(); UnsubscribeInvokedTimer(); }

    /// <summary>Add tick handler, tick function will be called async.</summary>
    /// <param name="tickInterval">Tick interval for this parameter.</param>
    /// <param name="tickParam">Parameter passed to the handler on tick, allows you to identify the timer
    /// or pass information to the handler</param>
    void SubscribeTimer(tick_clock::duration tickInterval = TickService::kDefTickInterval,
                        const TickParam& tickParam = 0)
    { get_service<TickService>().Subscribe<false>(this, tickInterval, tickParam); }

    /// <summary>Add tick timer, tick function will be called in main UI thread.</summary>
    /// <param name="tickInterval">Tick interval for this parameter.</param>
    /// <param name="tickParam">Parameter passed to the handler on tick, allows you to identify the timer
    /// or pass information to the handler</param>
    void SubscribeInvokedTimer(tick_clock::duration tickInterval = TickService::kDefTickInterval,
                               const TickParam& tickParam = 0)
    { get_service<TickService>().Subscribe<true>(this, tickInterval, tickParam); }

    /// <summary>Remove async tick timer</summary>
    /// <param name="tickParam">Tick parameter, if null - delete all handler timers.</param>
    void UnsubscribeTimer(const std::optional<TickParam>& tickParam = std::nullopt)
    { get_service<TickService>().UnsubscribeHandler<false>(this, tickParam); }

    /// <summary>Remove main UI thread tick timer</summary>
    /// <param name="tickParam">Tick parameter, if null - delete all handler timers</param>
    void UnsubscribeInvokedTimer(const std::optional<TickParam>& tickParam = std::nullopt)
    { get_service<TickService>().UnsubscribeHandler<true>(this, tickParam); }

    /// <summary>Checking if this handler has a timer with the given parameter.</summary>
    /// <param name="tickParam">Tick parameter.</param>
    EXT_NODISCARD bool IsTimerExist(const TickParam& tickParam)
    { return get_service<TickService>().IsTimerExist(this, tickParam); }
};

} // namespace ext::tick