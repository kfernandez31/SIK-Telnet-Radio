#pragma once

#include <memory>
#include <mutex>

/**
 * @class ProtectedResource
 * @brief A thread-safe wrapper around a resource.
 *
 * Provides controlled access to an underlying value of type `T`
 * by enforcing mutual exclusion via an internal `std::mutex`.
 *
 * This class is useful for synchronizing access to shared data in a
 * multithreaded environment.
 *
 * @tparam T The type of the wrapped resource.
 */
template <typename T>
struct ProtectedResource {
private:
    T _val;                  ///< The protected resource.
    mutable std::mutex _mtx; ///< Mutex for synchronizing access.

public:
    /**
     * @brief Constructs a ProtectedResource with the given arguments.
     * @tparam Args Variadic template parameters for constructing `_val`.
     * @param args Arguments forwarded to the constructor of `T`.
     */
    template<typename... Args>
    ProtectedResource(Args&&... args) : _val(std::forward<Args>(args)...) {}

    /**
     * @brief Locks the mutex and returns a `std::unique_lock` for scoped synchronization.
     * @return A `std::unique_lock<std::mutex>` guarding the resource.
     */
    std::unique_lock<std::mutex> lock() {
        return std::unique_lock<std::mutex>(_mtx);
    }

    T& value()             { return _val; }
    const T& value() const { return _val; }
};

/**
 * @class SyncedPtr
 * @brief A thread-safe reference-counted smart pointer.
 *
 * This class acts similarly to Rust's `Arc<Mutex<T>>`, providing
 * shared ownership of a `ProtectedResource<T>`. It enables multiple
 * threads to access and synchronize a shared resource.
 *
 * @tparam T The type of the wrapped resource.
 */
template <typename T>
struct SyncedPtr {
private:
    std::shared_ptr<ProtectedResource<T>> _ptr; ///< Shared pointer to the protected resource.

    explicit SyncedPtr(const std::shared_ptr<ProtectedResource<T>>& ptr) : _ptr(ptr) {}

public:
    SyncedPtr() : _ptr(std::make_shared<ProtectedResource<T>>()) {}

    SyncedPtr(const SyncedPtr<T>& other) : _ptr(other._ptr) {}

    /**
     * @brief Factory function for creating a `SyncedPtr` with an initialized resource, working similarly to `std::make_shared<T>`.
     * @tparam Args Variadic template parameters for constructing the resource.
     * @param args Arguments forwarded to the constructor of `T`.
     * @return A new `SyncedPtr<T>` instance.
     */
    template<typename... Args>
    static SyncedPtr<T> make(Args&&... args) {
        return {std::make_shared<ProtectedResource<T>>(std::forward<Args>(args)...)};
    }

    T& operator*()             { return _ptr->value(); }
    const T& operator*() const { return _ptr->value(); }

    T* operator->()             { return &_ptr->value(); }
    const T* operator->() const { return &_ptr->value(); }

    /**
     * @brief Locks the underlying mutex and returns a `std::unique_lock` for scoped synchronization.
     * @return A `std::unique_lock<std::mutex>` guarding the resource.
     */
    std::unique_lock<std::mutex> lock() { return _ptr->lock(); }
};