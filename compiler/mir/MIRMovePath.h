// Copyright (c) Chemical Language Foundation 2025.
//
// Move-path tree: tracks ownership/initialization state per owned field and
// fixed-array element. Built lazily, only for owned aggregates.
// See mir-design.md §4.3.1 and mir-implementation-plan.md §2.8.

#pragma once

#include "MIRTypes.h"
#include "MIRTypeTable.h"

namespace mir {

struct MIRFunction;

/**
 * One node of a move-path tree. The root node has
 * `parent_path == MIR_INVALID_ID` and `root_place` set; children form a sibling
 * list via `first_child` / `next_sibling`. All link fields use
 * `MIR_INVALID_ID` for "none" so path id 0 is a valid node.
 *
 * `field_index` identifies a direct aggregate field and `array_index` a fixed
 * array element. Exactly one of them is set on a non-root node; both are
 * `MIR_INVALID_ID` on the root.
 */
struct MIRMovePathNode {
    PlaceId root_place = MIR_NULL;           // place this path belongs to
    uint32_t parent_path = MIR_INVALID_ID;   // index of parent path
    uint32_t first_child = MIR_INVALID_ID;   // index of first child
    uint32_t next_sibling = MIR_INVALID_ID;  // index of next sibling
    uint8_t init_state = static_cast<uint8_t>(MIRInitState::Uninitialized);
    TypeId field_type = MIR_INVALID_ID;
    uint32_t field_index = MIR_INVALID_ID;   // direct aggregate field
    uint32_t array_index = MIR_INVALID_ID;   // fixed-array element
};

/**
 * True when `type` owns a resource whose move/drop state must be tracked.
 *
 * Today that is a destructor-bearing aggregate or an array of one. Aggregates
 * without a destructor still need move tracking for use-after-move, but their
 * native representation is trivial; the AST move tracker covers them until MIR
 * gains a Copy/move contract. Keeping this predicate conservative avoids
 * allocating a path tree for every scalar aggregate in the program.
 */
inline bool type_needs_move_path(const MIRTypeTable& types, TypeId type) {
    if (type == MIR_INVALID_ID || type >= types.types.size()) return false;
    const MIRTypeRecord& rec = types.types[type];
    if (rec.flags & TF_HAS_DESTRUCTOR) return true;
    switch (rec.kind) {
        case MIRTypeKind::Array:
            return type_needs_move_path(types, rec.element);
        case MIRTypeKind::Struct:
        case MIRTypeKind::Union:
        case MIRTypeKind::Variant: {
            for (uint32_t i = 0; i < rec.data_count; ++i) {
                const MIRFieldInfo* field = types.field_of(type, i);
                if (field != nullptr && type_needs_move_path(types, field->type)) {
                    return true;
                }
            }
            return false;
        }
        default:
            return false;
    }
}

/** Append a move-path tree for `place` to `function.move_paths`; returns root id. */
uint32_t append_move_path_tree(MIRFunction& function, const MIRTypeTable& types,
                               PlaceId place, TypeId type);

} // namespace mir
