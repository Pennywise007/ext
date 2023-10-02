/*
Example:

ext::stop_source source;
ext::thread myThread([stop_token = source.get_token()]()
{
    while (!stop_token.stop_requested())
    {
        ...
    }
});

source.request_stop();
myThread.join();
*/

#pragma once

#if defined(__cplusplus) && __cplusplus >= 202004L

#include <stop_source>

namespace ext {

using stop_token = ::std::stop_token;
using stop_source = ::std::stop_source;
template <typename Callback>
using stop_callback = ::std::stop_callback<Callback>;

} // namespace ext

#else // no C++20

#include <algorithm>

#include <ext/core/defines.h>
#include <ext/details/stop_token_details.h>

namespace ext {

class stop_source;

template <typename Callback>
class stop_callback;

// @see https://en.cppreference.com/w/cpp/thread/stop_token
class stop_token
{
public:
    stop_token() noexcept = default;
    stop_token(stop_token &&) noexcept = default;
    stop_token(const stop_token &) noexcept = default;
    stop_token &operator=(stop_token &&) noexcept = default;
    stop_token &operator=(const stop_token &) noexcept = default;

private:
    explicit stop_token(const std::shared_ptr<ext::detail::stop_state>& state) noexcept
        : m_state(state)
    {}

public:
    void swap(stop_token &other) noexcept
    {
        std::swap(m_state, other.m_state);
    }

public:
    /// <summary>
    ///     Checks if the `stop_token` object has associated stop-state and that state has received a stop request.
    ///     A default constructed `stop_token` has no associated stop-state, and thus has not had stop requested.
    /// </summary>
    /// <returns>
    ///     @retval true if the `stop_token` object has associated stop-state and it received a stop request.
    ///     @retval false otherwise.
    /// </returns>
    [[nodiscard]] bool stop_requested() const noexcept
    {
        return m_state && m_state->stop_requested();
    }

    /// <summary>
    ///     Checks if the `stop_token` object has associated stop-state, and that state either
    ///     has already had a stop requested or it has associated `stop_source` object(s).
    ///     A default constructed stop_token has no associated stop-state, and thus cannot be stopped;
    ///     associated stop-state for which no `stop_source` object(s) exist can also not be stopped
    ///     if such a request has not already been made.
    /// </summary>
    /// <returns>
    ///     @retval true if the `stop_token` object has associated stop-state and a stop request has already been made.
    ///     @retval false otherwise.
    /// </returns>
    [[nodiscard]] bool stop_possible() const noexcept
    {
        return m_state && m_state->stop_requestable();
    }

public:
    [[nodiscard]] friend bool operator==(const stop_token &a, const stop_token &b) noexcept
    {
        return a.m_state.get() == b.m_state.get();
    }
    [[nodiscard]] friend bool operator!=(const stop_token &a, const stop_token &b) noexcept { return !operator==(a, b); }

private:
    friend class stop_source;

    template <typename Callback>
    friend class stop_callback;

private:
    std::shared_ptr<ext::detail::stop_state> m_state;
};

// @see https://en.cppreference.com/w/cpp/thread/stop_source
class stop_source
{
public:
    stop_source() noexcept {} // compiller doesn`t like = default
    stop_source(stop_source&&) noexcept = default;
    stop_source(const stop_source&) noexcept = default;
    stop_source& operator=(stop_source&&) noexcept = default;
    stop_source& operator=(const stop_source&) noexcept = default;

public:
    /// <summary>
    ///     Issues a stop request to the stop-state, if the `stop_source` object has stop-state
    ///     and it has not yet already had stop requested.
    /// </summary>
    /// <remarks>
    ///     If the request_stop() does issue a stop request (i.e., returns true), then any stop_callbacks
    ///     registered for the same associated stop - state will be invoked synchronously, on the same thread request_stop() is issued on.
    ///     If an invocation of a callback exits via an exception, std::terminate is called.
    ///
    ///     If the `stop_source` object has stop-state but a stop request has already been made, this function returns false.
    ///     However there is no guarantee that another `stop_source` object which has just (successfully) requested stop
    ///     is not still in the middle of invoking a stop_callback function.
    /// </remarks>
    /// <returns>
    ///     @retval true if the `stop_source` object has stop-state and this invocation made a stop request.
    ///     @retval false otherwise.
    /// </returns>
    bool request_stop() noexcept
    {
        if (m_state)
            return m_state->request_stop();
        return false;
    }

    /// <summary>
    ///     Returns a `stop_token` object associated with the `stop_source`'s stop-state, if the `stop_source` has stop-state;
    ///     otherwise returns a default-constructed (empty) stop_token.
    /// </summary>
    [[nodiscard]] stop_token get_token() const noexcept
    {
        return stop_token(m_state);
    }

    /// <summary>Checks if the stop_source object has stop - state and that state has received a stop request.</summary>
    /// <returns>
    ///     @retval true if the `stop_source` object has stop-state and it received a stop request.
    ///     @retval false otherwise.
    /// </returns>
    [[nodiscard]] bool stop_requested() const noexcept
    {
        return m_state && m_state->stop_requested();
    }

    /// <summary>Checks if the `stop_source` object has stop-state.</summary>
    /// <returns>
    ///     @retval true if the `stop_source` object has stop-state.
    ///     @retval false otherwise.
    /// </returns>
    [[nodiscard]] bool stop_possible() const noexcept
    {
        return !!m_state;
    }

    [[nodiscard]] explicit operator bool() const noexcept
    {
        return stop_possible();
    }

    void swap(stop_source& other) noexcept
    {
        std::swap(m_state, other.m_state);
    }

public:
    [[nodiscard]] friend bool operator==(const stop_source &a, const stop_source &b) noexcept
    {
        return a.m_state.get() == b.m_state.get();
    }

    [[nodiscard]] friend bool operator!=(const stop_source &a, const stop_source &b) noexcept
    {
        return a.m_state.get() != b.m_state.get();
    }

private:
    std::shared_ptr<ext::detail::stop_state> m_state = std::make_shared<ext::detail::stop_state>();
};

// @see https://en.cppreference.com/w/cpp/thread/stop_callback
template <typename Callback>
class stop_callback
{
public:
    stop_callback(stop_callback &&) = delete;
    stop_callback(const stop_callback &) = delete;
    stop_callback &operator=(stop_callback &&) = delete;
    stop_callback &operator=(const stop_callback &) = delete;

public:
    static_assert(std::is_void_v<decltype(std::declval<Callback>()())>, "Callback should return `void`");

    template <typename StopToken>
    constexpr explicit stop_callback(StopToken&& token, Callback&& callback) noexcept
        : m_controlBlock{ std::make_shared<detail::stop_callback_control_block>([callback]() { callback(); }) }
        , m_state{ std::forward<StopToken>(token).m_state }
    {
        if (!m_state->try_add_callback(m_controlBlock))
            m_state = nullptr;
    }

    ~stop_callback() noexcept
    {
        if (m_state)
            m_state->remove_callback(m_controlBlock);
    }

private:
    std::shared_ptr<detail::stop_callback_control_block> m_controlBlock;
    std::shared_ptr<detail::stop_state> m_state;
};

} // namespace ext

#endif // C++20
