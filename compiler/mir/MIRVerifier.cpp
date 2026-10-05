// Copyright (c) Chemical Language Foundation 2025.

#include "MIRVerifier.h"

namespace mir {

namespace {

void diag(MIRVerifyResult& r, uint32_t fn, uint32_t block, uint32_t inst,
          std::string msg) {
    MIRVerifyDiagnostic d;
    d.function_index = fn;
    d.block_index = block;
    d.inst_index = inst;
    d.message = std::move(msg);
    r.diagnostics.push_back(std::move(d));
}

void verify_operand_bounds(MIRVerifyResult& r, const MIRFunction& fn,
                           uint32_t block_idx, uint32_t inst_idx,
                           const MIROperand& op) {
    switch (op.kind()) {
        case MIROperandKind::Value:
            if (op.id != MIR_NULL && op.id >= fn.values.size()) {
                diag(r, MIR_INVALID_ID, block_idx, inst_idx, "value operand out of range");
            }
            break;
        case MIROperandKind::Place:
            if (op.id != MIR_NULL && op.id >= fn.places.size()) {
                diag(r, MIR_INVALID_ID, block_idx, inst_idx, "place operand out of range");
            }
            break;
        case MIROperandKind::Block:
            if (op.id >= fn.blocks.size()) {
                diag(r, MIR_INVALID_ID, block_idx, inst_idx, "block operand out of range");
            }
            break;
        case MIROperandKind::Symbol:
        case MIROperandKind::Constant:
        case MIROperandKind::Type:
            // module-level; checked by verify_module when needed
            break;
    }
}

} // namespace

MIRVerifyResult verify_function(const MIRFunction& function) {
    MIRVerifyResult r;

    const uint32_t n_inst = function.instructions.size();
    const uint32_t n_ops = function.operands.size();

    for (uint32_t b = 0; b < function.blocks.size(); ++b) {
        const MIRBlock& block = function.blocks[b];

        if (block.id != b) {
            diag(r, MIR_INVALID_ID, b, MIR_INVALID_ID, "block.id does not match its index");
        }
        if (block.inst_start == MIR_INVALID_ID) {
            diag(r, MIR_INVALID_ID, b, MIR_INVALID_ID, "block was never positioned (no set_block)");
            continue;
        }
        if (block.inst_start > n_inst || block.inst_start + block.inst_count > n_inst) {
            diag(r, MIR_INVALID_ID, b, MIR_INVALID_ID, "block instruction range out of bounds");
            continue;
        }

        if (block.arg_count) {
            if (block.arg_type_offset + block.arg_count > function.block_arg_types.size()) {
                diag(r, MIR_INVALID_ID, b, MIR_INVALID_ID, "block argument types out of range");
            }
        }

        // non-terminator instructions
        for (uint32_t i = 0; i < block.inst_count; ++i) {
            const uint32_t idx = block.inst_start + i;
            const MIRInstruction& inst = function.instructions[idx];
            if (is_terminator(inst.opcode())) {
                diag(r, MIR_INVALID_ID, b, idx, "terminator inside block body");
            }
            if (inst.operand_offset > n_ops ||
                inst.operand_offset + inst.operand_count > n_ops) {
                diag(r, MIR_INVALID_ID, b, idx, "instruction operand range out of bounds");
                continue;
            }
            for (uint16_t k = 0; k < inst.operand_count; ++k) {
                verify_operand_bounds(r, function, b, idx,
                                      function.operands[inst.operand_offset + k]);
            }
        }

        // the terminator must exist and be a terminator
        const uint32_t term_idx = block.inst_start + block.inst_count;
        if (term_idx >= n_inst) {
            diag(r, MIR_INVALID_ID, b, MIR_INVALID_ID, "block has no terminator");
        } else {
            const MIRInstruction& term = function.instructions[term_idx];
            if (!is_terminator(term.opcode())) {
                diag(r, MIR_INVALID_ID, b, term_idx, "block does not end in a terminator");
            }
            if (term.operand_offset > n_ops ||
                term.operand_offset + term.operand_count > n_ops) {
                diag(r, MIR_INVALID_ID, b, term_idx, "terminator operand range out of bounds");
            } else {
                for (uint16_t k = 0; k < term.operand_count; ++k) {
                    verify_operand_bounds(r, function, b, term_idx,
                                          function.operands[term.operand_offset + k]);
                }
            }
        }
    }

    if (function.blocks.empty()) {
        diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID, "function has no blocks");
    } else if (function.entry_block >= function.blocks.size()) {
        diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID, "entry block out of range");
    }

    return r;
}

MIRVerifyResult verify_module(const MIRModule& module) {
    MIRVerifyResult r;
    for (uint32_t f = 0; f < module.functions.size(); ++f) {
        MIRVerifyResult fr = verify_function(module.functions[f]);
        for (MIRVerifyDiagnostic& d : fr.diagnostics) {
            d.function_index = f;
            r.diagnostics.push_back(std::move(d));
        }
    }
    return r;
}

} // namespace mir
