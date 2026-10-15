// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level interned constants (integers, floats, booleans, null, strings,
// byte blobs). A string literal is a constant plus its addressable global
// representation. See mir-design.md §5.2.

#pragma once

#include "MIRTypes.h"

#include <cstdint>
#include <cstring>
#include <mutex>
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
    std::vector<MIRConstant> constants;
    std::vector<char> data;
    /** hash -> a representative constant id (see `add`) */
    std::unordered_map<uint64_t, ConstantId> index_;

    const MIRConstant& get(ConstantId id) const { return constants[id]; }
    uint32_t size() const { return static_cast<uint32_t>(constants.size()); }

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
        std::lock_guard<std::mutex> lock(mtx_);
        return add_unlocked(c);
    }

    /** Drop every constant and payload byte (used when a `MIRModule` is reused). */
    void clear() {
        std::lock_guard<std::mutex> lock(mtx_);
        constants.clear();
        data.clear();
        index_.clear();
    }

    /** Exactly the comparison `add` performs. */
    bool matches(const MIRConstant& o, const MIRConstant& c) const {
        if (o.kind != c.kind || o.bits != c.bits || o.data_count != c.data_count) return false;
        for (uint32_t j = 0; j < c.data_count; ++j) {
            if (data[o.data_offset + j] != data[c.data_offset + j]) return false;
        }
        return true;
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
        for (uint32_t j = 0; j < c.data_count; ++j) {
            const uint32_t idx = c.data_offset + j;
            mix(idx < data.size() ? static_cast<uint8_t>(data[idx]) : 0u);
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
        std::lock_guard<std::mutex> lock(mtx_);
        c.data_offset = static_cast<uint32_t>(data.size());
        c.data_count = len;
        if (len) data.insert(data.end(), s, s + len);
        return add_unlocked(c);
    }

private:
    /** The intern body; the caller must hold `mtx_`. */
    ConstantId add_unlocked(const MIRConstant& c) {
        const uint64_t h = hash_of(c);
        auto hit = index_.find(h);
        if (hit != index_.end() && hit->second < constants.size() &&
            matches(constants[hit->second], c)) {
            return hit->second;
        }
        for (uint32_t i = 0; i < constants.size(); ++i) {
            if (matches(constants[i], c)) {
                index_[h] = i;
                return i;
            }
        }
        constants.push_back(c);
        const ConstantId id = static_cast<ConstantId>(constants.size() - 1);
        index_[h] = id;
        return id;
    }

    /** Guards the mutators (action E1: shared interning under parallel lowering). */
    std::mutex mtx_;
};

} // namespace mir
