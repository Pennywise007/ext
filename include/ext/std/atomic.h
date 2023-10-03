#pragma once

#include <atomic>

#if !_HAS_CXX20
#include <memory>
#include <type_traits>

namespace std {

template<typename T>
struct atomic<shared_ptr<T>> {
    constexpr atomic() noexcept = default;
    atomic(const atomic&) = delete;
    atomic& operator=(const atomic&) = delete;

    shared_ptr<T> load(memory_order order = memory_order_seq_cst) const noexcept {
        return atomic_load_explicit(&ptr, order);
    }

    void store(shared_ptr<T> desired, memory_order order = memory_order_seq_cst) noexcept {
        atomic_store_explicit(&ptr, desired, order);
    }

    shared_ptr<T> exchange(shared_ptr<T> desired, memory_order order = memory_order_seq_cst) noexcept {
        return atomic_exchange_explicit(&ptr, desired, order);
    }

    bool compare_exchange_weak(shared_ptr<T>& expected, shared_ptr<T> desired,
                               memory_order success = memory_order_seq_cst,
                               memory_order failure = memory_order_seq_cst) noexcept {
        return atomic_compare_exchange_weak_explicit(&ptr, &expected, desired, success, failure);
    }

    bool compare_exchange_strong(shared_ptr<T>& expected, shared_ptr<T> desired,
                                 memory_order success = memory_order_seq_cst,
                                 memory_order failure = memory_order_seq_cst) noexcept {
        return atomic_compare_exchange_strong_explicit(&ptr, &expected, desired, success, failure);
    }

    bool is_lock_free() const noexcept {
        return atomic_is_lock_free(&ptr);
    }

    shared_ptr<T> operator++(int) noexcept = delete;
    shared_ptr<T> operator--(int) noexcept = delete;
    shared_ptr<T> operator++() noexcept = delete;
    shared_ptr<T> operator--() noexcept = delete;

    void operator+=(ptrdiff_t) noexcept = delete;
    void operator-=(ptrdiff_t) noexcept = delete;

    void operator&=(shared_ptr<T>) noexcept = delete;
    void operator|=(shared_ptr<T>) noexcept = delete;
    void operator^=(shared_ptr<T>) noexcept = delete;

    atomic(shared_ptr<T> desired) noexcept : ptr(std::move(desired)) {}

    operator shared_ptr<T>() const noexcept {
        return load();
    }

    atomic& operator=(shared_ptr<T> desired) noexcept {
        store(desired);
        return *this;
    }

    shared_ptr<T> operator->() const noexcept {
        return load();
    }

    T& operator*() const noexcept {
        return *load();
    }

private:
    shared_ptr<T> ptr;
};
#endif // !_HAS_CXX20

} // namespace ext