#pragma once

/*
 * Interruptible thread implementation(boost/thread analog)
 *
 * Allow to create thread and check in any function is it interrupted and not check any flags or something everywhere
 *
 * Example:

#include <ext/thread/thread.h>

ext::thread myThread(thread_function, []()
{
    const ext::stop_token token = ext::this_thread::get_stop_token();
    const ext::stop_callback callback(token, []() { std::cout << "Stopped"; });

    while (!ext::this_thread::interruption_requested())
    {
        ...
    }

    if (token.stop_requested())
        ...
});

myThread.interrupt();
EXPECT_TRUE(myThread.interrupted());
 */
#include <atomic>
#include <chrono>
#include <future> //  _Fake_no_copy_callable_adapter
#include <mutex>
#include <thread>
#include <shared_mutex>
#include <unordered_map>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/singleton.h>

#include <ext/error/dump_writer.h>

#include <ext/thread/event.h>
#include <ext/thread/stop_token.h>

#include <ext/utils/thread_details.h>

namespace ext {
namespace this_thread {

using namespace std::this_thread;

// Getting current ext::thread stop_token
EXT_NODISCARD inline ext::stop_token get_stop_token() EXT_NOEXCEPT;
// Interruption point for ext::thread function, if thread interrupted - throws a ext::thread::thread_interrupted
inline void interruption_point() EXT_THROWS(ext::thread::thread_interrupted());
// Check if current ext:thread has been interrupted
EXT_NODISCARD inline bool interruption_requested() EXT_NOEXCEPT;
// Sleep until time period, if the thread is interrupted, throws an exception
template <class _Clock, class _Duration>
void interruptible_sleep_until(const std::chrono::time_point<_Clock, _Duration>& absoluteTime) EXT_THROWS(ext::thread::thread_interrupted());
// Sleep for time duration, if the thread is interrupted, throws an exception
template <class _Rep, class _Period>
void interruptible_sleep_for(const std::chrono::duration<_Rep, _Period>& timeDuration) EXT_THROWS(ext::thread::thread_interrupted());

} // namespace this_thread

// predefine thread pool class, it need access to restore_interrupted function
class thread_pool;

class thread : std::thread
{
    typedef std::thread base;
    inline static const std::thread::id kInvalidThreadId;

public:
    // constructors from std::thread
    thread() EXT_NOEXCEPT = default;
    explicit thread(base&& other) EXT_NOEXCEPT      { replace(std::move(other)); }
    explicit thread(thread&& other) EXT_NOEXCEPT    { replace(std::move(other)); }

    template<class _Fn, class... _Args>
    explicit thread(_Fn&& _Fx, _Args&&... _Ax)
    {
        run(std::forward<_Fn>(_Fx), std::forward<_Args>(_Ax)...);
    }

    thread& operator=(base&& other) EXT_NOEXCEPT    { replace(std::move(other)); return *this; }
    thread& operator=(thread&& other) EXT_NOEXCEPT  { replace(std::move(other)); return *this; }

    ~thread() EXT_NOEXCEPT
    {
        if (joinable())
            OnDestroyThread(get_id());
    }

    // Run function for execution in current thread
    template<class _Fn, class... _Args>
    void run(_Fn&& function, _Args&&... arguments) EXT_NOEXCEPT;

    void detach()
    {
        DetachThread();
        EXT_ASSERT(!joinable()) << EXT_TRACE_FUNCTION "Service must detach us";
    }

    void join()
    {
        const auto currentThreadId = get_id();
        base::join();
        OnDestroyThread(currentThreadId);
    }

    EXT_NODISCARD inline ext::stop_token get_token() const EXT_NOEXCEPT
    {
        return m_stopSource.get_token();
    }

    using base::get_id;
    using base::hardware_concurrency;
    using base::joinable;
    using base::native_handle;

public:
    // Special exception about thread interruption
    class thread_interrupted {};

    // interrupt current thread function
    void interrupt() EXT_THROWS()
    {
        if (!m_stopSource.request_stop())
            return; // already requested
        EXT_EXPECT(joinable()) << EXT_TRACE_FUNCTION "No function call for execution in this thread";
        OnInterrupt();
    }

    // check if thread has been interrupted
    EXT_NODISCARD inline bool interrupted() const EXT_NOEXCEPT
    {
        return m_stopSource.stop_requested();
    }

    // interrupt thread and wait for join
    void interrupt_and_join() EXT_THROWS() { interrupt(); if (joinable()) join(); }

    // check if thread function is executing
    EXT_NODISCARD bool thread_works() const EXT_NOEXCEPT
    {
        if (joinable())
        {
            return WaitForSingleObject(const_cast<thread*>(this)->native_handle(), 0) != WAIT_OBJECT_0;
            /*if (DWORD retCode = 0; GetExitCodeThread(native_handle(), &retCode) != 0)
                return retCode == STILL_ACTIVE;*/
        }
        return false;
    }

    // try join until time point
    template <class Clock, class Duration>
    bool try_join_until(const std::chrono::time_point<Clock, Duration>& time) EXT_THROWS();

    // try join for duration
    template<class Rep, class Period>
    bool try_join_for(const std::chrono::duration<Rep, Period>& duration) EXT_THROWS()
    {
        return try_join_until(std::_To_absolute_time(duration));
    }

private:
    void replace(base&& other) EXT_NOEXCEPT;
    void replace(thread&& other) EXT_NOEXCEPT;

private:
    // restore thread after interrupting
    void restore_interrupted() EXT_THROWS()
    {
        EXT_EXPECT(interrupted()) << EXT_TRACE_FUNCTION "Not interrupted yet";
        ext::stop_source temp;
        m_stopSource.swap(temp);
        OnRestoreInterrupted();
    }

    inline void OnRestoreInterrupted() const;
    inline void OnInterrupt() const;
    inline void OnDestroyThread(const ext::thread::id& id) const;
    inline void DetachThread();

private:
    // global storage of information about interrupted ext::threads
    class ThreadsManager;
    EXT_NODISCARD inline static ThreadsManager& manager();

    // for access to restore_interrupted
    friend class thread_pool;

    friend ext::stop_token this_thread::get_stop_token() EXT_NOEXCEPT;
    friend bool this_thread::interruption_requested() EXT_NOEXCEPT;

    template <class _Clock, class _Duration>
    friend void this_thread::interruptible_sleep_until(const std::chrono::time_point<_Clock, _Duration>& absoluteTime) EXT_THROWS(ext::thread::thread_interrupted());

private:
    ext::stop_source m_stopSource;
};

class thread::ThreadsManager
{
    friend ext::Singleton<ThreadsManager>;

    struct WorkingThreadInfo
    {
        explicit WorkingThreadInfo(ext::stop_token&& token) EXT_NOEXCEPT
            : stopToken(std::move(token))
        {}

        EXT_NODISCARD bool interrupted() const EXT_NOEXCEPT
        {
            return stopToken.stop_requested();
        }
        void on_interrupt()
        {
            EXT_ASSERT(interrupted());
            interruptionEvent->Set();
        }
        void restore_interrupted(ext::stop_token&& token)
        {
            EXT_ASSERT(interrupted());
            stopToken = std::move(token);
            interruptionEvent->Reset();
        }
        const std::shared_ptr<ext::Event> interruptionEvent = std::make_shared<ext::Event>(true);
        ext::stop_token stopToken;
    };
    // map with working ext::threads and their interruption events
    std::unordered_map<std::thread::id, WorkingThreadInfo> m_workingThreadsInterruptionEvents;
    mutable std::shared_mutex m_workingThreadsMutex;

    // list of detached threads
    mutable std::shared_mutex m_detachedInterruptedThreadsMutex;
    std::unordered_map<std::thread::id, ext::thread> m_detachedInterruptedThreads;

public:
    // Call this function when you move std::thead into ext::thread for register it inside manager
    void OnStartThread(const std::thread::id& id, ext::stop_token&& token) EXT_NOEXCEPT
    {
        EXT_ASSERT(id != kInvalidThreadId);
        {
            std::unique_lock lock(m_workingThreadsMutex);
            m_workingThreadsInterruptionEvents.try_emplace(id, std::move(token));
        }
        CleanupDetachedThreads();
    }

    // Call this function when you ext::thread destroying
    void OnDestroyThread(const std::thread::id& id) EXT_NOEXCEPT
    {
        EXT_ASSERT(id != kInvalidThreadId);
        {
            std::unique_lock lock(m_workingThreadsMutex);
            EXT_ASSERT(m_workingThreadsInterruptionEvents.find(id) != m_workingThreadsInterruptionEvents.end());
            m_workingThreadsInterruptionEvents.erase(id);
        }
        CleanupDetachedThreads();
    }

    // Call this function for detaching ext::thread
    void DetachThread(ext::thread& thread)
    {
        if (thread.thread_works())
        {
            {
                std::unique_lock interruptMutex(m_workingThreadsMutex);
                EXT_ASSERT(m_workingThreadsInterruptionEvents.find(thread.get_id()) != m_workingThreadsInterruptionEvents.end());
                m_workingThreadsInterruptionEvents.erase(thread.get_id());
            }

            if (thread.interrupted() && thread.thread_works())
            {
                {
                    std::unique_lock detachedMutex(m_detachedInterruptedThreadsMutex);
                    EXT_ASSERT(m_detachedInterruptedThreads.find(thread.get_id()) == m_detachedInterruptedThreads.end());
                    m_detachedInterruptedThreads.emplace(thread.get_id(), std::move(thread));
                }
                CleanupDetachedThreads();
                return;
            }
        }
        else if (thread.joinable())
        {
            // thread finished but not joined
            EXT_ASSERT(m_workingThreadsInterruptionEvents.find(thread.get_id()) != m_workingThreadsInterruptionEvents.end());
            m_workingThreadsInterruptionEvents.erase(thread.get_id());
        }

        static_cast<std::thread&>(thread).detach();
        CleanupDetachedThreads();
    }

    // Call this function for interrupting thread by thread id
    void OnInterrupt(const std::thread::id& id) EXT_NOEXCEPT
    {
        EXT_ASSERT(id != kInvalidThreadId);

        std::unique_lock lock(m_workingThreadsMutex);
        auto threadId = m_workingThreadsInterruptionEvents.find(id);
        if (threadId != m_workingThreadsInterruptionEvents.end())
            threadId->second.on_interrupt();
        else
            EXT_ASSERT(false) << "Thread not registered yet";
    }

    // Call this function for restore interrupted thread by thread id
    void OnRestoreInterrupted(const ext::thread& thread) EXT_NOEXCEPT
    {
        const auto id = thread.get_id();
        EXT_ASSERT(id != kInvalidThreadId);

        std::unique_lock lock(m_workingThreadsMutex);
        EXT_ASSERT(m_workingThreadsInterruptionEvents.find(id) != m_workingThreadsInterruptionEvents.end());
        m_workingThreadsInterruptionEvents.at(id).restore_interrupted(thread.get_token());
    }

    // Call this function for check if thread interrupted by thread id
    EXT_NODISCARD bool IsInterrupted(const std::thread::id& id) const EXT_NOEXCEPT
    {
        EXT_ASSERT(id != kInvalidThreadId);
        {
            std::shared_lock lock(m_workingThreadsMutex);
            if (const auto it = m_workingThreadsInterruptionEvents.find(id); it != m_workingThreadsInterruptionEvents.end())
                return it->second.interrupted();
        }
        std::shared_lock detachedMutex(m_detachedInterruptedThreadsMutex);
        return m_detachedInterruptedThreads.find(id) != m_detachedInterruptedThreads.end();
    }

    // Call this function for try getting interruption event for thread by id
    EXT_NODISCARD std::shared_ptr<ext::Event> TryGetInterruptionEvent(const std::thread::id& id) EXT_NOEXCEPT
    {
        EXT_ASSERT(id != kInvalidThreadId);
        {
            std::shared_lock lock(m_workingThreadsMutex);
            if (const auto it = m_workingThreadsInterruptionEvents.find(id); it != m_workingThreadsInterruptionEvents.end())
                return it->second.interruptionEvent;
        }
        {
            std::shared_lock detachedMutex(m_detachedInterruptedThreadsMutex);
            if (m_detachedInterruptedThreads.find(id) == m_detachedInterruptedThreads.end())
                return nullptr;
        }

        auto res = std::make_shared<ext::Event>();
        res->Create();
        res->Set();
        return res;
    }

    EXT_NODISCARD ext::stop_token GetStopToken(const std::thread::id& id) EXT_NOEXCEPT
    {
        EXT_ASSERT(id != kInvalidThreadId);
        {
            std::shared_lock lock(m_workingThreadsMutex);
            if (const auto it = m_workingThreadsInterruptionEvents.find(id);
                it != m_workingThreadsInterruptionEvents.end())
                return it->second.stopToken;
        }
        {
            std::shared_lock detachedMutex(m_detachedInterruptedThreadsMutex);
            if (const auto it = m_detachedInterruptedThreads.find(id);
                it != m_detachedInterruptedThreads.end())
                return it->second.get_token();
        }

        EXT_ASSERT(false) << "Not ext::thread";
        return {};
    }

private:
    ~ThreadsManager()
    {
        std::for_each(m_detachedInterruptedThreads.begin(), m_detachedInterruptedThreads.end(), [](auto& threadPair)
                      {
                          threadPair.second.detach();
                      });
    }

    void CleanupDetachedThreads()
    {
        std::unique_lock detachedMutex(m_detachedInterruptedThreadsMutex);
        for (auto it = m_detachedInterruptedThreads.begin(), end = m_detachedInterruptedThreads.end(); it != end;)
        {
            if (!it->second.thread_works())
            {
                // avoid call OnDestroyThread for already detached threads and reset thread id
                static_cast<std::thread&>(it->second).join();
                it = m_detachedInterruptedThreads.erase(it);
                end = m_detachedInterruptedThreads.end();
            }
            else
                ++it;
        }
    }
};

EXT_NODISCARD inline ext::thread::ThreadsManager& thread::manager()
{
    return ext::get_service<ThreadsManager>();
}

template<class _Fn, class... _Args>
void thread::run(_Fn&& function, _Args&&... arguments) EXT_NOEXCEPT
{
    // @see replace(std::thread&&)
    if (joinable())
        detach();

    if (interrupted())
    {
        ext::stop_source temp;
        m_stopSource.swap(temp);
    }

    // after setting stop source we can receive current stop token correctly
    // allows to write thread.run(&ThisClass::ExecuteFunction, this);
    typedef std::_Fake_no_copy_callable_adapter<_Fn, _Args...> Adapter;
    base::operator=(base([functionCaller = Adapter(std::forward<_Fn>(function),
                                                   std::forward<_Args>(arguments)...)]
                                                   (ext::stop_token token)
    {
        // 'function' can ask for interruption point or stop token before the end of the 'run' function
        manager().OnStartThread(this_thread::get_id(), std::move(token));
        const_cast<Adapter&>(functionCaller)();
    }, get_token()));

    if (joinable())
        manager().OnStartThread(get_id(), get_token());
}

inline void thread::replace(base&& other) EXT_NOEXCEPT
{
    if (joinable())
        detach();

    if (interrupted())
    {
        ext::stop_source temp;
        m_stopSource.swap(temp);
    }
    base::operator=(std::move(other));

    if (joinable())
        manager().OnStartThread(get_id(), get_token());
}

inline void thread::replace(thread&& other) EXT_NOEXCEPT
{
    if (joinable())
        detach();

    m_stopSource = std::move(other.m_stopSource);
    base::operator=(std::move(other));
}

template <class Clock, class Duration>
bool thread::try_join_until(const std::chrono::time_point<Clock, Duration>& time) EXT_THROWS()
{
    EXT_ASSERT(time < Clock::now());
    EXT_EXPECT(std::this_thread::get_id() != get_id()) << "Trying joining itself, deadlock occur!";

    thread_details::exponential_wait waitForJoin;
    while (Clock::now() < time)
    {
        if (!thread_works())
        {
            join();
            return true;
        }

        waitForJoin();
    }

    return !thread_works();
}

inline void thread::OnRestoreInterrupted() const
{
    manager().OnRestoreInterrupted(*this);
}

inline void thread::OnInterrupt() const
{
    manager().OnInterrupt(get_id());
}

inline void thread::OnDestroyThread(const ext::thread::id& id) const
{
    manager().OnDestroyThread(id);
}

inline void thread::DetachThread()
{
    manager().DetachThread(*this);
}

namespace this_thread {

// Check if current ext:thread has been interrupted
EXT_NODISCARD inline ext::stop_token get_stop_token() EXT_NOEXCEPT
{
    return ::ext::thread::manager().GetStopToken(ext::this_thread::get_id());
}

// Interruption point for ext::thread function, if thread interrupted - throws a ext::thread::thread_interrupted
void interruption_point() EXT_THROWS(ext::thread::thread_interrupted())
{
    if (interruption_requested())
        throw ::ext::thread::thread_interrupted();
}

// Check if current ext:thread has been interrupted
EXT_NODISCARD inline bool interruption_requested() EXT_NOEXCEPT
{
    return ::ext::thread::manager().IsInterrupted(ext::this_thread::get_id());
}

template <class _Clock, class _Duration>
void interruptible_sleep_until(const std::chrono::time_point<_Clock, _Duration>& timePoint) EXT_THROWS(ext::thread::thread_interrupted())
{
#if _HAS_CXX20
    static_assert(std::chrono::is_clock_v<_Clock>, "Clock type required");
#endif // _HAS_CXX20

    if (const auto interruptionEvent = ::ext::thread::manager().TryGetInterruptionEvent(ext::this_thread::get_id()))
    {
        auto _Now = _Clock::now();
        if (timePoint <= _Now)
            return;

        try
        {
            if (!interruptionEvent->Wait(timePoint - _Now))
                return;
        }
        catch (...)
        {}

        EXT_ASSERT(interruption_requested());
        throw ext::thread::thread_interrupted();
    }

    std::this_thread::sleep_until(timePoint);
}

template <class _Rep, class _Period>
void interruptible_sleep_for(const std::chrono::duration<_Rep, _Period>& duration) EXT_THROWS(ext::thread::thread_interrupted())
{
    interruptible_sleep_until(std::_To_absolute_time_custom(duration));
}

} // namespace this_thread
} // namespace ext
