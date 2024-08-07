#pragma once

#if _HAS_CXX20 ||  __cplusplus >= 202002L // C++ 20
#error Must not be included, use std::stop_token
#endif

#include <atomic>
#include <list>
#include <functional>
#include <memory>
#include <thread>

#include <ext/core/check.h>

#include <ext/details/atomic_details.h>
#include <ext/details/thread_details.h>

namespace ext::detail {

class stop_callback_control_block
{
public:
    using callback_type = std::function<void()>;

    stop_callback_control_block(callback_type&& callback) noexcept
        : m_callback{std::move(callback)}
    {}

    void invoke_callback() noexcept
    {
        m_callback();

        if (!m_isRemoved)
        {
            m_isRemoved = false;
            m_invokeFinishes = true;
        }
    }

    void mark_as_removed() noexcept
    {
        // Callback executed on this thread or is still currently executing
        // and is deregistering itself from within the callback.
        m_isRemoved = true;
    }

    void wait_until_invoke_finishes() noexcept
    {
        // Callback is currently executing on another thread, block until it finishes executing.

        thread_details::exponential_wait waitForExecution;
        while (!m_invokeFinishes)
        {
            waitForExecution();
        }
    }

private:
    std::atomic_bool m_isRemoved{false};
    std::atomic_bool m_invokeFinishes{false};
    const callback_type m_callback;
};

struct stop_state
{
    bool request_stop() noexcept
    {
        if (!try_lock_and_signal_until_signalled())
        {
            // Stop has already been requested.
            return false;
        }
        // Set the 'stop_requested' signal and acquired the lock.
        m_signallingThread = std::this_thread::get_id();

        while (!m_callbacks.empty())
        {
            // Dequeue the head of the queue
            auto cb = m_callbacks.front();
            m_callbacks.pop_front();
            const bool anyMore = !m_callbacks.empty();

            // Don't hold lock while executing callback
            // so we don't block other threads from deregistering callbacks.
            unlock();

            cb->invoke_callback();

            if (!anyMore)
            {
                // This was the last item in the queue when we dequeued it.
                // No more items should be added to the queue after we have
                // marked the state as interrupted, only removed from the queue.
                // Avoid acquring/releasing the lock in this case.
                return true;
            }

            lock();
        }

        unlock();

        return true;
    }

    [[nodiscard]] bool stop_requested() const noexcept
    {
        return stop_requested(atomic_uint32_load_acquire(&m_state));
    }

    [[nodiscard]] bool stop_requestable() const noexcept
    {
        return stop_requestable(atomic_uint32_load_acquire(&m_state));
    }

    [[nodiscard]] bool try_add_callback(std::shared_ptr<detail::stop_callback_control_block>& cb) noexcept
    {
        auto oldState = atomic_uint32_load_acquire(&m_state);
        do
        {
            while (true)
            {
                if (stop_requested(oldState))
                {
                    cb->invoke_callback();
                    return false;
                }
                if (!stop_requestable(oldState))
                {
                    return false;
                }
                if (!is_locked(oldState))
                {
                    break;
                }
                spin_yield();
                oldState = atomic_uint32_load_acquire(&m_state);
            }
        } while (!atomic_uint32_compare_exchange_weak_acquire_relaxed(&m_state, &oldState, oldState | locked_flag)); // both std::memory_order_acquire?
        // Push callback onto callback list.
        m_callbacks.push_back(cb);

        unlock();

        // Successfully added the callback.
        return true;
    }

    void remove_callback(std::shared_ptr<detail::stop_callback_control_block>& cb) noexcept
    {
        lock();

        // If still registered & not yet executed just remove from the list.
        if (const auto it = std::find(m_callbacks.begin(), m_callbacks.end(), cb); it != m_callbacks.end())
        {
            m_callbacks.erase(it);
            unlock();

            return;
        }
        const auto signallingThread{m_signallingThread};

        unlock();

        // Callback has either already executed or is executing
        // concurrently on another thread.
        if (signallingThread == std::this_thread::get_id())
        {
            // Callback executed on this thread or is still currently executing
            // and is deregistering itself from within the callback.
            cb->mark_as_removed();
        }
        else
        {
            // Callback is currently executing on another thread,
            // block until it finishes executing.
            cb->wait_until_invoke_finishes();
        }
    }

private:
    // bits 0-14 - owner ref count (15 bits)
    // bits 15-29 - source ref count (15 bits)
    // bit 30 - stop-requested
    // bit 31 - locked
    using state_type = std::uint32_t;

    static constexpr state_type owner_ref_increment = 1u;
    static constexpr state_type source_ref_increment = 1u << 15;
    static constexpr state_type stop_requested_flag = 1u << 30;
    static constexpr state_type locked_flag = 1u << 31;

    static constexpr state_type owner_ref_bits = source_ref_increment - 1;
    static constexpr state_type source_ref_bits = (stop_requested_flag - 1) ^ owner_ref_bits;

    [[nodiscard]] static bool is_locked(state_type state) noexcept
    {
        return (state & locked_flag) != 0;
    }

    [[nodiscard]] static bool stop_requested(state_type state) noexcept
    {
        return (state & stop_requested_flag) != 0;
    }

    [[nodiscard]] static bool stop_requestable(state_type state) noexcept
    {
        // Requestable if stop has already been requested or if there are still stop_source instances in existence.
        return stop_requested(state) || (state & source_ref_bits) != 0;
    }

    [[nodiscard]] bool try_lock_and_signal_until_signalled() noexcept
    {
        auto oldState = atomic_uint32_load_acquire(&m_state);
        do
        {
            while (true)
            {
                if (stop_requested(oldState))
                    return false;
                if (!is_locked(oldState))
                    break;
                spin_yield();
                oldState = atomic_uint32_load_acquire(&m_state);
            }
        } while (!atomic_uint32_compare_exchange_weak_acqrel_acquire(&m_state, &oldState, oldState | stop_requested_flag | locked_flag));
        return true;
    }

    void lock() noexcept
    {
        auto oldState = atomic_uint32_load_relaxed(&m_state);
        do
        {
            while (is_locked(oldState))
            {
                spin_yield();
                oldState = atomic_uint32_load_relaxed(&m_state);
            }
        } while (!atomic_uint32_compare_exchange_weak_acquire_relaxed(&m_state, &oldState, oldState | locked_flag));
    }

    void unlock() noexcept
    {
        const auto res = (0 == (locked_flag & atomic_uint32_sub_fetch_release(&m_state, locked_flag)));
        EXT_ASSERT(res);
        if (!res) {
            EXT_UNREACHABLE();
        }
    }

    void spin_yield() noexcept
    {
        ext::thread_details::exponential_wait::FastWait(1);
    }

private:
    state_type m_state{owner_ref_increment + source_ref_increment};
    std::thread::id m_signallingThread{};
    std::list<std::shared_ptr<detail::stop_callback_control_block>> m_callbacks;
};

} // namespace ext::detail
