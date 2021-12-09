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
        void OnTick(::ext::tick::TickParam) SSH_NOEXCEPT override
        {
            ... // execute text each 5 minutes
        }
    };
*/

#include <chrono>
#include <map>
#include <mutex>
#include <optional>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>

#include <ext/thread/invoker.h>

namespace ext {

typedef UINT CallbackId;
constexpr CallbackId kInvalidId = -1;

// Tick service implementation, creates Invoker and Async timers for sending information about ticks to subscribers
// You can set the tick interval (it will work with kDefTickInterval precision)
// see TickSubscriber
class Scheduler : ext::NonCopyable
{
    friend ext::Singleton<Scheduler>;
public://---------------------------------------------------------------------//
    Scheduler();
    ~Scheduler();

    static Scheduler& GlobalInstance();

    CallbackId SubscribeCallbackByPeriod(std::function<void()>&& callback,
                                         const std::chrono::high_resolution_clock::duration& callingPeriod,
                                         CallbackId callbackId = kInvalidId);

    CallbackId SubscribeCallbackAtTime(std::function<void()>&& callback,
                                       const std::chrono::high_resolution_clock::time_point& time,
                                       CallbackId callbackId = kInvalidId);

    SSH_NODISCARD bool IsCallbackExists(CallbackId callbackId) SSH_NOEXCEPT;

    void RemoveCallback(CallbackId callbackId);
private:
    void MainThread();

private:
    struct CallbackInfo;

    std::map<CallbackId, CallbackInfo> m_callbacks;

    std::mutex m_mutexCallbacks;
    std::condition_variable m_cvCallbacks;

    std::atomic_bool m_interrupted = false;
    std::thread m_thread;
};

struct Scheduler::CallbackInfo
{
    std::function<void()> callback;

    std::chrono::high_resolution_clock::time_point nextCallTime;
    std::optional<std::chrono::high_resolution_clock::duration> callingPeriod;

    CallbackInfo(std::function<void()>&& function, std::chrono::high_resolution_clock::duration&& period)
        : callback(function)
        , nextCallTime(std::chrono::high_resolution_clock::now())
        , callingPeriod(period)
    {}

    CallbackInfo(std::function<void()>&& function, std::chrono::high_resolution_clock::time_point&& callTime) SSH_THROWS()
        : callback(function)
        , nextCallTime(callTime)
    {
        SSH_ASSERT(nextCallTime > std::chrono::high_resolution_clock::now());
    }
};

inline Scheduler::Scheduler(): m_thread(&MainThread, this)
{}

inline Scheduler::~Scheduler()
{
    SSH_ASSERT(m_thread.joinable());
    m_interrupted = true;
    m_cvCallbacks.notify_all();
    m_thread.join();
}

inline Scheduler& Scheduler::GlobalInstance()
{
    static Scheduler globalScheduler;
    return globalScheduler;
}

inline CallbackId Scheduler::SubscribeCallbackByPeriod(std::function<void()>&& callback,
                                                       const std::chrono::high_resolution_clock::duration&
                                                       callingPeriod, CallbackId callbackId)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexCallbacks);

        if (callbackId == kInvalidId || m_callbacks.find(callbackId) != m_callbacks.end())
            callbackId = m_callbacks.empty() ? 0 : (m_callbacks.rbegin()->first + 1);

        SSH_DUMP_IF(!m_callbacks.try_emplace(callbackId, std::move(callback), callingPeriod).second);
    }
    m_cvCallbacks.notify_one();
    return callbackId;
}

inline CallbackId Scheduler::SubscribeCallbackAtTime(std::function<void()>&& callback,
                                                     const std::chrono::high_resolution_clock::time_point& time,
                                                     CallbackId callbackId)
{
    {
        std::lock_guard<std::mutex> lock(m_mutexCallbacks);

        if (callbackId == kInvalidId || m_callbacks.find(callbackId) != m_callbacks.end())
            callbackId = m_callbacks.empty() ? 0 : (m_callbacks.rbegin()->first + 1);

        SSH_DUMP_IF(!m_callbacks.try_emplace(callbackId, std::move(callback), time).second);
    }
    m_cvCallbacks.notify_one();
    return callbackId;
}

inline bool Scheduler::IsCallbackExists(CallbackId callbackId) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutexCallbacks);
    return m_callbacks.find(callbackId) != m_callbacks.end();
}

inline void Scheduler::RemoveCallback(CallbackId callbackId)
{
    SSH_EXPECT(callbackId != kInvalidId);
    {
        std::lock_guard<std::mutex> lock(m_mutexCallbacks);
        const auto it = m_callbacks.find(callbackId);
        if (it == m_callbacks.end())
            return;

        m_callbacks.erase(it);
    }

    m_cvCallbacks.notify_one();
}

inline void Scheduler::MainThread()
{
    while (!m_interrupted)
    {
        std::function<void()> callBack = nullptr;
        {
            std::unique_lock<std::mutex> lk(m_mutexCallbacks);

            // wait for scheduled tasks
            while (m_callbacks.empty())
            {
                m_cvCallbacks.wait(lk);

                // notification call can be connected with interrupting
                if (m_interrupted)
                    return;
            }

            const auto it = std::min_element(m_callbacks.begin(), m_callbacks.end(),
                                             [](const auto &a, const auto &b)
                                             {
                                                 return a.second.nextCallTime < b.second.nextCallTime;
                                             });

            const auto nextCallTime = it->second.nextCallTime;
            if (nextCallTime <= std::chrono::high_resolution_clock::now() ||
                m_cvCallbacks.wait_until(lk, nextCallTime) == std::cv_status::timeout)
            {
                if (!it->second.callingPeriod.has_value())
                {
                    callBack = std::move(it->second.callback);
                    m_callbacks.erase(it);
                }
                else
                {
                    it->second.nextCallTime = std::chrono::high_resolution_clock::now() + *it->second.callingPeriod;
                    callBack = it->second.callback;
                }
            }
        }

        // выполняем задание
        if (callBack)
            callBack();
    }
}

} // namespace ext