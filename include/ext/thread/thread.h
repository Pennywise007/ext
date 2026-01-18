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
#include <mutex>
#include <memory>
#include <thread>
#include <shared_mutex>
#include <unordered_map>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/singleton.h>

#include <ext/error/dump_writer.h>

#include <ext/thread/event.h>
#include <ext/thread/stop_token.h>

#include <ext/details/thread_details.h>

#include <ext/utils/invoke.h>

#if !(defined(_WIN32) || defined(__CYGWIN__)) // not windows
#include <pthread.h>
#endif

namespace ext {

namespace core {
// For Init function to be able to init a thread manager
void Init();
} // namespace core

namespace this_thread {

using namespace std::this_thread;

// Getting current ext::thread stop_token
[[nodiscard]] inline ext::stop_token get_stop_token() noexcept;
// Interruption point for ext::thread function, if thread interrupted - throws a ext::thread::thread_interrupted
inline void interruption_point() EXT_THROWS(ext::thread::thread_interrupted());
// Check if current ext:thread has been interrupted
[[nodiscard]] inline bool interruption_requested() noexcept;
// Sleep until time period, if the thread is interrupted, throws an exception
template <class _Clock, class _Duration>
void interruptible_sleep_until(const std::chrono::time_point<_Clock, _Duration>& absoluteTime) EXT_THROWS(ext::thread::thread_interrupted());
// Sleep for time duration, if the thread is interrupted, throws an exception
template <class _Rep, class _Period>
void interruptible_sleep_for(const std::chrono::duration<_Rep, _Period>& timeDuration) EXT_THROWS(ext::thread::thread_interrupted());
// Sleep untill time point
template <class _Clock, class _Duration>
void sleep_until(const std::chrono::time_point<_Clock, _Duration>& absoluteTime);
// Sleep for time duration
template <class _Rep, class _Period>
inline void sleep_for(const std::chrono::duration<_Rep, _Period>& timeDuration);

} // namespace this_thread

// predefine thread pool class, it need access to restore_interrupted function
class thread_pool;

class thread : std::thread
{
    typedef std::thread base;
    inline static const std::thread::id kInvalidThreadId;

public:
    // constructors from std::thread
    thread() noexcept = default;
    explicit thread(thread&& other) noexcept    { operator=(std::move(other)); }

    template<class _Function, class... _Args>
    explicit thread(_Function&& function, _Args&&... args)
        : thread(ext::stop_source(), std::forward<_Function>(function), std::forward<_Args>(args)...)
    {}

    thread& operator=(thread&& other) noexcept;

    // Run function for execution in current thread
    template<class _Function, class... _Args>
    void run(_Function&& function, _Args&&... args) noexcept;

    [[nodiscard]] inline ext::stop_token get_token() const noexcept { return m_stopSource.get_token(); }

    using base::detach;
    using base::join;
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
        EXT_EXPECT(joinable()) << EXT_TRACE_FUNCTION << "No function call for execution in this thread";
        OnInterrupt();
    }

    // check if thread has been interrupted
    [[nodiscard]] inline bool interrupted() const noexcept
    {
        return m_stopSource.stop_requested();
    }

    // interrupt thread and wait for join
    void interrupt_and_join() EXT_THROWS() { interrupt(); if (joinable()) join(); }

    // check if thread function is executing
    [[nodiscard]] bool thread_works() const noexcept
    {
        if (joinable())
        {
#if defined(_WIN32) || defined(__CYGWIN__) // windows
            return WaitForSingleObject(const_cast<thread*>(this)->native_handle(), 0) != WAIT_OBJECT_0;
            /*if (DWORD retCode = 0; GetExitCodeThread(native_handle(), &retCode) != 0)
                return retCode == STILL_ACTIVE;*/
#else
            pthread_t pthreadId = const_cast<thread*>(this)->native_handle();
            if (pthreadId != 0 && pthread_tryjoin_np(pthreadId, nullptr) == EBUSY)
                return true;
#endif
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
        return try_join_until(std::_To_absolute_time_custom(duration));
    }

private:
    // internal constructor which used to optimize stop source usage
    template<class _Function, class... _Args>
    explicit thread(ext::stop_source&& source, _Function&& function, _Args&&... arguments);

    // wrapper of an execution function into an invoker. Allows to reduce a number of possible arguments copies
    template<class _Function, class... _Args>
    [[nodiscard]] base create_thread(ext::stop_token&& token, _Function&& function, _Args&&... args);

private:
    // restore thread after interrupting
    void restore_interrupted() EXT_THROWS()
    {
        EXT_EXPECT(interrupted()) << EXT_TRACE_FUNCTION << "Not interrupted yet";
        ext::stop_source temp;
        m_stopSource.swap(temp);
        OnRestoreInterrupted();
    }

    inline void OnRestoreInterrupted() const;
    inline void OnInterrupt() const;

private:
    // global storage of information about interrupted ext::threads
    class ThreadsManager;
    [[nodiscard]] inline static ThreadsManager& manager();

    // for access to restore_interrupted
    friend class thread_pool;

    friend ext::stop_token this_thread::get_stop_token() noexcept;
    friend bool this_thread::interruption_requested() noexcept;

    template <class _Rep, class _Period>
    friend void this_thread::interruptible_sleep_for(const std::chrono::duration<_Rep, _Period>& duration) EXT_THROWS(ext::thread::thread_interrupted());

    friend void core::Init();

private:
    ext::stop_source m_stopSource;
};

class thread::ThreadsManager
{
    friend ext::Singleton<ThreadsManager>;

    struct WorkingThreadInfo
    {
        explicit WorkingThreadInfo(ext::stop_token&& token) noexcept
            : stopToken(std::move(token))
        {
            // We might interrupt thread before calling a thread function
            if (stopToken.stop_requested())
                interruptionEvent->RaiseAll();
        }

        [[nodiscard]] bool interrupted() const noexcept
        {
            return stopToken.stop_requested();
        }
        void on_interrupt()
        {
            EXT_ASSERT(interrupted());
            interruptionEvent->RaiseAll();
        }
        void restore_interrupted(ext::stop_token&& token)
        {
            EXT_ASSERT(interrupted());
            stopToken = std::move(token);
            interruptionEvent->Reset();
        }
        const std::shared_ptr<ext::Event> interruptionEvent = std::make_shared<ext::Event>();
        ext::stop_token stopToken;
    };
    // map with working ext::threads and their interruption events
    std::unordered_map<std::thread::id, WorkingThreadInfo> m_workingThreadsInterruptionEvents;
    mutable std::shared_mutex m_workingThreadsMutex;

public:
    ThreadsManager() noexcept = default;

    // Notification about thread starting
    void OnStartingThread(const std::thread::id& id, ext::stop_token&& token) noexcept
    {
        EXT_ASSERT(id != kInvalidThreadId);
        
        std::unique_lock lock(m_workingThreadsMutex);
        EXT_DUMP_IF(!m_workingThreadsInterruptionEvents.try_emplace(id, std::move(token)).second) << "Double thread registration";
    }

    // Notification about finishing thread
    void OnFinishingThread(const std::thread::id& id) noexcept
    {
        EXT_ASSERT(id != kInvalidThreadId);

        std::unique_lock lock(m_workingThreadsMutex);
        EXT_DUMP_IF(m_workingThreadsInterruptionEvents.erase(id) == 0) << "Finishing unregistered thread";
    }

    // Call this function for interrupting thread by thread id
    void OnInterrupt(const ext::thread& thread) noexcept
    {
        auto id = thread.get_id();
        EXT_ASSERT(id != kInvalidThreadId);

        std::unique_lock lock(m_workingThreadsMutex);
        const auto threadIt = m_workingThreadsInterruptionEvents.find(id);
        // the thread might be interrupted before calling a thread function
        if (threadIt != m_workingThreadsInterruptionEvents.end())
            threadIt->second.on_interrupt();
        else
            EXT_ASSERT(false) << "Interrupting non registered thread";
    }

    // Call this function for restore interrupted thread by thread id
    void OnRestoreInterrupted(const ext::thread& thread) noexcept
    {
        const auto id = thread.get_id();
        EXT_ASSERT(id != kInvalidThreadId);

        std::unique_lock lock(m_workingThreadsMutex);
        auto it = m_workingThreadsInterruptionEvents.find(id);
        if (it != m_workingThreadsInterruptionEvents.end())
            it->second.restore_interrupted(thread.get_token());
        else
            EXT_ASSERT(false) << "Trying to restoring not registered thread";
    }

    // Call this function for check if thread interrupted by thread id
    [[nodiscard]] bool IsInterrupted(const std::thread::id& id) const noexcept
    {
        EXT_ASSERT(id != kInvalidThreadId);
        std::shared_lock lock(m_workingThreadsMutex);
        if (const auto it = m_workingThreadsInterruptionEvents.find(id); it != m_workingThreadsInterruptionEvents.end())
            return it->second.interrupted();

        EXT_ASSERT(false) << "Not ext::thread";
        return false;
    }

    // Getting interruption event for thread by id
    [[nodiscard]] std::shared_ptr<ext::Event> GetInterruptionEvent(const std::thread::id& id) noexcept
    {
        EXT_ASSERT(id != kInvalidThreadId);
        std::shared_lock lock(m_workingThreadsMutex);
        if (const auto it = m_workingThreadsInterruptionEvents.find(id); it != m_workingThreadsInterruptionEvents.end())
            return it->second.interruptionEvent;
        else
            EXT_ASSERT(false) << "Trying to get an interruption event from a non ext::thread";
        return nullptr;
    }

    [[nodiscard]] ext::stop_token GetStopToken(const std::thread::id& id) noexcept
    {
        EXT_ASSERT(id != kInvalidThreadId);
        {
            std::shared_lock lock(m_workingThreadsMutex);
            if (const auto it = m_workingThreadsInterruptionEvents.find(id);
                it != m_workingThreadsInterruptionEvents.end())
                return it->second.stopToken;
        }

        EXT_ASSERT(false) << "Not ext::thread";
        return {};
    }
};

[[nodiscard]] inline ext::thread::ThreadsManager& thread::manager()
{
    return ext::get_singleton<ThreadsManager>();
}

template<class _Function, class... _Args>
[[nodiscard]] thread::base thread::create_thread(ext::stop_token&& token, _Function&& function, _Args&&... arguments)
{
    class ThreadRegistrator {
    public:
        ThreadRegistrator() = default;
        ~ThreadRegistrator() 
        {
            manager().OnFinishingThread(threadId);
        }

        void RegisterThread(std::thread::id id, ext::stop_token&& token)
        {
            std::call_once(flag, [&, id, token = std::move(token)]() mutable
            {
                threadId = std::move(id);
                manager().OnStartingThread(threadId, std::move(token));
            });
        }

    private:
        std::thread::id threadId;
        std::once_flag flag;
    };

    auto threadRegistrator = std::make_shared<ThreadRegistrator>();

    using Invoker = ext::ThreadInvoker<_Function, _Args...>;
    auto thread = base([invoker = Invoker(std::forward<_Function>(function), std::forward<_Args>(arguments)...),
                        threadRegistrator]
        (ext::stop_token token) mutable
        {
            threadRegistrator->RegisterThread(this_thread::get_id(), std::move(token));
            invoker();
        }, token);

    threadRegistrator->RegisterThread(thread.get_id(), std::move(token));
    return thread;
}

template<class _Function, class... _Args>
thread::thread(ext::stop_source&& source, _Function&& function, _Args&&... arguments)
    : base(create_thread(source.get_token(), std::forward<_Function>(function), std::forward<_Args>(arguments)...))
    , m_stopSource(std::move(source))
{}

template<class _Function, class... _Args>
void thread::run(_Function&& function, _Args&&... arguments) noexcept
{
    // @see replace(std::thread&&)
    if (joinable())
        detach();

    if (interrupted())
    {
        ext::stop_source temp;
        m_stopSource.swap(temp);
    }

    base::operator=(create_thread(get_token(), std::forward<_Function>(function), std::forward<_Args>(arguments)...));
}

inline thread& thread::operator=(thread&& other) noexcept
{
    // @see replace(std::thread&&)
    if (joinable())
        detach();

    m_stopSource = std::move(other.m_stopSource);
    base::operator=(std::move(static_cast<base&>(other)));
    return *this;
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
    manager().OnInterrupt(*this);
}

namespace this_thread {

// Check if current ext:thread has been interrupted
[[nodiscard]] inline ext::stop_token get_stop_token() noexcept
{
    return ::ext::thread::manager().GetStopToken(ext::this_thread::get_id());
}

// Interruption point for ext::thread function, if thread interrupted - throws a ext::thread::thread_interrupted
// NOTE: slow method, use ext::this_thread::get_stop_token() and check if stop_requested
void interruption_point() EXT_THROWS(ext::thread::thread_interrupted())
{
    if (interruption_requested())
        throw ::ext::thread::thread_interrupted();
}

// Check if current ext:thread has been interrupted
[[nodiscard]] inline bool interruption_requested() noexcept
{
    return ::ext::thread::manager().IsInterrupted(ext::this_thread::get_id());
}

template <class _Clock, class _Duration>
void interruptible_sleep_until(const std::chrono::time_point<_Clock, _Duration>& timePoint) EXT_THROWS(ext::thread::thread_interrupted())
{
#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20
    static_assert(std::chrono::is_clock_v<_Clock>, "Clock type required");
#endif // C++20
    ext::this_thread::interruptible_sleep_for(timePoint - _Clock::now());
}

template <class _Rep, class _Period>
void interruptible_sleep_for(const std::chrono::duration<_Rep, _Period>& duration) EXT_THROWS(ext::thread::thread_interrupted())
{
    if (const auto interruptionEvent = ::ext::thread::manager().GetInterruptionEvent(ext::this_thread::get_id()))
    {
        if (duration.count() < 0)
            return;

        try
        {
            if (!interruptionEvent->Wait(duration))
                return;
        }
        catch (...)
        {}

        EXT_ASSERT(interruption_requested());
        throw ext::thread::thread_interrupted();
    }

    ext::this_thread::sleep_for(duration);
}

template <class _Clock, class _Duration>
void sleep_until(const std::chrono::time_point<_Clock, _Duration>& timePoint)
{
#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++20
    static_assert(std::chrono::is_clock_v<_Clock>, "Clock type required");
#endif // C++20
    ext::this_thread::sleep_for(timePoint - _Clock::now());
}

template <class _Rep, class _Period>
void sleep_for(const std::chrono::duration<_Rep, _Period>& duration)
{
    ext::thread_details::sleep_for(duration);
}

} // namespace this_thread
} // namespace ext
