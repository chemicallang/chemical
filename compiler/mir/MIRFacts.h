// Copyright (c) Chemical Language Foundation 2025.
//
// Backend-neutral per-function facts, computed once from a lowered function and
// read by the emitters. This is action B of mir-migration-evaluation.md: the
// facts a backend needs before it can spill a place, alias a parameter or place
// a label are MIR's to publish, not each emitter's to re-derive. The C emitter
// computes them today (it is the one with a `place`-based code shape); the LLVM
// emitter promotes places with mem2reg and needs none, and a JVM/JS backend
// reads the same struct instead of growing its own analysis.
//
// AST-free and target-free (it reads the function and the interned type table
// only), so it lives in MIR core and is covered by the standalone MIRTests
// target. Nothing here spells anything: the facts are ids and counts.

#pragma once

#include "MIRModule.h"

#include <cstdint>
#include <vector>

namespace mir {

/**
 * Per-value facts, by `ValueId` — the classification a compacting backend
 * applies before it recomputes a value at its use site. `MIRValueDef` already
 * carries the raw `use_count`/`def_inst`; this derives the portable predicates
 * from them plus the defining opcode's static flags, so the C emitter and a
 * future JVM/JS backend share one answer instead of each re-deriving it.
 */
struct MIRValueFacts {
    /**
     * The opcode defining each value, by `ValueId`, or
     * `static_cast<uint16_t>(MIROpcode::Count)` when the value has no defining
     * instruction.
     */
    std::vector<uint16_t> def_opcode;
    /** 1 when the value is used exactly once (a single compaction candidate). */
    std::vector<uint8_t> single_use;
    /**
     * 1 when the value is `single_use` *and* its defining opcode is pure (no
     * side effects — `opcode_flags & OF_Pure`): a backend may recompute it at
     * its use site without changing observable behavior. The C emitter layers
     * its own policy (integer promotion, load fusion) on top.
     */
    std::vector<uint8_t> single_use_pure;
};

/** Compute `out` for `function`, which must belong to `module`. */
void compute_value_facts(const MIRModule& module, const MIRFunction& function,
                         MIRValueFacts& out);

/**
 * Everything a backend needs to know about a function's shape before emitting
 * it. Turing-complete *analysis*, not policy: what a backend does with
 * `param_alias` (skip the spill) or `label_at_inst` (print a label) is its own
 * business.
 *
 * Every vector is indexed by the id its name says; a vector is sized for the
 * whole function even when a fact is MIR_INVALID_ID (0 is a valid id, so
 * sentinels — not length — mark "absent").
 */
struct MIRFunctionFacts {
    /** Instruction index each block begins at, by `BlockId`. */
    std::vector<uint32_t> block_start;
    /** 1 at every instruction index that begins a block. */
    std::vector<uint8_t> block_starts_inst;
    /**
     * 1 for a block some terminator operand *jumps* to, by `BlockId`. A branch
     * to the block that starts at the very next instruction is a fall-through
     * and is not a jump — see `is_fallthrough_inst`.
     */
    std::vector<uint8_t> block_referenced;
    /** Label block id at each instruction index, or MIR_INVALID_ID for none. */
    std::vector<BlockId> label_at_inst;
    /**
     * 1 at the instruction index of a "trampoline" block — a block with no
     * instructions (`inst_count == 0`) whose sole terminator is an
     * unconditional `Br` to the block that begins on the very next slot. Such a
     * block emits nothing: its `Br` is a fall-through and its label is dead, so
     * a jump to it is redirected to its target (`resolve_block`). By instruction
     * index.
     */
    std::vector<uint8_t> inst_transparent_br;
    /** By `BlockId`: the first non-trampoline block a jump to it resolves to. */
    std::vector<BlockId> block_resolve;

    /** Writes (destination operand of the writing opcodes) per place, by `PlaceId`. */
    std::vector<uint32_t> place_writes;
    /**
     * 1 when a place is used somewhere other than operand 0 of a plain
     * load/store — i.e. its address may escape and it must keep its own storage.
     */
    std::vector<uint8_t> place_addressed;
    /** C-argument-style index of each `Param` instruction, by instruction index. */
    std::vector<uint32_t> param_index_at_inst;
    /**
     * A by-value scalar (non-aggregate, non-array) parameter place that is
     * written exactly once, by its own `Param` spill, and never addressed *is*
     * its argument: the value a backend would spill is already in the argument
     * slot, so the spill (and its storage) can be skipped. Holds the argument
     * index, or MIR_INVALID_ID when the place must keep its own storage.
     *
     * Aggregates are deliberately excluded: such a place holds either a copy of
     * the pointee or the hidden argument pointer, and skipping its by-value
     * semantics would be wrong.
     */
    std::vector<uint32_t> param_alias;

    /** The first non-trampoline block a jump to `block_id` resolves to. */
    BlockId resolve_block(BlockId block_id) const {
        return block_id < block_resolve.size() && block_resolve[block_id] != MIR_INVALID_ID
                   ? block_resolve[block_id]
                   : block_id;
    }

    /**
     * True when the block `block_id` resolves to begins right after instruction
     * `inst`. Trampoline blocks between the two emit nothing (their `Br` is a
     * fall-through), so they are stepped over: a branch into a trampoline that
     * lands on the very next slot falls through to the trampoline's target.
     */
    bool is_fallthrough_inst(const MIRFunction& fn, uint32_t inst, BlockId block_id) const {
        const BlockId b = resolve_block(block_id);
        if (b >= block_start.size()) return false;
        const uint32_t s = block_start[b];
        if (s == MIR_INVALID_ID) return false;
        uint32_t next = inst + 1;
        while (next < fn.instructions.size() && next < inst_transparent_br.size() &&
               inst_transparent_br[next]) {
            ++next;
        }
        return s == next;
    }
};

/** Compute `out` for `function`, which must belong to `module`. */
void compute_function_facts(const MIRModule& module, const MIRFunction& function,
                            MIRFunctionFacts& out);

} // namespace mir
