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
    while (!ext::this_thread::interruption_requested())
    {
        ...
    }
});

myThread.interrupt();
EXPECT_TRUE(myThread.interrupted());
 */
#include <atomic>
#include <thread>
#include <chrono>
#include <shared_mutex>
#include <unordered_map>

#include <ext/core/defines.h>
#include <ext/core/check.h>
#include <ext/core/singleton.h>
#include <ext/error/dump_writer.h>

#include <ext/thread/event.h>

namespace std {

// remove in v142 toolset VS 2019
template <class _Rep, class _Period>
_NODISCARD auto _To_absolute_time_custom(const chrono::duration<_Rep, _Period>& _Rel_time) noexcept {
    constexpr auto _Zero                 = chrono::duration<_Rep, _Period>::zero();
    const auto _Now                      = chrono::steady_clock::now();
    decltype(_Now + _Rel_time) _Abs_time = _Now; // return common type
    if (_Rel_time > _Zero) {
        constexpr auto _Forever = (chrono::steady_clock::time_point::max)();
        if (_Abs_time < _Forever - _Rel_time) {
            _Abs_time += _Rel_time;
        } else {
            _Abs_time = _Forever;
        }
    }
    return _Abs_time;
}
}

namespace ext {
namespace this_thread {

using namespace std::this_thread;

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

class thread : public std::thread
{
    typedef std::thread base;
    inline static const std::thread::id kInvalidThreadId;

public:
    // constructors from std::thread
    thread() EXT_NOEXCEPT = default;
    explicit thread(base&& other) EXT_NOEXCEPT
        : base(std::forward<base>(other))
    {
        if (joinable())
            ext::get_service<InterruptionManager>().OnStartThread(get_id());
    }
    explicit thread(thread&& other) EXT_NOEXCEPT
        : base(std::forward<base>(other))
        , m_interrupted(static_cast<bool>(other.m_interrupted))
    {}

    template<class _Fn, class... _Args>
    explicit thread(_Fn&& _Fx, _Args&&... _Ax)
        : base([_Fx, _Ax...]()
        {
            // register thread before call function to avoid problemS with interruptible checks inside call function
            ext::get_service<InterruptionManager>().OnStartThread(std::this_thread::get_id());
            _Fx(std::move(_Ax)...);
        })
    {}

    thread& operator=(base&& other) EXT_NOEXCEPT
    {
        if (joinable())
            ext::get_service<InterruptionManager>().DetachThread(*this);
        base::operator=(std::forward<base>(other));
        m_interrupted = false;
        if (joinable())
            ext::get_service<InterruptionManager>().OnStartThread(get_id());
        return *this;
    }

    thread& operator=(thread&& other) EXT_NOEXCEPT
    {
        if (joinable())
            ext::get_service<InterruptionManager>().DetachThread(*this);
        m_interrupted = (bool)other.m_interrupted;
        base::operator=(std::forward<base>(other));
        return *this;
    }

    void swap(base& other)   EXT_NOEXCEPT { operator=(std::move(other)); }
    void swap(thread& other) EXT_NOEXCEPT { operator=(std::move(other)); }

    // Run function for execution in current thread
    template<class _Fn, class... _Args>
    void run(_Fn&& _Fx, _Args&&... _Ax) EXT_NOEXCEPT
    {
        base temp(std::forward<_Fn>(_Fx), std::forward<_Args>(_Ax)...);
        swap(temp);
    }

    ~thread() EXT_NOEXCEPT
    {
        if (joinable())
            ext::get_service<InterruptionManager>().OnDestroyThread(get_id());
    }

    void detach()
    {
        ext::get_service<InterruptionManager>().DetachThread(*this);
        EXT_ASSERT(!joinable()) << EXT_TRACE_FUNCTION "Service must detach us";
    }

    void join()
    {
        const auto currentThreadId = get_id();
        base::join();
        ext::get_service<InterruptionManager>().OnDestroyThread(currentThreadId);
    }

public:
    // Special exception about thread interruption
    class thread_interrupted {};

    // interrupt current thread function
    void interrupt() EXT_THROWS()
    {
        if (m_interrupted)
            return;
        m_interrupted = true;
        EXT_EXPECT(joinable()) << EXT_TRACE_FUNCTION "No function call for execution in this thread";
        ext::get_service<InterruptionManager>().Interrupt(get_id());
    }

    // restore thread after interrupting
    void restore_interrupted() EXT_THROWS()
    {
        EXT_EXPECT(m_interrupted) << EXT_TRACE_FUNCTION "Not interrupted yet";
        m_interrupted = false;
        ext::get_service<InterruptionManager>().RestoreInterrupted(get_id());
    }

    // check if thread has been interrupted
    EXT_NODISCARD bool interrupted() const EXT_THROWS()
    {
        return m_interrupted;
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
    bool try_join_until(const std::chrono::time_point<Clock, Duration>& time) EXT_THROWS()
    {
        EXT_ASSERT(time < Clock::now());
        EXT_EXPECT(std::this_thread::get_id() != get_id()) << "Trying joining itself, deadlock occur!";

        while (Clock::now() < time)
        {
            if (!thread_works())
            {
                join();
                return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        return !thread_works();
    }

    // try join for duration
    template<class Rep, class Period>
    bool try_join_for(const std::chrono::duration<Rep, Period>& duration) EXT_THROWS()
    {
        return try_join_until(std::_To_absolute_time(duration));
    }

private:
    // variable for faster deduction of interruption thread state
    std::atomic_bool m_interrupted = false;

    // global storage of information about interrupted ext::threads
    class InterruptionManager
    {
        friend ext::Singleton<InterruptionManager>;

        enum class ThreadStatus
        {
            eWorking,
            eDetached,
            eJoined
        };

        // map with working ext::threads and their interruption events
        std::unordered_map<std::thread::id, std::shared_ptr<ext::Event>> m_workingThreadsInterruptionEvents;
        mutable std::shared_mutex m_workingThreadsMutex;

        // list of detached threads
        mutable std::shared_mutex m_detachedInterruptedThreadsMutex;
        std::unordered_map<std::thread::id, ext::thread> m_detachedInterruptedThreads;

    public:
        // Call this function when you move std::thead into ext::thread for register it inside manager
        void OnStartThread(const std::thread::id& id) EXT_NOEXCEPT
        {
            EXT_ASSERT(id != kInvalidThreadId);
            {
                std::unique_lock lock(m_workingThreadsMutex);
                auto emplaceRes = m_workingThreadsInterruptionEvents.emplace(id, std::make_shared<ext::Event>());
                EXT_ASSERT(emplaceRes.second) << "Double start of thread with same id " << id;
                emplaceRes.first->second->Create(true);
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

        // Call this function for check if thread interrupted by thread id
        EXT_NODISCARD bool IsInterrupted(const std::thread::id& id) const EXT_NOEXCEPT
        {
            EXT_ASSERT(id != kInvalidThreadId);
            {
                std::shared_lock lock(m_workingThreadsMutex);
                if (const auto it = m_workingThreadsInterruptionEvents.find(id); it != m_workingThreadsInterruptionEvents.end())
                    return !*it->second;
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
                    return it->second;
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

        // Call this function for interrupting thread by thread id
        void Interrupt(const std::thread::id& id) EXT_NOEXCEPT
        {
            EXT_ASSERT(id != kInvalidThreadId);

            std::unique_lock lock(m_workingThreadsMutex);
            auto threadId = m_workingThreadsInterruptionEvents.find(id);
            if (threadId != m_workingThreadsInterruptionEvents.end())
            {
                auto& interruptionEvent = threadId->second;
                EXT_ASSERT(interruptionEvent);
                interruptionEvent->Set();
                interruptionEvent->Destroy();
            }
            else
                EXT_ASSERT(false) << "Thread not registered yet";
        }

        // Call this function for restore interrupted thread by thread id
        void RestoreInterrupted(const std::thread::id& id) EXT_NOEXCEPT
        {
            EXT_ASSERT(id != kInvalidThreadId);

            std::unique_lock lock(m_workingThreadsMutex);
            EXT_ASSERT(m_workingThreadsInterruptionEvents.find(id) != m_workingThreadsInterruptionEvents.end());
            auto& interruptionEvent = m_workingThreadsInterruptionEvents.at(id);
            EXT_ASSERT(interruptionEvent);
            interruptionEvent->Create(true);
        }

    private:
        ~InterruptionManager()
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

    friend bool this_thread::interruption_requested() EXT_NOEXCEPT;

    template <class _Clock, class _Duration>
    friend void this_thread::interruptible_sleep_until(const std::chrono::time_point<_Clock, _Duration>& absoluteTime) EXT_THROWS(ext::thread::thread_interrupted());
};

namespace this_thread {

// Interruption point for ext::thread function, if thread interrupted - throws a ext::thread::thread_interrupted
void interruption_point() EXT_THROWS(ext::thread::thread_interrupted())
{
    if (interruption_requested())
        throw ext::thread::thread_interrupted();
}

// Check if current ext:thread has been interrupted
EXT_NODISCARD inline bool interruption_requested() EXT_NOEXCEPT
{
    return ext::get_service<::ext::thread::InterruptionManager>().IsInterrupted(ext::this_thread::get_id());
}

template <class _Clock, class _Duration>
void interruptible_sleep_until(const std::chrono::time_point<_Clock, _Duration>& timePoint) EXT_THROWS(ext::thread::thread_interrupted())
{
#if _HAS_CXX20
    static_assert(std::chrono::is_clock_v<_Clock>, "Clock type required");
#endif // _HAS_CXX20

    if (const auto interruptionEvent = ext::get_service<::ext::thread::InterruptionManager>().TryGetInterruptionEvent(ext::this_thread::get_id()))
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
