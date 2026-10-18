// Copyright (c) Chemical Language Foundation 2025.
//
// MIRFunctionFacts — see MIRFacts.h. This is the exact analysis the C emitter
// used to run inside `emit_function_c`, moved to MIR core so every backend
// reads the same answer.

#include "MIRFacts.h"
#include "MIRTraversal.h"

namespace mir {

namespace {

/** Opcodes whose operand 0 is a written place. */
bool writes_place(MIROpcode op) {
    switch (op) {
        case MIROpcode::Store:
        case MIROpcode::Assign:
        case MIROpcode::CopyInit:
        case MIROpcode::MoveInit:
        case MIROpcode::Init:
        case MIROpcode::MemCpy:
        case MIROpcode::MemSet:
            return true;
        default:
            return false;
    }
}

/** Type kinds a parameter place may be aliased to its argument for. */
bool aliasable_param_kind(MIRTypeKind kind) {
    switch (kind) {
        case MIRTypeKind::Void:
        case MIRTypeKind::Array:
        case MIRTypeKind::Struct:
        case MIRTypeKind::Union:
        case MIRTypeKind::Variant:
        case MIRTypeKind::Function:
        case MIRTypeKind::Opaque:
            return false;
        default:
            return true;
    }
}

} // namespace

void compute_function_facts(const MIRModule& module, const MIRFunction& function,
                            MIRFunctionFacts& out) {
    const uint32_t ninst = static_cast<uint32_t>(function.instructions.size());
    const uint32_t nplace = static_cast<uint32_t>(function.places.size());
    const uint32_t nblock = static_cast<uint32_t>(function.blocks.size());

    // ── blocks: where each starts, and which ones a terminator jumps to ──
    out.block_start.assign(nblock, MIR_INVALID_ID);
    for (uint32_t b = 0; b < nblock; ++b) {
        const MIRBlock& blk = function.blocks[b];
        if (blk.id < nblock && blk.inst_start != MIR_INVALID_ID) out.block_start[blk.id] = blk.inst_start;
    }

    // a jump to the block beginning at the very next instruction is a
    // fall-through, so it neither prints a jump nor requires a label
    out.block_referenced.assign(nblock, 0);
    for (uint32_t ii = 0; ii < ninst; ++ii) {
        const MIRInstruction& inst = function.instructions[ii];
        for (uint32_t i = 0; i < inst.operand_count; ++i) {
            const MIROperand* op = operand_at(function, inst, i);
            if (!op || op->kind() != MIROperandKind::Block) continue;
            if (op->id >= nblock) continue;
            if (out.is_fallthrough_inst(function, ii, op->id)) continue;
            out.block_referenced[op->id] = 1;
        }
    }

    out.label_at_inst.assign(ninst, MIR_INVALID_ID);
    out.block_starts_inst.assign(ninst, 0);
    for (uint32_t b = 0; b < nblock; ++b) {
        const MIRBlock& blk = function.blocks[b];
        if (blk.inst_start == MIR_INVALID_ID || blk.inst_start >= ninst) continue;
        out.block_starts_inst[blk.inst_start] = 1;
        if (blk.id >= nblock || !out.block_referenced[blk.id]) continue;
        out.label_at_inst[blk.inst_start] = blk.id;
    }

    // ── places: write counts, escape, and parameter aliasing ──
    out.place_writes.assign(nplace, 0);
    for (uint32_t ii = 0; ii < ninst; ++ii) {
        const MIRInstruction& inst = function.instructions[ii];
        if (!writes_place(inst.opcode())) continue;
        const MIROperand* d = operand_at(function, inst, 0);
        if (d && d->kind() == MIROperandKind::Place && d->id < nplace) {
            ++out.place_writes[d->id];
        }
    }

    // the C-argument index of each `Param`, independent of whether the Param
    // instruction itself is emitted (a single-use Param is inlined)
    out.param_index_at_inst.assign(ninst, MIR_INVALID_ID);
    {
        uint32_t pi = 0;
        for (uint32_t ii = 0; ii < ninst; ++ii) {
            if (function.instructions[ii].opcode() == MIROpcode::Param) {
                out.param_index_at_inst[ii] = pi++;
            }
        }
    }

    out.place_addressed.assign(nplace, 0);
    out.param_alias.assign(nplace, MIR_INVALID_ID);
    {
        // the argument index a place's single store copies, when that store
        // stores the parameter value itself
        // reused across calls so the candidate table does not reallocate per function
        static thread_local std::vector<uint32_t> candidate;
        candidate.assign(nplace, MIR_INVALID_ID);
        auto is_param_place = [&](const MIROperand* op) {
            return op && op->kind() == MIROperandKind::Place && op->id < nplace &&
                   function.places[op->id].storage_class ==
                       static_cast<uint8_t>(MIRStorageClass::Parameter);
        };
        for (uint32_t ii = 0; ii < ninst; ++ii) {
            const MIRInstruction& inst = function.instructions[ii];
            const bool readable = inst.opcode() == MIROpcode::Load;
            const bool writable = inst.opcode() == MIROpcode::Store;
            const MIROperand* d = operand_at(function, inst, 0);
            if (is_param_place(d)) {
                if (!readable && !writable) {
                    out.place_addressed[d->id] = 1;
                } else if (writable && candidate[d->id] == MIR_INVALID_ID) {
                    const MIROperand* s = operand_at(function, inst, 1);
                    if (s && s->kind() == MIROperandKind::Value && s->id < function.values.size()) {
                        const MIRValueDef& vd = function.values[s->id];
                        if (vd.def_inst != MIR_INVALID_ID && vd.def_inst < ninst &&
                            function.instructions[vd.def_inst].opcode() == MIROpcode::Param) {
                            candidate[d->id] = out.param_index_at_inst[vd.def_inst];
                        }
                    }
                }
            }
            // only operand 0 of a plain read/write may name the place
            for (uint32_t i = 1; i < inst.operand_count; ++i) {
                const MIROperand* op = operand_at(function, inst, i);
                if (is_param_place(op)) out.place_addressed[op->id] = 1;
            }
        }
        const uint32_t nparams = (function.function_type < module.types.size())
                                     ? module.types.get(function.function_type).data_count
                                     : 0;
        for (PlaceId p = 0; p < nplace; ++p) {
            if (candidate[p] == MIR_INVALID_ID || candidate[p] >= nparams) continue;
            // exactly one write (that store) and no address taken anywhere
            if (out.place_writes[p] != 1 || out.place_addressed[p]) continue;
            const MIRPlaceDef& pd = function.places[p];
            if (pd.type >= module.types.size()) continue;
            if (!aliasable_param_kind(module.types.get(pd.type).kind)) continue;
            out.param_alias[p] = candidate[p];
        }
    }
}

void compute_value_facts(const MIRModule& module, const MIRFunction& function,
                         MIRValueFacts& out) {
    (void) module;
    const uint32_t nval = static_cast<uint32_t>(function.values.size());
    // `Count` is not a real opcode, so it is the "no defining instruction"
    // sentinel (0 is a valid opcode — `Nop`).
    const uint16_t none = static_cast<uint16_t>(MIROpcode::Count);
    out.def_opcode.assign(nval, none);
    out.single_use.assign(nval, 0);
    out.single_use_pure.assign(nval, 0);

    const uint32_t ninst = static_cast<uint32_t>(function.instructions.size());
    for (uint32_t ii = 0; ii < ninst; ++ii) {
        const MIRInstruction& inst = function.instructions[ii];
        const ValueId v = inst.result_or_place;
        if (v >= nval) continue;
        // `result_or_place` also holds a place id for place-producing opcodes,
        // which can collide with a value id; the recorded defining instruction
        // filters those out.
        if (function.values[v].def_inst != ii) continue;
        out.def_opcode[v] = static_cast<uint16_t>(inst.opcode());
    }

    for (ValueId v = 0; v < nval; ++v) {
        if (function.values[v].use_count != 1) continue;
        out.single_use[v] = 1;
        const uint16_t op = out.def_opcode[v];
        if (op != none && opcode_is_pure(static_cast<MIROpcode>(op))) {
            out.single_use_pure[v] = 1;
        }
    }
}

} // namespace mir
