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
#include "MIRKeyMap.h"
#include "MIRTableLock.h"

#include <cstdint>
#include <cstring>
#include <shared_mutex>
#include <string>

namespace mir {

class MIRBytePool {
public:
    MIRBytePool() = default;
    MIRBytePool(const MIRBytePool&) = delete;
    MIRBytePool& operator=(const MIRBytePool&) = delete;

    /**
     * Declare that one thread performs every append (see MIRTableLock.h). The
     * shipping pipeline is single-writer, and this pool is on the lowering hot
     * path — one `append` per aggregate field name, type name and symbol name —
     * so taking a `shared_mutex` there was pure overhead on the one path that
     * runs most often. Readers were already lock-free (the entries never move).
     */
    void set_single_threaded(bool v) noexcept { lock_policy_.set_single_threaded(v); }
    bool single_threaded() const noexcept { return lock_policy_.single_threaded(); }

    /**
     * Append `n` bytes and return the entry index (the stable "offset").
     *
     * The index is an opaque handle that callers only ever read back through
     * `data_at`/`text`, so **identical text shares one entry**: names repeat
     * constantly in a module (field names, `__chx_*` spellings, repeated symbol
     * names) and this is the single most frequent allocation site in lowering
     * (~193k appends on the plugin suite). Deduplicating by content removes both
     * the entry and, on a hit, the `std::string` allocation.
     *
     * The index is a *hint* verified by a byte compare, so a 64-bit hash
     * collision can never alias two different texts. A colliding append keeps
     * the existing entry as the hash's representative instead of overwriting it,
     * so an unindexed duplicate can never ping-pong the entry out of the map.
     */
    uint32_t append(const char* s, uint32_t n) {
        MIRWriteLock lock(mtx_, single_threaded());
        const uint64_t h = hash_bytes(s, n);
        const uint32_t hit = index_.find(h);
        if (hit != MIRKeyMap::kNotFound && hit < entries_.size() &&
            entries_[hit].size() == n &&
            (n == 0 || std::memcmp(entries_[hit].data(), s, n) == 0)) {
            return hit;
        }
        const uint32_t id = entries_.emplace_back(s, n);
        if (hit == MIRKeyMap::kNotFound) index_.set(h, id);
        return id;
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
        MIRWriteLock lock(mtx_, single_threaded());
        entries_.clear();
        index_.clear();
    }

private:
    static uint64_t hash_bytes(const char* s, uint32_t n) {
        uint64_t h = 1469598103934665603ull;
        for (uint32_t i = 0; i < n; ++i) {
            h ^= static_cast<unsigned char>(s[i]);
            h *= 1099511628211ull;
        }
        return h;
    }

    MIRTableLockPolicy lock_policy_;
    mutable std::shared_mutex mtx_;
    MIRStableArray<std::string> entries_;
    /** content hash -> a representative entry index (a verified hint) */
    MIRKeyMap index_;
};

} // namespace mir
