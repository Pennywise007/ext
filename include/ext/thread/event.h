#pragma once

#include <chrono>
#include <condition_variable>
#include <optional>
#include <mutex>

namespace ext {

struct Event
{
    // Raise of the single event, only one context who is waiting this event will be awaked.
    // After wait is done the event will be reseted
    void RaiseOne() noexcept
    {
        std::unique_lock<std::mutex> lock(mutex_);
        state_ = State::eRaisedOne;
        cv_.notify_one();
    }

    // Raise all contexts who wait for the event
    void RaiseAll() noexcept
    {
        std::unique_lock<std::mutex> lock(mutex_);
        state_ = State::eRaisedAll;
        cv_.notify_all();
    }

    // Reset event state, set the event state to not raised
    void Reset() noexcept
    {
        std::unique_lock<std::mutex> lock(mutex_);
        state_ = State::eNotRaised;
    }

    static constexpr auto INFINITY_WAIT = std::nullopt;
    /// <summary> Wait until the set of the event object for the specified duration </summary>
    /// <param name="timeout">Waiting timeout, infinite if not installed</param>
    /// <returns> true if signal raised, false if timeout expired</returns>
    bool Wait(const std::optional<std::chrono::steady_clock::duration>& timeout = INFINITY_WAIT)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (timeout.has_value())
        {
            if (cv_.wait_for(lock, *timeout, [&] { return state_ != State::eNotRaised; }))
            {
                if (state_ == State::eRaisedOne)
                    state_ = State::eNotRaised;
                return true;
            }
            else
                return false;
        }
        else
        {
            cv_.wait(lock, [&] { return state_ != State::eNotRaised; });
            if (state_ == State::eRaisedOne)
                state_ = State::eNotRaised;
            return true;
        }
    }

    /// Check if event was raised
    [[nodiscard]] bool Raised() const noexcept
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return state_ != State::eNotRaised;
    }

private:
    std::condition_variable cv_;
    mutable std::mutex mutex_;

    enum class State {
        eNotRaised,         
        eRaisedOne,
        eRaisedAll,
    } state_ = State::eNotRaised;
};

} // namespace ext
