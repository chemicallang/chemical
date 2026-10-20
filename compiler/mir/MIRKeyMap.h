// Copyright (c) Chemical Language Foundation 2025.
//
// A small open-addressing map from a 64-bit key to a 32-bit id.
//
// The module tables' interning caches are looked up once per interned type,
// constant and AST type mapping — hundreds of thousands of times per suite
// compile — and they are all the same shape: a 64-bit key (a hash, a pointer, or
// a packed `(declaration, kind)`) to a small dense id. `std::unordered_map` is a
// poor fit there: one heap allocation per node, a pointer chase per lookup, and
// a modulo by a prime bucket count (a hardware division) for every probe.
// `std::map` is worse still (a red-black tree, compared key by key).
//
// This map keeps the entries in one flat power-of-two array, hashes with a
// multiply+xor (no division), probes linearly and grows by doubling. It is not
// an ordered container and offers no iteration: every use site only needs
// `find`/`set`/`clear`, and the ids it stores are assigned by the caller, so the
// map's own layout never affects the result.
//
// Thread-safety: no locking. Every user is already serialized by the owning
// table's write lock, or is per-thread (see `MIRTypeBuilder`'s thread memo).

#pragma once

#include <cstdint>
#include <vector>

namespace mir {

class MIRKeyMap {
public:
    /** Returned by `find` when the key is absent. Also the empty-slot marker. */
    static constexpr uint32_t kNotFound = 0xFFFFFFFFu;

    /** The id stored for `key`, or `kNotFound`. */
    uint32_t find(uint64_t key) const {
        if (size_ == 0) return kNotFound;
        const uint32_t mask = static_cast<uint32_t>(slots_.size()) - 1u;
        for (uint32_t i = static_cast<uint32_t>(mix(key)) & mask;; i = (i + 1u) & mask) {
            const Slot& s = slots_[i];
            if (s.value == kNotFound) return kNotFound;
            if (s.key == key) return s.value;
        }
    }

    /** Insert or overwrite `key` -> `value`. */
    void set(uint64_t key, uint32_t value) {
        if ((size_ + 1u) * 4u >= slots_.size() * 3u) grow();
        const uint32_t mask = static_cast<uint32_t>(slots_.size()) - 1u;
        for (uint32_t i = static_cast<uint32_t>(mix(key)) & mask;; i = (i + 1u) & mask) {
            Slot& s = slots_[i];
            if (s.value == kNotFound) {
                s.key = key;
                s.value = value;
                ++size_;
                return;
            }
            if (s.key == key) {
                s.value = value;
                return;
            }
        }
    }

    /** Drop every entry, keeping the storage for reuse. */
    void clear() {
        for (Slot& s : slots_) s.value = kNotFound;
        size_ = 0;
    }

    uint32_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

private:
    struct Slot {
        uint64_t key = 0;
        uint32_t value = kNotFound;
    };

    /** splitmix64's finalizer: avalanche a key into a probe index. */
    static uint64_t mix(uint64_t x) noexcept {
        x += 0x9E3779B97F4A7C15ull;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
        return x ^ (x >> 31);
    }

    void grow() {
        const size_t old_cap = slots_.size();
        const size_t new_cap = old_cap ? old_cap * 2 : 16;
        std::vector<Slot> old;
        old.swap(slots_);
        slots_.assign(new_cap, Slot{});
        size_ = 0;
        const uint32_t mask = static_cast<uint32_t>(new_cap) - 1u;
        for (const Slot& s : old) {
            if (s.value == kNotFound) continue;
            for (uint32_t i = static_cast<uint32_t>(mix(s.key)) & mask;; i = (i + 1u) & mask) {
                if (slots_[i].value == kNotFound) {
                    slots_[i] = s;
                    ++size_;
                    break;
                }
            }
        }
    }

    std::vector<Slot> slots_;
    uint32_t size_ = 0;
};

} // namespace mir
