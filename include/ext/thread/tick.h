#pragma once

/*
    Service TickService

    Implementation of a class with a timer and the ability to connect timer tick handlers,
    You can set different intervals for the timer and specific identifiers for each timer.
    Available to set invoked UI timer and async

    Implemented a helper class CTickHandlerImpl that monitors timers and simplifies working with them

Example:
    struct Test : ::ext::tick::TickSubscriber
    {
        Test()
        {
            TickSubscriber::SubscribeTimer(std::chrono::minutes(5));
        }

    // ITickHandler
    private:
        /// <summary>Called when the timer ticks</summary>
        /// <param name="tickParam">Parameter passed during subscription.</param>
        void OnTick(::ext::tick::TickParam) noexcept override
        {
            ... // execute text each 5 minutes
        }
    };
*/

#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <optional>

#ifdef __AFX_H__
#include <windows.h>
#include <WinUser.h>
#endif // __AFX_H__

#include <ext/core/check.h>
#include <ext/core/singleton.h>

#include <ext/scope/defer.h>

#include <ext/thread/invoker.h>
#include <ext/thread/thread.h>

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
    virtual void OnTick(TickParam tickParam) noexcept = 0;
};

// Tick service implementation, creates Invoker and Async timers for sending information about ticks to subscribers
// You can set the tick interval (it will work with kDefTickInterval precision)
// see TickSubscriber
class TickService
{
    friend ext::Singleton<TickService>;
public:
    TickService() = default;

    // default tick interval
    inline static const auto kDefTickInterval = std::chrono::milliseconds(200);

public:
    /// <summary>Add tick handler, OnTick will be called asynhroniously.</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickInterval">Tick interval for this parameter.</param>
    /// <param name="tickParam">Parameter passed to the handler on tick, allows you to identify the timer
    /// or pass information to the handler</param>
    void SubscribeAsync(ITickHandler* handler, tick_clock::duration tickInterval = kDefTickInterval, TickParam tickParam = 0)
    {
        m_asyncTimer.AddHandlerTimer(handler, std::move(tickInterval), std::move(tickParam));
    }

    /// <summary>Remove asynchronious tick handler</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickParam">Tick parameter, if null - delete all handler timers.</param>
    void UnsubscribeAsync(ITickHandler* handler, const std::optional<TickParam>& tickParam = std::nullopt)
    {
        m_asyncTimer.RemoveHandler(handler, tickParam);
    }

#ifdef __AFX_H__
    /// <summary>Add tick handler, OnTick must be called from Invoker thread.</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickInterval">Tick interval for this parameter.</param>
    /// <param name="tickParam">Parameter passed to the handler on tick, allows you to identify the timer
    /// or pass information to the handler</param>
    void SubscribeInvoked(ITickHandler* handler, tick_clock::duration tickInterval = kDefTickInterval, TickParam tickParam = 0)
    {
        m_invokedTimer.AddHandlerTimer(handler, std::move(tickInterval), std::move(tickParam));
    }

    /// <summary>Remove invoked tick handler</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickParam">Tick parameter, if null - delete all handler timers.</param>
    void UnsubscribeInvoked(ITickHandler* handler, const std::optional<TickParam>& tickParam = std::nullopt)
    {
        m_invokedTimer.RemoveHandler(handler, tickParam);
    }
#endif // __AFX_H__

    /// <summary>Checking if this handler has a timer with the given parameter</summary>
    /// <param name="handler">Handler pointer.</param>
    /// <param name="tickParam">Tick parameter.</param>
    [[nodiscard]] bool IsTimerExist(ITickHandler* handler, const TickParam& tickParam)
    {
        return 
#ifdef __AFX_H__
            m_invokedTimer.IsHandlerExist(handler, tickParam) ||
#endif // __AFX_H__
            m_asyncTimer.IsHandlerExist(handler, tickParam);
    }

private:
    struct Timer
    {
        [[nodiscard]] bool IsHandlerExist(ITickHandler* handler, const TickParam& tickParam)
        {
            std::scoped_lock lock(m_handlersMutex);
            return FindTickHandler(handler, tickParam) != m_handlers.end();
        }

        void AddHandlerTimer(ITickHandler* handler, tick_clock::duration&& tickInterval, TickParam&& tickParam)
        {
            std::scoped_lock lock(m_handlersMutex);
            if (auto it = FindTickHandler(handler, tickParam); it != m_handlers.end())
            {
                it->second.tickInterval = std::move(tickInterval);
                return;
            }

            m_handlers.emplace(std::make_pair(handler, TickHandlerInfo(std::move(tickParam), std::move(tickInterval))));
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
                for (auto it = handlers.first; it != handlers.second;)
                {
                    if (it->second.tickParam == tickParam)
                        it = m_handlers.erase(it);
                    else
                        ++it;
                }
            }
            else
                m_handlers.erase(handler);

            m_handlersChanged = previousCount != m_handlers.size();
            if (m_handlersChanged)
                CheckTimerNecessary();
        }

    public:
        void OnTickTimer()
        {
            bool changed = false;

            m_handlersMutex.lock();
            EXT_DEFER(m_handlersMutex.unlock());
            for (size_t index = 0; index < m_handlers.size();)
            {
                auto handler = std::next(m_handlers.begin(), index);

                if (tick_clock::now() - handler->second.lastTickTime >= handler->second.tickInterval)
                {
                    const TickParam tickParam = handler->second.tickParam;
                    ITickHandler* handlerPointer = handler->first;

                    ext::scope::AutoSet changedAutoSet(m_handlersChanged, false, false);

                    // While tick subscriptions may happened
                    {
                        m_handlersMutex.unlock();
                        EXT_DEFER(m_handlersMutex.lock());
                        handlerPointer->OnTick(tickParam);
                    }

                    if (m_handlersChanged)
                    {
                        changed = true;
                        handler = FindTickHandler(handlerPointer, tickParam);
                        if (handler != m_handlers.end())
                            handler->second.lastTickTime = tick_clock::now();
                    }
                    else
                        handler->second.lastTickTime = tick_clock::now();
                }

                ++index;
            }

            if (changed)
                CheckTimerNecessary();
        }

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

        TickHandlersMap::iterator FindTickHandler(ITickHandler* handler, const TickParam& tickParam)
        {
            EXT_ASSERT(!m_handlersMutex.try_lock()) << "Mutex must be locked";
            const auto handlers = m_handlers.equal_range(handler);
            for (auto it = handlers.first; it != handlers.second; ++it)
            {
                if (it->second.tickParam == tickParam)
                    return it;
            }
            return m_handlers.end();
        }

        virtual void StartTimer() = 0;
        virtual void StopTimer() = 0;

    private:
        void CheckTimerNecessary()
        {
            // if handlers exist - timer should work
            if (m_handlers.empty() == m_timerWorks)
            {
                m_timerWorks ? StopTimer() : StartTimer();
                m_timerWorks = !m_timerWorks;
            }
        }

    protected:
        std::atomic_bool m_handlersChanged = false;
        std::atomic_bool m_timerWorks = false;

        std::mutex m_handlersMutex;
        std::multimap<ITickHandler*, TickHandlerInfo> m_handlers;
    };

#ifdef __AFX_H__
    struct InvokedTimer : Timer
    {
        ~InvokedTimer() { if (m_timerWorks) StopTimer(); }

    private:
        void StartTimer() override
        {
            EXT_ASSERT(!m_timerId.has_value());
            ext::InvokeMethod([timerId = &m_timerId]()
            {
                timerId->emplace(::SetTimer(nullptr, NULL, (UINT)kDefTickInterval.count(),
                                 [](HWND, UINT, UINT_PTR, DWORD)
                {
                    get_singleton<TickService>().m_invokedTimer.OnTickTimer();
                }));
            });
            EXT_ASSERT(m_timerId.value_or(0) != 0) << "Timer must be set";
        }

        void StopTimer() override
        {
            if (m_timerId.has_value())
            {
                ::KillTimer(nullptr, *m_timerId);
                m_timerId.reset();
            }
        }

    private:
        std::optional<UINT_PTR> m_timerId;
    } m_invokedTimer;
#endif // __AFX_H__

    struct AsyncTimer : Timer
    {
        ~AsyncTimer() { if (m_timerWorks) StopTimer(); }

    private:
        void StartTimer() override
        {
            EXT_ASSERT(!m_tickThread.joinable());
            m_tickThread.run([&]()
            {
                try
                {
                    while (!m_tickThread.interrupted())
                    {
                        OnTickTimer();
                        ext::this_thread::interruptible_sleep_for(kDefTickInterval);
                    }
                }
                catch (const ext::thread::thread_interrupted& /*interrupted*/)
                {}
            });
        }

        void StopTimer() override
        {
            EXT_ASSERT(m_tickThread.joinable());
            m_tickThread.interrupt_and_join();
        }
    private:
        ext::thread m_tickThread;
    } m_asyncTimer;
};

// Implementation of the base class of the tick handler, simplify tick subscription
struct TickSubscriber : public ITickHandler
{
    TickSubscriber() = default;
    virtual ~TickSubscriber()
    {
        UnsubscribeTimer();
#ifdef __AFX_H__
        UnsubscribeInvokedTimer();
#endif // __AFX_H__
    }

    /// <summary>Add tick handler, tick function will be called async.</summary>
    /// <param name="tickInterval">Tick interval for this parameter.</param>
    /// <param name="tickParam">Parameter passed to the handler on tick, allows you to identify the timer
    /// or pass information to the handler</param>
    void SubscribeTimer(tick_clock::duration tickInterval = TickService::kDefTickInterval, const TickParam& tickParam = 0)
    { get_singleton<TickService>().SubscribeAsync(this, tickInterval, tickParam); }

    /// <summary>Remove async tick timer</summary>
    /// <param name="tickParam">Tick parameter, if null - delete all handler timers.</param>
    void UnsubscribeTimer(const std::optional<TickParam>& tickParam = std::nullopt)
    { get_singleton<TickService>().UnsubscribeAsync(this, tickParam); }

#ifdef __AFX_H__
    /// <summary>Add tick timer, tick function will be called in main UI thread.</summary>
    /// <param name="tickInterval">Tick interval for this parameter.</param>
    /// <param name="tickParam">Parameter passed to the handler on tick, allows you to identify the timer
    /// or pass information to the handler</param>
    void SubscribeInvokedTimer(tick_clock::duration tickInterval = TickService::kDefTickInterval, const TickParam& tickParam = 0)
    { get_singleton<TickService>().SubscribeInvoked(this, tickInterval, tickParam); }

    /// <summary>Remove main UI thread tick timer</summary>
    /// <param name="tickParam">Tick parameter, if null - delete all handler timers</param>
    void UnsubscribeInvokedTimer(const std::optional<TickParam>& tickParam = std::nullopt)
    { get_singleton<TickService>().UnsubscribeInvoked(this, tickParam); }
#endif // __AFX_H__

    /// <summary>Checking if this handler has a timer with the given parameter.</summary>
    /// <param name="tickParam">Tick parameter.</param>
    [[nodiscard]] bool IsTimerExist(const TickParam& tickParam)
    { return get_singleton<TickService>().IsTimerExist(this, tickParam); }
};

} // namespace ext::tick

