// Copyright (c) Chemical Language Foundation 2025.
//
// Lock policy for the shared module tables (`MIRTypeTable`, `MIRConstantPool`,
// `MIRSymbolTable`).
//
// Reads of those tables are lock-free against their stable storage (action E3d,
// see MIRStableArray.h / MIRBytePool.h): a reader never races an append. Only
// *mutation* (interning, appending a field/payload) needs exclusion, and only
// when more than one lowering worker runs. A table is therefore told, once,
// whether its owner guarantees a single *writer* thread
// (`MIRTableLockPolicy::set_single_threaded`); when it does, the mutators skip
// the lock entirely. When it does not, the mutators take the exclusive lock, so
// an accidentally-parallel caller keeps the old (correct) behaviour.

#pragma once

#include <atomic>
#include <mutex>
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
