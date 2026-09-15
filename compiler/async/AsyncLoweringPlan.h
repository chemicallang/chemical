// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <vector>
#include "std/chem_string_view.h"

class BaseType;
class ASTNode;

/**
 * One frame-backed storage slot (design Section 8.8).
 *
 * `destructible` is true when the slot's type has a destructor; such slots are
 * candidates for a `live_drops` entry or a drop flag.
 *
 * `node` is the AST node that owns the slot (`FunctionParam*` or
 * `VarInitStatement*`) so backends can map a local/parameter to its frame field.
 */
struct AsyncFrameSlot {
    chem::string_view name;
    BaseType* type = nullptr;
    bool destructible = false;
    ASTNode* node = nullptr;
};

/**
 * A single suspension point in an async function body (design Section 6.5).
 *
 * `resume_state` is the state value the coroutine stores before returning
 * `Pending` at this site; on resume the poll dispatch jumps to the site.
 *
 * `live_drops` are frame slot ids whose destructors must run if the future is
 * cancelled while suspended at this site, in reverse creation order.
 */
struct AwaitSite {
    unsigned resume_state = 0;
    std::vector<unsigned> live_drops;

    /**
     * the ids of every frame slot that is in scope at this site (parameters and
     * live locals), in creation order. Backends spill these before returning
     * `Pending` and reload them on resume.
     */
    std::vector<unsigned> live_slots;

    BaseType* awaited_type = nullptr;

    /**
     * the type of the operand `e` of `await e` (a `FutureHandle<T>` when the
     * operand is an async call). Backends store it in a frame field so the
     * child future survives suspension.
     */
    BaseType* awaited_handle_type = nullptr;

    /**
     * the `VarInitStatement` that holds this await (`var __chx_await_N = await e`).
     */
    ASTNode* var_init = nullptr;
};

/**
 * The shared lowering plan for one async function. Produced by a single pass
 * over the (normalized) body and consumed by both backends. It is the contract
 * between front end and code generation (design Section 6.5).
 */
struct AsyncLoweringPlan {
    std::vector<AwaitSite> sites;
    unsigned frame_size = 0;
    unsigned frame_align = 0;
    bool needs_frame = false;
    bool result_has_destructor = false;

    /**
     * frame-backed slots in creation order: parameters first, then locals.
     * Slot indices are the ids used by `AwaitSite::live_drops`.
     */
    std::vector<AsyncFrameSlot> slots;

    /**
     * number of frame-backed slots (parameters + locals).
     */
    unsigned slot_count = 0;
};
