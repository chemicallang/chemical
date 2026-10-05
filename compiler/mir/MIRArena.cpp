// Copyright (c) Chemical Language Foundation 2025.

#include "MIRArena.h"

namespace mir {

MIRArena::MIRArena(size_t inline_bytes) {
    if (inline_bytes < 256) inline_bytes = 256;
    Chunk* c = make_chunk(inline_bytes);
    first_ = c;
    current_ = c;
}

MIRArena::~MIRArena() {
    free_all();
}

MIRArena::Chunk* MIRArena::make_chunk(size_t min_bytes) {
    size_t cap = min_bytes < 1024 ? 1024 : min_bytes;
    // One allocation for the header + payload; Chunk is alignas(max_align_t),
    // so data_of() is suitably aligned for any MIR record.
    void* mem = ::operator new(sizeof(Chunk) + cap, std::nothrow);
    if (!mem) return nullptr;
    Chunk* c = new (mem) Chunk{nullptr, cap, 0};
    cap_total_ += cap;
    return c;
}

void* MIRArena::allocate(size_t bytes, size_t alignment) {
    if (alignment == 0) alignment = 1;
    if (bytes == 0) bytes = 1;

    Chunk* c = current_;
    if (!c) return nullptr;

    uintptr_t base = reinterpret_cast<uintptr_t>(data_of(c)) + c->used;
    uintptr_t aligned = (base + (alignment - 1)) & ~(static_cast<uintptr_t>(alignment - 1));
    size_t padding = static_cast<size_t>(aligned - base);

    if (c->used + padding + bytes > c->capacity) {
        // Grow geometrically, but always enough for this allocation.
        size_t need = c->capacity * 2;
        size_t required = padding + bytes + 64;
        if (need < required) need = required;

        Chunk* nc = make_chunk(need);
        if (!nc) return nullptr;
        c->next = nc;
        current_ = nc;
        c = nc;

        base = reinterpret_cast<uintptr_t>(data_of(c));
        aligned = (base + (alignment - 1)) & ~(static_cast<uintptr_t>(alignment - 1));
        padding = static_cast<size_t>(aligned - base);
    }

    c->used += padding + bytes;
    used_total_ += padding + bytes;
    return reinterpret_cast<void*>(aligned);
}

void MIRArena::reset() {
    for (Chunk* c = first_; c; c = c->next) {
        c->used = 0;
    }
    current_ = first_;
    used_total_ = 0;
}

void MIRArena::free_all() {
    Chunk* c = first_;
    while (c) {
        Chunk* next = c->next;
        c->~Chunk();
        ::operator delete(static_cast<void*>(c));
        c = next;
    }
    first_ = nullptr;
    current_ = nullptr;
    cap_total_ = 0;
}

} // namespace mir
