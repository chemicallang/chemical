// Copyright (c) Chemical Language Foundation 2025.
//
// A stable, contiguous byte pool for MIR's interned text (type/field/symbol
// names, constant and const-expression payloads).
//
// Why not `std::vector<char>`: MIR hands out `const char*` + length into these
// pools (`name_data`, `data_at`) and other lowering workers append to the same
// pool, so a reallocating vector would dangle a pointer a reader still holds
// (action E3).
//
// Why not `std::deque<char>`: deque storage is chunked, so `&pool[i]` is not
// followed by `pool[i + 1]` — a `const char*` read of `length` bytes walks off
// the chunk. The pool must be *contiguous per entry*.
//
// Each entry is its own `std::string`, held in a `MIRStableArray` whose block
// elements never move, so an entry's bytes are both stable and contiguous for
// its whole lifetime (even for a short string, whose characters live inside the
// `std::string` object that never moves). An "offset" is an entry index;
// `data_at`/`text` recover the bytes with a lock-free read (action E3d).

#pragma once

#include "MIRStableArray.h"

#include <cstdint>
#include <shared_mutex>
#include <string>

namespace mir {

class MIRBytePool {
public:
    MIRBytePool() = default;
    MIRBytePool(const MIRBytePool&) = delete;
    MIRBytePool& operator=(const MIRBytePool&) = delete;

    /** Append `n` bytes and return the entry index (the stable "offset"). */
    uint32_t append(const char* s, uint32_t n) {
        std::lock_guard<std::shared_mutex> lock(mtx_);
        return entries_.push_back(std::string(s, n));
    }

    /**
     * Pointer to an entry's contiguous bytes. Lock-free: the entry object never
     * moves and its bytes are immutable, so the pointer stays valid for the
     * pool's lifetime (until `clear`).
     */
    const char* data_at(uint32_t index) const { return entries_[index].data(); }

    const std::string& text(uint32_t index) const { return entries_[index]; }

    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    /** Drop every entry (writer-only barrier; no concurrent readers). */
    void clear() {
        std::lock_guard<std::shared_mutex> lock(mtx_);
        entries_.clear();
    }

private:
    mutable std::shared_mutex mtx_;
    MIRStableArray<std::string> entries_;
};

} // namespace mir
