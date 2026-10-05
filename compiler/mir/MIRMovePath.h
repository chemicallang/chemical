// Copyright (c) Chemical Language Foundation 2025.
//
// Move-path tree: tracks ownership/initialization state per owned field and
// fixed-array element. Built lazily, only for owned aggregates.
// See mir-design.md §4.3.1 and mir-implementation-plan.md §2.8.

#pragma once

#include "MIRTypes.h"

namespace mir {

/**
 * One node of a move-path tree. Root node has parent_path == 0 with
 * root_place set; children form a sibling list via first_child/next_sibling.
 */
struct MIRMovePathNode {
    PlaceId root_place = MIR_NULL; // place this path belongs to
    uint32_t parent_path = 0;      // index of parent path (0 = root)
    uint32_t first_child = 0;      // index of first child (0 = none)
    uint32_t next_sibling = 0;     // index of next sibling (0 = none)
    uint8_t init_state = static_cast<uint8_t>(MIRInitState::Uninitialized);
    TypeId field_type = MIR_INVALID_ID;
};

} // namespace mir
