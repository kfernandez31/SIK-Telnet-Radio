#pragma once

#include <memory>
#include <mutex>

template <typename T>
struct ProtectedResource {
private:
    T _val;
    mutable std::mutex _mtx;
public:
    template<typename... Args>
    ProtectedResource(Args&&... args) : _val(std::forward<Args>(args)...) {}

    std::unique_lock<std::mutex> lock() {
        return std::unique_lock<std::mutex>(_mtx);
    }

    T& value() {
        return _val;
    }

    const T& value() const {
        return _val;
    }
};

template <typename T>
struct SyncedPtr {
private:
    std::shared_ptr<ProtectedResource<T>> _ptr;

    SyncedPtr(const std::shared_ptr<ProtectedResource<T>>& ptr) : _ptr(ptr) {}

public:
    template<typename... Args>
    static SyncedPtr<T> make(Args&&... args) {
        return SyncedPtr<T>(std::make_shared<ProtectedResource<T>>(std::forward<Args>(args)...));
    }
    
    SyncedPtr() : _ptr(std::make_shared<ProtectedResource<T>>()) {}
    SyncedPtr(const SyncedPtr<T>& other) : _ptr(other._ptr) {}

    T& operator*() {
        return _ptr->value();
    }

    const T& operator*() const {
        return _ptr->value();
    }

    T* operator->() {
        return &_ptr->value();
    }

    const T* operator->() const {
        return &_ptr->value();
    }

    std::unique_lock<std::mutex> lock() {
        return _ptr->lock();
    }
};
