// Copyright (c) Chemical Language Foundation 2025.
//
// Function-level MIR container: block metadata, dense value/place tables,
// cleanup scopes, move-path nodes and optional source locations.
// See mir-implementation-plan.md §2.4.

#pragma once

#include "MIRTypes.h"
#include "MIRArray.h"
#include "MIRInstruction.h"
#include "MIRMovePath.h"

#include <vector>

namespace mir {

/** Per-block metadata. The terminator is instructions[inst_start + inst_count]. */
struct MIRBlock {
    uint32_t inst_start = 0;      // index into MIRFunction::instructions
    uint32_t inst_count = 0;      // number of non-terminator instructions
    uint16_t arg_count = 0;       // block arguments
    uint16_t arg_type_offset = 0; // index into MIRFunction::block_arg_types
    BlockId id = MIR_NULL;
};

/** Dense, ValueId-indexed value definition. */
struct MIRValueDef {
    TypeId type = MIR_INVALID_ID;
    uint32_t def_inst = MIR_INVALID_ID;
    uint16_t use_count = 0; // saturating; used only for inline compaction
    uint8_t flags = VF_NONE;
};

/** Dense, PlaceId-indexed place definition. */
struct MIRPlaceDef {
    TypeId type = MIR_INVALID_ID;
    uint8_t storage_class = static_cast<uint8_t>(MIRStorageClass::Local);
    uint8_t init_state = static_cast<uint8_t>(MIRInitState::Uninitialized);
    uint32_t move_path_id = 0; // 0 = no ownership
    // For MIRStorageClass::FrameField: the ConstantId of the frame field name.
    uint32_t frame_field = MIR_INVALID_ID;
};

/** A lexical cleanup scope (function entry, block, loop, temporary region). */
struct MIRCleanupScope {
    uint32_t parent = 0;         // index of enclosing scope (0 = function entry)
    uint32_t owned_start = 0;    // index into MIRFunction::owned_places
    uint32_t owned_count = 0;
    BlockId unwind_target = MIR_NULL; // abnormal-exit target, or null
};

/** A compact source location. `source_index` in an instruction indexes these. */
struct MIRSourceLoc {
    uint32_t file = 0;
    uint32_t line = 0;
    uint32_t column = 0;
};

/**
 * A single function's MIR. Instruction/operand/block storage is arena-backed;
 * dense ID tables are ordinary vectors (module-lifetime, not per-instruction).
 */
struct MIRFunction {
    SymbolId symbol = MIR_INVALID_ID;
    TypeId function_type = MIR_INVALID_ID;
    uint32_t flags = 0; // e.g. has sret, is lambda, is comptime-eliminated

    MIRArray<MIRInstruction> instructions;
    MIRArray<MIROperand> operands;
    MIRArray<MIRBlock> blocks;

    std::vector<MIRValueDef> values;
    std::vector<MIRPlaceDef> places;
    std::vector<TypeId> block_arg_types;

    std::vector<MIRCleanupScope> cleanups;
    std::vector<PlaceId> owned_places;
    std::vector<MIRMovePathNode> move_paths;
    std::vector<MIRSourceLoc> sources;

    uint32_t entry_block = MIR_NULL;

    void clear() {
        instructions = {};
        operands = {};
        blocks = {};
        values.clear();
        places.clear();
        block_arg_types.clear();
        cleanups.clear();
        owned_places.clear();
        move_paths.clear();
        sources.clear();
        entry_block = MIR_NULL;
    }
};

} // namespace mir
