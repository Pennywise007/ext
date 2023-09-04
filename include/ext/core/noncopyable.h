#pragma once

namespace ext {

/// <summary>
/// Class NonCopyable is a base class.  Derive your own class from
/// NonCopyable when you want to prohibit copy construction and copy assignment.
/// </summary>
class NonCopyable
{
protected:
    NonCopyable() noexcept = default;
    ~NonCopyable() noexcept = default;

    NonCopyable(const NonCopyable&) = delete;
    NonCopyable(NonCopyable&&) noexcept = default;

    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable& operator=(NonCopyable&&) noexcept = default;
};

} // namespace ext
