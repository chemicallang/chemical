// Copyright (c) Chemical Language Foundation 2025.
//
// A joinable thread created with an explicit stack size.
//
// The compiler recurses on its own stack: MIR lowering walks nested
// expressions/statements, and comptime evaluation nests one lowering per
// comptime call. POSIX main threads default to 8 MiB, but a Windows process
// gets 1 MiB, and the comptime recursion guard (`StackSpace.h`) keeps the last
// 512 KiB free for reporting its diagnostic. On a 1 MiB stack that reserve is
// half the stack, so an ordinary deep compile of a large module looks exactly
// like an infinitely recursive comptime function and aborts the job.
//
// `std::thread` cannot request a stack size, so this small wrapper over the
// platform API provides the surface the compiler's thread pools need
// (`joinable`/`join`/`detach`, and worker identity for `wait_idle`).
// Header-only on purpose: the compiler targets list their sources explicitly,
// and `ctpl.h` (a third-party header) includes this one to give its pool
// threads a large stack.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cerrno>
#include <functional>
#include <memory>
#include <system_error>
#include <utility>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>

#else

#include <pthread.h>

#endif

namespace chem {

/**
 * Stack reserved for a compiler work thread. 16 MiB leaves the 512 KiB comptime
 * reserve a small fraction of the stack (as on POSIX) while staying far below
 * the address space a 64-bit process has; only touched pages are committed.
 */
inline constexpr size_t LARGE_STACK_BYTES = 16u * 1024u * 1024u;

/**
 * A joinable thread with an explicit stack size, used for work that recurses
 * deeply inside the compiler (module jobs, symres/type-verify workers, MIR
 * lowering workers).
 */
class thread {

    /** Everything the OS entry point needs: the body and the thread identity. */
    struct Entry {
        thread* self = nullptr;
        std::function<void()> body;
    };public:

    thread() noexcept = default;

    explicit thread(std::function<void()> fn, size_t stack_bytes = LARGE_STACK_BYTES) {
        auto* entry = new Entry{this, std::move(fn)};
#if defined(_WIN32)
        unsigned int id = 0;
        const uintptr_t handle = _beginthreadex(
                nullptr,
                static_cast<unsigned>(stack_bytes),
                &thread::windows_entry,
                entry,
                0,
                &id
        );
        if (handle == 0) {
            // `_beginthreadex` reports failure through `errno` and returns 0
            const int err = errno;
            delete entry;
            throw std::system_error(err, std::generic_category(),
                                    "could not create a compiler worker thread");
        }
        handle_ = reinterpret_cast<void*>(handle);
#else
        auto* native = new pthread_t();
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setstacksize(&attr, stack_bytes);
        const int rc = pthread_create(native, &attr, &thread::posix_entry, entry);
        pthread_attr_destroy(&attr);
        if (rc != 0) {
            delete native;
            delete entry;
            throw std::system_error(rc, std::generic_category(),
                                    "could not create a compiler worker thread");
        }
        handle_ = native;
#endif
    }

    ~thread() {
        // Same contract as `std::thread`: a joinable thread must be joined or
        // detached. The pools always do one of the two; detaching here keeps a
        // build tool from terminating on an unexpected path.
        if (joinable()) {
            detach();
        }
    }

    thread(thread&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    thread& operator=(thread&& other) noexcept {
        if (this != &other) {
            if (joinable()) detach();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    thread(const thread&) = delete;
    thread& operator=(const thread&) = delete;

    bool joinable() const noexcept { return handle_ != nullptr; }

    void join() {
        if (!joinable()) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                                    "join on a non-joinable compiler worker thread");
        }
#if defined(_WIN32)
        WaitForSingleObject(static_cast<HANDLE>(handle_), INFINITE);
        CloseHandle(static_cast<HANDLE>(handle_));
#else
        pthread_join(*static_cast<pthread_t*>(handle_), nullptr);
        delete static_cast<pthread_t*>(handle_);
#endif
        handle_ = nullptr;
    }

    void detach() {
        if (!joinable()) {
            throw std::system_error(std::make_error_code(std::errc::invalid_argument),
                                    "detach on a non-joinable compiler worker thread");
        }
#if defined(_WIN32)
        CloseHandle(static_cast<HANDLE>(handle_));
#else
        pthread_detach(*static_cast<pthread_t*>(handle_));
        delete static_cast<pthread_t*>(handle_);
#endif
        handle_ = nullptr;
    }

    /**
     * The `chem::thread` whose entry function is running on the calling thread,
     * or null. Lets the thread pool recognise its own workers (its `wait_idle`
     * must not count the calling worker's own frame). Pointer identity, so no
     * OS thread id and no `std::thread::id` is needed.
     */
    static thread* current() noexcept {
        return current_thread_;
    }

private:

    /** Per-thread identity; the entry point sets and clears it. */
    inline static thread_local thread* current_thread_ = nullptr;

    static void run_entry(Entry* entry) {
        current_thread_ = entry->self;
        if (entry->body) {
            entry->body();
        }
        current_thread_ = nullptr;
        delete entry;
    }

#if defined(_WIN32)

    static unsigned __stdcall windows_entry(void* arg) {
        run_entry(static_cast<Entry*>(arg));
        return 0;
    }

#else

    static void* posix_entry(void* arg) {
        run_entry(static_cast<Entry*>(arg));
        return nullptr;
    }

#endif

    /** `HANDLE` on Windows, `pthread_t*` (heap owned) elsewhere. */
    void* handle_ = nullptr;

};

/**
 * Run `fn` on a thread with a large stack and wait for it. Used for the
 * compiler's top-level entry so the main thread gets the same stack the worker
 * threads have.
 */
inline void run_on_large_stack(const std::function<void()>& fn,
                               size_t stack_bytes = LARGE_STACK_BYTES) {
    thread worker(fn, stack_bytes);
    worker.join();
}

} // namespace chem
