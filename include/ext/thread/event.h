#pragma once

#include <chrono>
#include <condition_variable>
#include <optional>
#include <mutex>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>
#include <ext/core/check.h>

#include <ext/scope/on_exit.h>

namespace ext {

struct Event
{
    /// <param name="manualReset">True if event will be reset manually by Reset function,
    /// False to allow system automatically resets the event state to nonsignaled after a single waiting thread has been released </param>
    explicit Event() EXT_NOEXCEPT = default;

    void Set(bool notifyAll = false) EXT_NOEXCEPT
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_signaled = true;
        if (notifyAll)
            m_cv.notify_all();
        else
            m_cv.notify_one();
    }

    // Reset event state, set the event state to non signaled
    void Reset() EXT_NOEXCEPT
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_signaled = false;
    }

    static constexpr auto INFINITY_WAIT = std::nullopt;
    /// <summary> Wait until the set of the event object for the specified duration </summary>
    /// <param name="timeout">Waiting timeout, infinite if not installed</param>
    /// <returns> true if signal raised, false if timeout expired</returns>
    bool Wait(const std::optional<std::chrono::steady_clock::duration>& timeout = INFINITY_WAIT)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (timeout.has_value())
        {
            if (m_cv.wait_for(lock, *timeout, [&] { return m_signaled; }))
                return true;
            else
                return false;
        }
        else
        {
            m_cv.wait(lock, [&] { return m_signaled; });
            return true;
        }
    }

    /// <summary> Wait until the set of the event object for the specified duration </summary>
    /// <param name="timeout">Waiting timeout, infinite if not installed</param>
    /// <returns> true if signal raised, false if timeout expired</returns>
    EXT_NODISCARD bool Raised() EXT_NOEXCEPT
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_signaled;
    }

private:
    std::condition_variable m_cv;
    mutable std::mutex m_mutex;
    bool m_signaled = false;
};

} // namespace ext
