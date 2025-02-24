// Copyright (c) Chemical Language Foundation 2025.

#pragma once

template <typename T>
class hybrid_ptr {
private:
    std::unique_ptr<T> ptr;
    bool should_free;

public:
    // Constructor taking a raw pointer
    explicit hybrid_ptr(T* raw_ptr = nullptr, bool should_free = true)
            : ptr(raw_ptr), should_free(should_free) {}

    // Disable copy constructor and assignment operator
    hybrid_ptr(const hybrid_ptr&) = delete;
    hybrid_ptr& operator=(const hybrid_ptr&) = delete;

    // Enable move constructor and assignment operator
    hybrid_ptr(hybrid_ptr&& other) noexcept
            : ptr(std::move(other.ptr)), should_free(other.should_free) {
        other.should_free = false;
    }

    hybrid_ptr& operator=(hybrid_ptr&& other) noexcept {
        if(other) {
            if(ptr.get() == other.get() || !should_free) {
                ptr.release();
            }
            ptr = std::move(other.ptr);
            should_free = other.should_free;
            other.should_free = false;
        }
        return *this;
    }

    bool get_will_free() {
        return should_free;
    }

    // Destructor
    ~hybrid_ptr() {
        if (!should_free) {
            ptr.release();
        }
    }

    // Functions to control whether to free or not
    void do_free() { should_free = true; }
    void do_not_free() { should_free = false; }

    // Access the raw pointer
    T* get() const { return ptr.get(); }
    T* release() { return ptr.release(); }
    void reset(T* raw_ptr = nullptr) { ptr.reset(raw_ptr); }

    // Dereference operators
    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr.get(); }

    // Check if the pointer is not null
    explicit operator bool() const { return static_cast<bool>(ptr); }
};