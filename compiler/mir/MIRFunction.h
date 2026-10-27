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
    /**
     * The lowering gave this place per-field/per-element drop flags, so it may
     * be initialized one owned child at a time. MIRMoveCheck only accepts a
     * partial write when the destination place sets this.
     */
    bool field_wise_drop = false;
    /** A mutable raw pointer escaped from this place (unsafe: trust writes). */
    bool raw_escaped = false;
    uint32_t move_path_id = MIR_INVALID_ID; // MIR_INVALID_ID = no ownership
    /** For a by-value/reference parameter place: its declaration index. */
    uint32_t param_index = MIR_INVALID_ID;
    // For MIRStorageClass::FrameField: the ConstantId of the frame field name.
    uint32_t frame_field = MIR_INVALID_ID;
    /**
     * The source identifier this place was allocated for, as a String
     * `ConstantId` (`MIR_INVALID_ID` = unnamed — a temporary, a call result, a
     * coroutine frame field). Filled by `MIRLowerer::bind_name` **only when
     * debug info is enabled**, and read by the LLVM emitter to attach a
     * `DILocalVariable` + `llvm.dbg.declare` to the place's alloca. The C
     * emitter ignores it (it names places `__chx_pN`).
     */
    uint32_t debug_name = MIR_INVALID_ID;
    /**
     * The lexical scope this place was declared in — an index into
     * `MIRFunction::lexical_scopes`, or `MIR_INVALID_ID` for the function scope
     * (a parameter, a temporary). Debug info only; it decides which
     * `DILexicalBlock` owns the place's `DILocalVariable`.
     */
    uint32_t debug_scope = MIR_INVALID_ID;
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

/** Sentinel for `MIRInstruction::source_index` / `MIRBuilder::set_source`. */
inline constexpr uint32_t MIR_NO_SOURCE = 0xFFFFu;

/**
 * A source-level lexical scope, recorded only for debug info. The extent is a
 * half-open range of the *instruction stream* in emission order (the order the
 * lowerer walked the AST), which is what a debugger's `DILexicalBlock` needs —
 * source nesting, not a runtime property. See `MIRBuilder::push_lexical_scope`.
 */
struct MIRLexicalScope {
    uint32_t parent = MIR_INVALID_ID;     // enclosing scope; MIR_INVALID_ID = function
    uint32_t inst_begin = MIR_INVALID_ID; // first instruction in the scope
    uint32_t inst_end = MIR_INVALID_ID;   // one past the last; INVALID = to the end
    MIRSourceLoc loc{};                   // the scope's own source location
    bool has_loc = false;
};

/** Bits of `MIRFunction::flags`. */
enum MIRFunctionFlags : uint32_t {
    /**
     * An auto-generated stub for a static interface method (see
     * `MIRLowerer::lower_static_interface_stub`). A static interface's method
     * and the methods of every impl of it mangle to the *same* symbol, so the
     * stub is emitted weak: a real implementation, wherever it lives, wins at
     * link time.
     */
    MF_WEAK_STUB = 1u << 0,
};

/**
 * A single function's MIR. Instruction/operand/block storage is arena-backed;
 * dense ID tables are ordinary vectors (module-lifetime, not per-instruction).
 */
struct MIRFunction {
    SymbolId symbol = MIR_INVALID_ID;
    /** Index into `MIRModule::region_signatures` (S4.2), or MIR_INVALID_ID. */
    uint32_t region_signature = MIR_INVALID_ID;
    /** Highest region variable id allocated in this function (S4.4b). */
    uint32_t region_var_count = 0;
    TypeId function_type = MIR_INVALID_ID;
    uint32_t flags = 0; // MF_WEAK_STUB; e.g. has sret, is lambda, is comptime-eliminated

    MIRArray<MIRInstruction> instructions;
    MIRArray<MIROperand> operands;
    MIRArray<MIRBlock> blocks;

    std::vector<MIRValueDef> values;
    std::vector<MIRPlaceDef> places;
    std::vector<TypeId> block_arg_types;

    std::vector<MIRCleanupScope> cleanups;
    std::vector<MIRLexicalScope> lexical_scopes;
    std::vector<PlaceId> owned_places;
    std::vector<MIRMovePathNode> move_paths;
    std::vector<MIRSourceLoc> sources;

    /**
     * The innermost lexical scope covering instruction `inst`, or
     * `MIR_INVALID_ID` when the instruction belongs to the function scope.
     * Scopes are pushed and popped LIFO, so their instruction ranges nest and a
     * nested scope always has a *higher* index than the scope it nests in — hence
     * the highest-index range containing `inst` is the innermost one. Used only
     * to pick a `DILexicalBlock` for debug locations.
     */
    uint32_t lexical_scope_of_inst(uint32_t inst) const {
        uint32_t best = MIR_INVALID_ID;
        for (uint32_t s = 0; s < lexical_scopes.size(); ++s) {
            const MIRLexicalScope& sc = lexical_scopes[s];
            if (sc.inst_begin == MIR_INVALID_ID) continue;
            const uint32_t end = sc.inst_end == MIR_INVALID_ID
                                     ? static_cast<uint32_t>(instructions.size())
                                     : sc.inst_end;
            if (inst >= sc.inst_begin && inst < end) best = s;
        }
        return best;
    }

    /** The declaration's source location (for a DISubprogram). */
    MIRSourceLoc decl_loc;
    bool has_decl_loc = false;
    /** For a generated function (lambda/destructor): the enclosing function. */
    uint32_t parent_symbol = MIR_INVALID_ID;

    uint32_t entry_block = MIR_NULL;

    void clear() {
        instructions = {};
        operands = {};
        blocks = {};
        values.clear();
        places.clear();
        block_arg_types.clear();
        cleanups.clear();
        lexical_scopes.clear();
        owned_places.clear();
        move_paths.clear();
        region_signature = MIR_INVALID_ID;
        region_var_count = 0;
        has_decl_loc = false;
        parent_symbol = MIR_INVALID_ID;
        sources.clear();
        entry_block = MIR_NULL;
    }
};

} // namespace mir
