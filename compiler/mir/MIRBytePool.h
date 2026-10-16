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
// Each entry is its own `std::string` held in a `std::deque` (whose existing
// elements keep their address on `push_back`). An entry's bytes are therefore
// both stable and contiguous for its whole lifetime; an "offset" is an entry
// index, and `data_at`/`text` recover the bytes.

#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <string>

namespace mir {

class MIRBytePool {
public:
    /** Append `n` bytes and return the entry index (the stable "offset"). */
    uint32_t append(const char* s, uint32_t n) {
        std::lock_guard<std::shared_mutex> lock(mtx_);
        entries_.emplace_back(s, n);
        return static_cast<uint32_t>(entries_.size() - 1);
    }

    /**
     * Pointer to an entry's contiguous bytes. The lock is only needed to look
     * the entry up (`std::deque`'s chunk map can reallocate on a concurrent
     * append even though the entry itself never moves); the returned bytes are
     * immutable, so the caller may read them after the lock is released.
     */
    const char* data_at(uint32_t index) const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return entries_[index].data();
    }

    const std::string& text(uint32_t index) const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return entries_[index];
    }

    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return entries_.size();
    }
    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mtx_);
        return entries_.empty();
    }
    void clear() {
        std::lock_guard<std::shared_mutex> lock(mtx_);
        entries_.clear();
    }

private:
    mutable std::shared_mutex mtx_;
    std::deque<std::string> entries_;
};

} // namespace mir
