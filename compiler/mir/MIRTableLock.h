// Copyright (c) Chemical Language Foundation 2025.
//
// Lock policy for the shared module tables (`MIRTypeTable`, `MIRConstantPool`,
// `MIRSymbolTable`).
//
// Those tables support several lowering workers interning concurrently (action
// E1), so every access takes a `std::shared_mutex`. That is required when the
// pipeline really is parallel — but the shipping pipeline is single-writer: one
// thread lowers and emits, so the lock is pure overhead on the hottest lookups
// (a type/constant read per emitted operand, a type intern per lowered
// expression).
//
// A table is therefore told, once, whether its owner guarantees a single
// *writer* thread (`MIRTableLockPolicy::set_single_threaded`). Concurrent
// *readers* are always fine: the flag says "no other thread mutates", so a
// reader may skip the lock whenever it holds. Mutation still uses the mutex
// whenever the flag is off, so an accidentally-parallel caller keeps the old
// (correct) behaviour rather than racing.

#pragma once

#include <atomic>
#include <shared_mutex>

namespace mir {

/** The "is there more than one writer?" flag a table lock consults. */
class MIRTableLockPolicy {
public:
    void set_single_threaded(bool v) noexcept {
        single_threaded_.store(v, std::memory_order_relaxed);
    }
    bool single_threaded() const noexcept {
        return single_threaded_.load(std::memory_order_relaxed);
    }

private:
    std::atomic<bool> single_threaded_{false};
};

/**
 * Shared (reader) lock over a table: no lock at all when the owner is
 * single-writer, a plain `shared_lock` otherwise. Named `lock` at the use site,
 * exactly like the `std::shared_lock` it replaced.
 */
class MIRReadLock {
public:
    MIRReadLock(std::shared_mutex& mtx, bool single_threaded)
        : lk_(mtx, std::defer_lock) {
        if (!single_threaded) lk_.lock();
    }

private:
    std::shared_lock<std::shared_mutex> lk_;
};

/** Exclusive (writer) lock over a table, elided for a single-writer owner. */
class MIRWriteLock {
public:
    MIRWriteLock(std::shared_mutex& mtx, bool single_threaded)
        : lk_(mtx, std::defer_lock) {
        if (!single_threaded) lk_.lock();
    }

private:
    std::unique_lock<std::shared_mutex> lk_;
};

} // namespace mir
