/*
Realization of channel concept from GoLang. 
Allows to create a queue of data and read it asynchroniously

 ext::Channel<int> channel;

std::thread([&]()
    {
        for (auto val : channel) {
            ...
        }
    });
channel.add(1);
channel.add(10);
channel.close();
*/

#pragma once

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <optional>
#include <mutex>
#include <queue>

#include <ext/core/defines.h>

namespace ext {

template <typename T>
class Channel {
private:
    mutable std::mutex m_queue_mutex;
    mutable std::condition_variable m_queue_not_full;
    mutable std::condition_variable m_queue_not_empty;
    std::queue<T> m_queue;
    const size_t m_max_size;
    std::atomic<bool> m_closed = false;

    class ChannelIterator {
    public:
        using value_type = T;

    private:
        Channel* m_channel;
        std::optional<value_type> m_value;

        constexpr ChannelIterator(Channel* channel, std::optional<value_type>&& value = std::nullopt)
            : m_channel(channel)
            , m_value(std::move(value))
        {}

        friend class Channel;
    public:
        ChannelIterator(const ChannelIterator& other) = delete;
        constexpr explicit ChannelIterator(ChannelIterator&& other) EXT_NOEXCEPT
            : m_channel(other.m_channel)
            , m_value(std::move(other.m_value))
        {
            other.m_value = std::nullopt;
        }

        constexpr EXT_NODISCARD bool operator==(const ChannelIterator& other) const {
            return m_channel == other.m_channel && !m_value.has_value() && !other.m_value.has_value();
        }

        constexpr EXT_NODISCARD bool operator!=(const ChannelIterator& other) const {
            return !operator==(other);
        }

        constexpr EXT_NODISCARD const value_type& operator*() const { return m_value.value(); }

        constexpr EXT_NODISCARD value_type& operator*() { return m_value.value(); }

        constexpr EXT_NODISCARD value_type* operator->() { return &m_value.value(); }

        constexpr EXT_NODISCARD const value_type* operator->() const { return &m_value.value(); }

        ChannelIterator& operator++() EXT_THROWS(std::bad_function_call) {
            if (!m_value.has_value()) {
                throw std::bad_function_call();
            }
            m_value = m_channel->get();
            return *this;
        }
    };
public:
    using iterator = ChannelIterator;

    Channel(size_t size = 1) 
        : m_max_size(size)
    {}

    void close() {
        std::lock_guard<std::mutex> lock(m_queue_mutex);
        m_closed = true;
        m_queue_not_full.notify_all();
        m_queue_not_empty.notify_all();
    }

    template <typename ...Args>
    void add(Args&& ...args) EXT_THROWS(std::bad_function_call) {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_queue_not_full.wait(lock, [&]() {
            return (m_queue.size() < m_max_size) || m_closed;
        });
        std::lock_guard<std::mutex> guard(*lock.release(), std::adopt_lock);
        if (m_closed) {
            throw std::bad_function_call();
        }
        m_queue.emplace(std::forward<Args>(args)...);
        m_queue_not_empty.notify_one();
    }
    
    EXT_NODISCARD std::optional<T> get() EXT_NOEXCEPT
    {
        std::unique_lock<std::mutex> lock(m_queue_mutex);
        m_queue_not_empty.wait(lock, [this] { return !m_queue.empty() || m_closed; });

        std::lock_guard<std::mutex> guard(*lock.release(), std::adopt_lock);
        if (m_closed && m_queue.empty()) {
            return std::nullopt;
        }
        const auto result = std::move(m_queue.front());
        m_queue.pop();
        m_queue_not_full.notify_one();
        return result;
    }

    EXT_NODISCARD iterator begin() { return ChannelIterator(this, get()); }
    EXT_NODISCARD iterator end() { return ChannelIterator(this, std::nullopt); }
};

} // namespace dadrian
