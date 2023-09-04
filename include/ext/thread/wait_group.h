/*
Realization of channel concept from GoLang. 

ext::WaitGroup wg;
wg.add();
auto thread = std::thread([&]()
    {
        EXT_DEFER({ wg.done(); });
        ...
    });
// will wait till the end of the thread
wg.done();
*/

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

#include <ext/core/defines.h>
#include <ext/core/noncopyable.h>

namespace ext {

class WaitGroup : ext::NonCopyable {
private:
    std::atomic_int_fast64_t m_counter = 0;
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;

public:
    WaitGroup() = default;

    void add(size_t delta = 0) EXT_NOEXCEPT {
        m_counter += delta;
    }

    void done() EXT_NOEXCEPT {
        if (m_counter.fetch_sub(1) == 1) {
            m_cv.notify_all();
        }
    }

    void wait() const EXT_NOEXCEPT {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&]() { return m_counter == 0; });
    }
};

} // namespace ext