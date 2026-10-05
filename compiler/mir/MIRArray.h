// Copyright (c) Chemical Language Foundation 2025.
//
// MIRArray<T>: a non-owning, arena-backed append-only array used for
// instruction streams, operand streams, block tables and move-path tables.
// See mir-implementation-plan.md §2.2 and §2.7.

#pragma once

#include "MIRTypes.h"
#include "MIRArena.h"

#include <cstring>
#include <type_traits>

namespace mir {

template<typename T>
struct MIRArray {
    static_assert(std::is_trivially_copyable_v<T>,
                  "MIRArray elements must be trivially copyable (no destructors)");

    T* ptr = nullptr;
    uint32_t len = 0;
    uint32_t cap = 0;

    T* begin() { return ptr; }
    T* end() { return ptr + len; }
    const T* begin() const { return ptr; }
    const T* end() const { return ptr + len; }

    uint32_t size() const { return len; }
    bool empty() const { return len == 0; }

    T& operator[](uint32_t i) { return ptr[i]; }
    const T& operator[](uint32_t i) const { return ptr[i]; }

    void clear() {
        ptr = nullptr;
        len = 0;
        cap = 0;
    }

    /** Ensure capacity for at least `n` elements, growing from the arena. */
    bool reserve(MIRArena& arena, uint32_t n) {
        if (n <= cap) return true;
        T* np = arena.allocate_array<T>(n);
        if (!np) return false;
        if (ptr && len) std::memcpy(np, ptr, sizeof(T) * len);
        ptr = np;
        cap = n;
        return true;
    }

    /** Append a default-initialized element. Returns nullptr on OOM. */
    T* push(MIRArena& arena) {
        if (len == cap) {
            uint32_t grown = cap == 0 ? 8u : cap * 2u;
            if (!reserve(arena, grown)) return nullptr;
        }
        T* slot = &ptr[len++];
        *slot = T{};
        return slot;
    }

    bool push(MIRArena& arena, const T& value) {
        T* slot = push(arena);
        if (!slot) return false;
        *slot = value;
        return true;
    }
};

} // namespace mir
