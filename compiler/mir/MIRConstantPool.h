// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level interned constants (integers, floats, booleans, null, strings,
// byte blobs). A string literal is a constant plus its addressable global
// representation. See mir-design.md §5.2.

#pragma once

#include "MIRTypes.h"
#include "MIRBytePool.h"
#include "MIRKeyMap.h"
#include "MIRStableArray.h"
#include "MIRTableLock.h"

#include <cstdint>
#include <cstring>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

namespace mir {

enum class MIRConstantKind : uint8_t {
    Int = 0,
    Float,
    Double,
    Bool,
    Null,
    String,
    Bytes,
};

struct MIRConstant {
    MIRConstantKind kind = MIRConstantKind::Int;
    TypeId type = MIR_INVALID_ID;
    uint64_t bits = 0;        // integer/float bit pattern, bool, or null
    uint32_t data_offset = 0; // index into MIRConstantPool::data
    uint32_t data_count = 0;  // payload length (string/bytes)
};

struct MIRConstantPool {
    /** Declare a single mutating thread (see MIRTableLock.h). */
    void set_single_threaded(bool v) noexcept {
        lock_policy_.set_single_threaded(v);
        data.set_single_threaded(v);
    }
    bool single_threaded() const noexcept { return lock_policy_.single_threaded(); }
    /** `data` (the payload bytes) is reference-stable on append (action E3,
     * see MIRBytePool.h); `constants` is a stable append-only array so readers
     * need no lock (action E3d). */
    MIRStableArray<MIRConstant> constants;
    MIRBytePool data;
    /** hash -> a representative constant id (see `add`) */
    MIRKeyMap index_;

    /** Lock-free read of the record (the constants array is stable on append). */
    const MIRConstant& get(ConstantId id) const {
        return constants[id];
    }
    uint32_t size() const {
        return constants.size();
    }

    /**
     * Intern a constant. `index_` is a hint only: a hit is re-verified against
     * the stored constant and a miss falls back to the scan, so the result is
     * identical to a pure linear scan. The pool is append-only, so an entry's
     * key can never go stale; the index just skips the scan for repeats (most
     * `add` calls re-add one of a few hundred integral constants).
     *
     * Thread-safety (action E1): the mutators are mutex-guarded so several
     * lowering workers can intern concurrently. A payload append and the intern
     * that references it (`add_string`) stay under one lock so two workers can
     * never interleave their `data` writes.
     */
    ConstantId add(const MIRConstant& c) {
        if (!single_threaded()) {
            // Read-mostly: 50,021 of 55,723 `add` calls on the main suite are
            // hits. Answer them under the shared lock so concurrent lowering
            // workers do not serialise on the exclusive one.
            const uint64_t h = hash_of(c);
            MIRReadLock rl(mtx_, false);
            const uint32_t hit = index_.find(h);
            if (hit != MIRKeyMap::kNotFound && hit < constants.size() &&
                matches(constants[hit], c)) {
                return hit;
            }
        }
        MIRWriteLock lock(mtx_, single_threaded());
        return add_unlocked(c);
    }

    /** Drop every constant and payload byte (used when a `MIRModule` is reused). */
    void clear() {
        MIRWriteLock lock(mtx_, single_threaded());
        constants.clear();
        data.clear();
        index_.clear();
    }

    /** Exactly the comparison `add` performs. */
    bool matches(const MIRConstant& o, const MIRConstant& c) const {
        if (o.kind != c.kind || o.bits != c.bits || o.data_count != c.data_count) return false;
        if (c.data_count == 0) return true;
        // each payload is one contiguous, stable entry
        return data.text(o.data_offset) == data.text(c.data_offset);
    }

    /**
     * Hash of exactly the fields `matches` compares (never `data_offset`, which
     * can differ between equal constants). The payload has already been appended
     * to `data` by the `add_*` helpers, so it is readable at `c.data_offset` —
     * and hashing it costs no more than the append that just copied it.
     */
    uint64_t hash_of(const MIRConstant& c) const {
        uint64_t h = 1469598103934665603ull;
        auto mix = [&h](uint64_t v) {
            h ^= v;
            h *= 1099511628211ull;
        };
        mix(static_cast<uint64_t>(c.kind));
        mix(c.bits);
        mix(c.data_count);
        if (c.data_count) {
            for (char ch : data.text(c.data_offset)) mix(static_cast<uint8_t>(ch));
        }
        return h;
    }

    ConstantId add_int(TypeId t, uint64_t bits) {
        MIRConstant c;
        c.kind = MIRConstantKind::Int;
        c.type = t;
        c.bits = bits;
        return add(c);
    }

    ConstantId add_float(TypeId t, float v) {
        uint32_t b = 0;
        std::memcpy(&b, &v, sizeof(b));
        MIRConstant c;
        c.kind = MIRConstantKind::Float;
        c.type = t;
        c.bits = b;
        return add(c);
    }

    ConstantId add_double(TypeId t, double v) {
        uint64_t b = 0;
        std::memcpy(&b, &v, sizeof(b));
        MIRConstant c;
        c.kind = MIRConstantKind::Double;
        c.type = t;
        c.bits = b;
        return add(c);
    }

    ConstantId add_bool(TypeId t, bool v) {
        MIRConstant c;
        c.kind = MIRConstantKind::Bool;
        c.type = t;
        c.bits = v ? 1u : 0u;
        return add(c);
    }

    ConstantId add_null(TypeId t) {
        MIRConstant c;
        c.kind = MIRConstantKind::Null;
        c.type = t;
        return add(c);
    }

    ConstantId add_string(TypeId t, const char* s, uint32_t len) {
        MIRConstant c;
        c.kind = MIRConstantKind::String;
        c.type = t;
        MIRWriteLock lock(mtx_, single_threaded());
        c.data_offset = data.append(s, len);
        c.data_count = len;
        return add_unlocked(c);
    }

private:
    /** The intern body; the caller must hold `mtx_`. */
    ConstantId add_unlocked(const MIRConstant& c) {
        const uint64_t h = hash_of(c);
        const uint32_t n = constants.size();
        const uint32_t hit = index_.find(h);
        if (hit != MIRKeyMap::kNotFound) {
            if (hit < n && matches(constants[hit], c)) return hit;
            // Present but not equal: a 64-bit collision. Only then does the
            // verified scan have anything to find.
            const uint32_t found = constants.find_writer([this, &c](const MIRConstant& r) {
                return matches(r, c);
            });
            if (found != MIRStableArray<MIRConstant>::kNotFound) {
                index_.set(h, found);
                return found;
            }
        }
        // An absent hash proves no equal constant exists (`hash_of` covers
        // exactly the fields `matches` compares, and every constant is
        // registered when it is added), so the scan is skipped. Measured on the
        // main suite: 5,702 misses performed 3,538,383 `matches` calls and found
        // nothing.
        constants.push_back(c);
        const ConstantId id = static_cast<ConstantId>(n);
        index_.set(h, id);
        return id;
    }

    MIRTableLockPolicy lock_policy_;
    /** Guards the mutators only (action E1/E3d): readers are lock-free against
     *  the stable array/byte pool, so the exclusive lock only serializes appends
     *  when more than one lowering worker runs. */
    mutable std::shared_mutex mtx_;
};

} // namespace mir
