// Copyright (c) Chemical Language Foundation 2025.
//
// Shared MIR traversal, operand-decode and naming services (action B of
// mir-migration-evaluation.md). The block/instruction walk and the operand,
// symbol and constant lookups every backend performs used to be copied into
// each emitter: the C emitter, the fact pass and the LLVM emitter each carried
// their own `operand_at`, and the LLVM emitter hand-rolled the same symbol and
// constant access three more times. They are pure functions of the MIR module
// and function — AST-free and target-free — so they live here and a new backend
// reads them instead of re-deriving them. Nothing here spells anything: the
// callers own the target syntax.

#pragma once

#include "MIRFunction.h"
#include "MIRModule.h"

#include <cstdint>
#include <string_view>

namespace mir {

/**
 * The `i`th operand of `inst`, or nullptr when the instruction has fewer. The
 * operands of one instruction are contiguous in the function's operand array;
 * every backend decodes through this one accessor.
 */
inline const MIROperand* operand_at(const MIRFunction& fn, const MIRInstruction& inst,
                                    uint32_t i) {
    if (i >= inst.operand_count) return nullptr;
    return &fn.operands[inst.operand_offset + i];
}

/** The half-open instruction range [begin, end) of a block (terminator included). */
struct MIRBlockRange {
    uint32_t begin;
    uint32_t end;
};

/**
 * The instruction range a block owns: `inst_count` non-terminators followed by
 * its terminator, clamped to the function's stream. A block with no valid start
 * yields an empty range.
 */
inline MIRBlockRange block_range(const MIRFunction& fn, const MIRBlock& blk) {
    const uint32_t n = static_cast<uint32_t>(fn.instructions.size());
    const uint32_t begin = blk.inst_start;
    if (begin >= n) return {n, n};
    uint32_t end = begin + blk.inst_count + 1u; // + the terminator slot
    if (end > n) end = n;
    return {begin, end};
}

/** Visit every block of `fn` as `f(block_index, block)`. */
template <typename F>
inline void for_each_block(const MIRFunction& fn, F&& f) {
    const uint32_t n = static_cast<uint32_t>(fn.blocks.size());
    for (uint32_t bi = 0; bi < n; ++bi) f(bi, fn.blocks[bi]);
}

/** Visit every instruction of one block (terminator included) as `f(index, inst)`. */
template <typename F>
inline void for_each_block_inst(const MIRFunction& fn, const MIRBlock& blk, F&& f) {
    const MIRBlockRange r = block_range(fn, blk);
    for (uint32_t ii = r.begin; ii < r.end; ++ii) f(ii, fn.instructions[ii]);
}

/**
 * A symbol's linker name as a view into the module's stable name pool (its
 * mangled spelling, falling back to the plain name). Empty for an out-of-range
 * id. Use this instead of re-reading the record's offset/length fields.
 */
inline std::string_view symbol_name_view(const MIRModule& m, SymbolId sym) {
    if (sym == MIR_INVALID_ID || sym >= m.symbols.size()) return {};
    const MIRSymbolRecord rec = m.symbols.get(sym);
    return m.symbols.mangled_view(rec);
}

/**
 * The interned bytes of a String constant as a view into the module's stable
 * data pool. Empty for an out-of-range id or a non-string constant.
 */
inline std::string_view constant_string_view(const MIRModule& m, ConstantId cid) {
    if (cid >= m.constants.size()) return {};
    const MIRConstant c = m.constants.get(cid);
    if (c.kind != MIRConstantKind::String) return {};
    return m.constants.data.text(c.data_offset);
}

} // namespace mir
