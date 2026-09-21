// Copyright (c) Chemical Language Foundation 2025.

#include "StackSpace.h"

#if defined(_WIN32)

// `GetCurrentThreadStackLimits` (used below) is only declared when the Windows
// target version is at least Windows 8 (0x0602). Some MinGW toolchains — notably
// the msvcrt-based llvm-mingw used for the Windows x64 build — default
// `_WIN32_WINNT` lower, which hides the declaration and fails the build. Raise it
// for this translation unit before the first Windows header is pulled in
// (StackSpace.h only includes <cstddef>).
#if defined(_WIN32_WINNT)
#if (_WIN32_WINNT < 0x0602)
#undef _WIN32_WINNT
#endif
#endif
#if !defined(_WIN32_WINNT)
#define _WIN32_WINNT 0x0602
#endif

#include <windows.h>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#else

#include <pthread.h>

#if defined(__linux__)
#include <unistd.h>
#endif

#endif

namespace chem {

    namespace {

        struct StackBounds {
            // highest address the stack may use (the stack grows downwards)
            size_t base;
            // total size of the stack
            size_t size;
            // whether the bounds have already been queried on this thread
            bool resolved;
        };

        // cached per thread, because querying the bounds takes a system call
        thread_local StackBounds stack_bounds{0, 0, false};

        /**
         * address of the calling frame, used to know how much of the stack is
         * already in use
         */
        inline size_t current_frame_address() {
#if defined(_MSC_VER)
            return (size_t) _AddressOfReturnAddress();
#else
            return (size_t) (void*) __builtin_frame_address(0);
#endif
        }

        void resolve_stack_bounds(StackBounds& bounds) {
            bounds.resolved = true;
#if defined(_WIN32)
            ULONG_PTR low = 0;
            ULONG_PTR high = 0;
            GetCurrentThreadStackLimits(&low, &high);
            bounds.base = (size_t) high;
            bounds.size = high > low ? (size_t) (high - low) : 0;
#elif defined(__APPLE__)
            const auto base = pthread_get_stackaddr_np(pthread_self());
            const auto size = pthread_get_stacksize_np(pthread_self());
            bounds.base = (size_t) base;
            bounds.size = size;
#else
            pthread_attr_t attr;
            if(pthread_getattr_np(pthread_self(), &attr) != 0) {
                return;
            }
            void* addr = nullptr;
            size_t size = 0;
            if(pthread_attr_getstack(&attr, &addr, &size) == 0) {
                bounds.base = (size_t) addr + size;
                bounds.size = size;
            }
            pthread_attr_destroy(&attr);
#endif
        }

    }

    size_t remaining_stack_bytes() {
        if(!stack_bounds.resolved) {
            resolve_stack_bounds(stack_bounds);
        }
        if(stack_bounds.size == 0) {
            return 0;
        }
        const auto frame = current_frame_address();
        if(frame >= stack_bounds.base) {
            return 0;
        }
        const auto used = stack_bounds.base - frame;
        if(used >= stack_bounds.size) {
            return 0;
        }
        return stack_bounds.size - used;
    }

}
