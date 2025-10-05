// Copyright (c) Chemical Language Foundation 2025.

#ifdef _WIN32
#include "win/atomic_compat.h"
#else
#include "nix/atomic.h"
#endif

void atomic_thread_fence_explicit(int mo) {
    atomic_thread_fence(mo);
}