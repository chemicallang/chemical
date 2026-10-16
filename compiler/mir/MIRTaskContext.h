// Copyright (c) Chemical Language Foundation 2025.
//
// Per-worker MIR lowering context. Owns the mutable working set one lowering
// task needs — a lowerer, a bump arena and the private C buffer for the
// function being emitted — so that (E3) several tasks can run concurrently,
// each producing one function artifact the coordinator merges into the module
// writer in source order.
//
// Serial compilation creates exactly one context and drives the module loop
// through it; that is the same code path a worker thread runs, so the serial
// and parallel paths cannot drift. See mir-implementation-plan.md Stage 9 and
// mir-design.md §14.0.

#pragma once

#include "MIRModule.h"
#include "MIRTypeBuilder.h"
#include "MIRLowerer.h"
#include "MIRArena.h"

#include <string>

namespace mir {

/**
 * One lowering task's working set.
 *
 * - `lowerer` lowers one function at a time (AST -> MIR). It holds the
 *   function-local bindings (variable -> place, callee -> symbol) and the
 *   module-scoped caches that stay warm for the module's lifetime.
 * - `arena` is the function's bump arena: every MIR record the lowered function
 *   owns is allocated from it, so the whole function is discarded by one
 *   `reset()`.
 * - `out` is the private C buffer the function's body (and any lambda bodies it
 *   materialized) is emitted into. The coordinator appends it to the module
 *   writer once, at the function's source-order slot.
 *
 * Not copyable or movable: `MIRLowerer` holds references to the module tables,
 * so a context is constructed in place.
 */
struct MIRTaskContext {
    MIRLowerer lowerer;
    MIRArena arena;
    std::string out;
    /** Reused scratch for one function's emitted body, so emitting does not
     *  allocate a fresh string per function (its capacity persists). */
    std::string body;

    MIRTaskContext(MIRModule& module, MIRTypeBuilder& types)
        : lowerer(module, types), arena() {}

    MIRTaskContext(const MIRTaskContext&) = delete;
    MIRTaskContext& operator=(const MIRTaskContext&) = delete;

    /**
     * Begin a fresh function: drop the previous C buffer and, when the caller
     * does not need the MIR retained (the C backend emits immediately), rewind
     * the arena. The LLVM backend keeps every function's MIR alive for the
     * module-wide emission, so it passes false.
     */
    void begin_function(bool reset_arena) {
        out.clear();
        if (reset_arena) arena.reset();
    }

    MIRModule& module() { return lowerer.module(); }
    MIRTypeBuilder& types() { return lowerer.types(); }
};

} // namespace mir
