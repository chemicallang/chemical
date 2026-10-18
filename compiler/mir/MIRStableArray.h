// Copyright (c) Chemical Language Foundation 2025.
//
// An append-only array whose reads are lock-free and whose elements never move
// (action E3d).
//
// The MIR tables are read constantly (a type/constant/symbol per emitted
// operand, a type lookup per lowered expression, a field pointer per aggregate)
// and, with parallel lowering, written by several workers at once. A
// `std::vector` cannot be read while another thread appends: growth reallocates
// the storage out from under a reader. Guarding every read with a
// `std::shared_mutex` serialized the workers (the hot read lock's refcount is
// one cache line) and made parallel lowering a net loss.
//
// Storage is block-chunked: `new T[kBlockSize]` blocks are allocated as the
// array grows and are never moved or freed until the array is destroyed, so an
// element address (`&arr[i]`, a `MIRFieldInfo*`, a `const char*` into a pooled
// string) stays valid across any later append — the same guarantee the previous
// `std::deque` gave. The pointers to those blocks live in a small *map* that is
// published through an atomic pointer; growing the map copies the block
// pointers into a larger map and *retains* the old one, so a reader that loaded
// an older map still finds every block it could hold an id for.
//
// A reader therefore does two atomic loads (map, then block) and no lock. A
// writer-input scan (the intern fallback) walks block by block, so it pays the
// indirection once per block instead of once per element.

#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

namespace mir {

template <typename T>
class MIRStableArray {
public:
    static constexpr uint32_t kBlockShift = 10;
    static constexpr uint32_t kBlockSize = 1u << kBlockShift;
    static constexpr uint32_t kBlockMask = kBlockSize - 1;
    static constexpr uint32_t kNotFound = 0xFFFFFFFFu;

    MIRStableArray() = default;
    MIRStableArray(const MIRStableArray&) = delete;
    MIRStableArray& operator=(const MIRStableArray&) = delete;
    ~MIRStableArray() {
        for (uint32_t b = 0; b < block_count_; ++b) delete[] map_[b].load(std::memory_order_relaxed);
        for (Map* m : retired_maps_) delete[] m;
        delete[] map_;
    }

    /** Append one element; returns its index. Writer-only (owning lock held). */
    uint32_t push_back(const T& v) {
        const uint32_t i = size_;
        T* blk = ensure_block(i >> kBlockShift);
        blk[i & kBlockMask] = v;
        size_ = i + 1;
        size_atomic_.store(size_, std::memory_order_release);
        return i;
    }

    /** Append `n` elements copied from `src`; returns the first index. */
    uint32_t append(const T* src, uint32_t n) {
        if (n == 0) return size_;
        const uint32_t start = size_;
        for (uint32_t k = 0; k < n; ++k) push_back(src[k]);
        return start;
    }

    /**
     * Reset the logical length. Writer-only; must not run while a reader relies
     * on a previously published element (a module-reuse barrier). Blocks are
     * kept for reuse.
     */
    void clear() noexcept {
        size_ = 0;
        size_atomic_.store(0, std::memory_order_release);
    }

    /** Lock-free read; `i` must be < `size()`. The address never changes. */
    const T& operator[](uint32_t i) const noexcept {
        const Map* m = published_map_.load(std::memory_order_acquire);
        return m[i >> kBlockShift].load(std::memory_order_acquire)[i & kBlockMask];
    }
    T& operator[](uint32_t i) noexcept {
        Map* m = published_map_.load(std::memory_order_acquire);
        return m[i >> kBlockShift].load(std::memory_order_acquire)[i & kBlockMask];
    }

    uint32_t size() const noexcept { return size_atomic_.load(std::memory_order_acquire); }
    bool empty() const noexcept { return size() == 0; }

    /**
     * Writer-only linear scan: the index of the first element `pred` accepts, or
     * `kNotFound`. Walks one block at a time so the map indirection is paid per
     * block, not per element.
     */
    template <class Pred>
    uint32_t find_writer(Pred&& pred) const {
        const uint32_t n = size_;
        uint32_t i = 0;
        while (i < n) {
            const T* blk = writer_block(i >> kBlockShift);
            const uint32_t cnt = (n - i) < kBlockSize ? (n - i) : kBlockSize;
            for (uint32_t k = 0; k < cnt; ++k, ++i) {
                if (pred(blk[k])) return i;
            }
        }
        return kNotFound;
    }

private:
    using Map = std::atomic<T*>;

    /** Writer-only: block `b` (must already be allocated). */
    const T* writer_block(uint32_t b) const noexcept {
        return map_[b].load(std::memory_order_relaxed);
    }

    T* ensure_block(uint32_t b) {
        if (b >= map_cap_) grow_map(b + 1);
        T* blk = map_[b].load(std::memory_order_relaxed);
        if (!blk) {
            blk = new T[kBlockSize];
            map_[b].store(blk, std::memory_order_release);
            if (b + 1 > block_count_) block_count_ = b + 1;
        }
        return blk;
    }

    void grow_map(uint32_t needed) {
        uint32_t ncap = map_cap_ ? map_cap_ * 2 : 16;
        while (ncap < needed) ncap *= 2;
        Map* nm = new Map[ncap];
        for (uint32_t i = 0; i < ncap; ++i) nm[i].store(nullptr, std::memory_order_relaxed);
        for (uint32_t i = 0; i < map_cap_; ++i) {
            nm[i].store(map_[i].load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        if (map_) retired_maps_.push_back(map_);
        map_ = nm;
        map_cap_ = ncap;
        published_map_.store(map_, std::memory_order_release);
    }

    Map* map_ = nullptr;          // writer's current block map
    uint32_t map_cap_ = 0;        // blocks `map_` can address
    uint32_t block_count_ = 0;    // blocks allocated (for destruction)
    uint32_t size_ = 0;           // writer's logical length
    std::atomic<uint32_t> size_atomic_{0};
    std::atomic<Map*> published_map_{nullptr};
    // Old maps are retained so a reader holding one (and an id that maps to a
    // block it contains) stays valid across a map growth.
    std::vector<Map*> retired_maps_;
};

} // namespace mir
