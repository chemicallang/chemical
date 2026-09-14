// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include <vector>
#include "std/chem_string_view.h"

class BaseType;

/**
 * One frame-backed storage slot (design Section 8.8).
 *
 * `destructible` is true when the slot's type has a destructor; such slots are
 * candidates for a `live_drops` entry or a drop flag.
 */
struct AsyncFrameSlot {
    chem::string_view name;
    BaseType* type = nullptr;
    bool destructible = false;
};

/**
 * A single suspension point in an async function body (design Section 6.5).
 *
 * `resume_state` is the state value the coroutine stores before returning
 * `Pending` at this site; on resume the poll loop dispatches to it.
 *
 * `live_drops` are frame slot ids whose destructors must run if the future is
 * cancelled while suspended at this site, in reverse creation order.
 */
struct AwaitSite {
    unsigned resume_state = 0;
    std::vector<unsigned> live_drops;
    BaseType* awaited_type = nullptr;
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
