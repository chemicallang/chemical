// Copyright (c) Chemical Language Foundation 2025.
//
// Per-worker bump arena for MIR function-local data. Deliberately NOT
// ASTAllocator: no mutex, no destructor tracking, trivial records only.
// See mir-implementation-plan.md §2.1 and §14.6.

#pragma once

#include "MIRTypes.h"

#include <cstddef>
#include <new>
#include <utility>

namespace mir {

/**
 * A small thread-owned bump allocator.
 *
 * The first chunk is allocated inline (one heap allocation for the whole
 * arena), so small functions never grow the chunk list. `reset()` rewinds all
 * chunks so the same arena can be reused for the next function.
 */
class MIRArena {
public:
    static constexpr size_t DEFAULT_INLINE_BYTES = 32 * 1024;

    explicit MIRArena(size_t inline_bytes = DEFAULT_INLINE_BYTES);
    ~MIRArena();

    MIRArena(const MIRArena&) = delete;
    MIRArena& operator=(const MIRArena&) = delete;

    /**
     * Allocate `bytes` with the given alignment. Returns nullptr only on
     * out-of-memory; callers (the builder/emitter) must treat that as a
     * compilation failure, never dereference it.
     */
    void* allocate(size_t bytes, size_t alignment = MIR_DEFAULT_ALIGN);

    template<typename T>
    T* allocate_array(size_t count) {
        if (count == 0) return nullptr;
        return static_cast<T*>(allocate(sizeof(T) * count, alignof(T)));
    }

    template<typename T, typename... Args>
    T* create(Args&&... args) {
        void* mem = allocate(sizeof(T), alignof(T));
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    /** Rewind every chunk to empty and make the first chunk current again. */
    void reset();

    size_t bytes_used() const { return used_total_; }
    size_t capacity() const { return cap_total_; }

private:
    struct alignas(std::max_align_t) Chunk {
        Chunk* next;
        size_t capacity;
        size_t used;
    };

    static char* data_of(Chunk* c) {
        return reinterpret_cast<char*>(c) + sizeof(Chunk);
    }

    Chunk* make_chunk(size_t min_bytes);
    void free_all();

    Chunk* first_ = nullptr;
    Chunk* current_ = nullptr;
    size_t used_total_ = 0;
    size_t cap_total_ = 0;
};

} // namespace mir
