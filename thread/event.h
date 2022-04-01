#pragma once

#include <chrono>
#include <handleapi.h>
#include <optional>
#include <Windows.h>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>
#include <ext/core/check.h>

#include <ext/scope/on_exit.h>

namespace ext {

struct Event : private ext::NonCopyable
{
    /// <summary> Initialize of event object </summary>
    /// <param name="manualReset">True if event will be reset manually by Reset function,
    /// False to allow system automatically resets the event state to nonsignaled after a single waiting thread has been released </param>
    void Create(bool manualReset = false) EXT_THROWS(std::system_error)
    {
        EXT_ASSERT(!m_handle) << "Handle already exist";
        m_handle = ::CreateEvent(nullptr, manualReset ? TRUE : FALSE, FALSE, nullptr);
        EXT_ASSERT(m_handle);
        if (!m_handle)
            throw std::system_error(::GetLastError(), std::system_category(), "Failed to create event");
    }

    // Destroy event object
    void Destroy() { m_handle.DestroyObject(); }

    // Signal event object so wait thread is about to release
    void Set() EXT_THROWS(std::system_error, ::ext::check::CheckFailedException)
    {
        EXT_EXPECT(m_handle) << "Handle uninitialized";
        if (!::SetEvent(m_handle))
            throw std::system_error(::GetLastError(), std::system_category(), "Failed to create event");
    }

    // Reset event state, set the event state to nonsignaled
    void Reset() EXT_THROWS(std::system_error)
    {
        EXT_EXPECT(m_handle) << "Handle uninitialized";
        if (!::ResetEvent(m_handle))
            throw std::system_error(::GetLastError(), std::system_category(), "Failed to create event");
    }

    /// <summary> Wait until the set of the event object for the specified duration </summary>
    /// <param name="timeout">Waiting timeout, infinite if not installed</param>
    /// <returns> true if signal raised, false if timeout expired</returns>
    bool Wait(const std::optional<std::chrono::steady_clock::duration>& timeout = std::nullopt) const EXT_THROWS(std::system_error, ::ext::check::CheckFailedException)
    {
        const DWORD timeoutValue = timeout.has_value() ? (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(*timeout).count() : INFINITE;
        switch (const auto res = ::WaitForSingleObjectEx(m_handle, timeoutValue, FALSE))
        {
        case WAIT_OBJECT_0:
            return true;
        case WAIT_TIMEOUT:
            return false;
        case WAIT_ABANDONED:
            return false;
        default:
            EXT_ASSERT(res == WAIT_FAILED);
            throw std::system_error(::GetLastError(), std::system_category(), "Wait failed");;
        }
    }

    /// <summary> Wait until the set of the event object for the specified duration </summary>
    /// <param name="timeout">Waiting timeout, infinite if not installed</param>
    /// <returns> true if signal raised, false if timeout expired</returns>
    EXT_NODISCARD bool Raised() const EXT_THROWS(std::system_error, ::ext::check::CheckFailedException)
    {
        if (m_handle)
            return Wait(std::chrono::milliseconds(0));
        return false;
    }

    EXT_NODISCARD constexpr bool operator!() const EXT_NOEXCEPT { return !m_handle; }
    EXT_NODISCARD constexpr operator bool() const EXT_NOEXCEPT { return m_handle; }

    // Get event handle
    EXT_NODISCARD constexpr HANDLE& GetHandle() { return m_handle; }
    EXT_NODISCARD constexpr const HANDLE& GetHandle() const { return m_handle; }

private:
    ext::scope::ObjectHolder<HANDLE, decltype(&::CloseHandle)> m_handle = { &::CloseHandle, nullptr };
};

} // namespace ext
