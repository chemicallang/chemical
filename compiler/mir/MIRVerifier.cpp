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

void verify_move_paths(MIRVerifyResult& r, const MIRFunction& function) {
    const uint32_t n_paths = static_cast<uint32_t>(function.move_paths.size());
    for (uint32_t p = 0; p < n_paths; ++p) {
        const MIRMovePathNode& node = function.move_paths[p];
        if (node.root_place >= function.places.size()) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "move path root_place out of range");
        }
        if (node.init_state > static_cast<uint8_t>(MIRInitState::Destroyed)) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "move path has an invalid init state");
        }
        if (node.parent_path != MIR_INVALID_ID &&
            (node.parent_path >= n_paths || node.parent_path == p)) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "move path parent out of range or self-referential");
        }
        if (node.first_child != MIR_INVALID_ID && node.first_child >= n_paths) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "move path first_child out of range");
        }
        if (node.next_sibling != MIR_INVALID_ID && node.next_sibling >= n_paths) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "move path next_sibling out of range");
        }
        if (node.field_index != MIR_INVALID_ID && node.array_index != MIR_INVALID_ID) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "move path node has both a field and an array index");
        }
        if (node.parent_path == MIR_INVALID_ID) {
            if (node.field_index != MIR_INVALID_ID || node.array_index != MIR_INVALID_ID) {
                diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                     "move path root has a projection index");
            }
            if (node.root_place < function.places.size() &&
                function.places[node.root_place].move_path_id != p) {
                diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                     "move path root is not referenced by its place");
            }
        }
        // every child and sibling must point back at this node as its parent
        uint32_t child = node.first_child;
        uint32_t guard = 0;
        while (child != MIR_INVALID_ID && child < n_paths && guard++ < n_paths) {
            if (function.move_paths[child].parent_path != p) {
                diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                     "move path child has the wrong parent");
            }
            child = function.move_paths[child].next_sibling;
        }
        if (guard >= n_paths && child != MIR_INVALID_ID) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "move path sibling list does not terminate");
        }
    }
    for (uint32_t place = 0; place < function.places.size(); ++place) {
        const uint32_t path = function.places[place].move_path_id;
        if (path == MIR_INVALID_ID) continue;
        if (path >= n_paths) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "place move_path_id out of range");
            continue;
        }
        if (function.move_paths[path].root_place != place) {
            diag(r, MIR_INVALID_ID, MIR_INVALID_ID, MIR_INVALID_ID,
                 "place move path points at another place");
        }
    }
}

} // namespace

MIRVerifyResult verify_function(const MIRFunction& function) {
    MIRVerifyResult r;
    verify_move_paths(r, function);

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
