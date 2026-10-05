// Copyright (c) Chemical Language Foundation 2025.

#include "MIRDump.h"

#include <sstream>

namespace mir {

namespace {

void write_operand(std::ostream& out, const MIROperand& op) {
    switch (op.kind()) {
        case MIROperandKind::Value:
            out << "%v" << op.id;
            break;
        case MIROperandKind::Place:
            out << "p" << op.id;
            break;
        case MIROperandKind::Symbol:
            out << "s" << op.id;
            break;
        case MIROperandKind::Constant:
            out << "c" << op.id;
            break;
        case MIROperandKind::Block:
            out << "bb" << op.id;
            break;
        case MIROperandKind::Type:
            out << "ty" << op.id;
            break;
    }
}

void write_flags(std::ostream& out, uint16_t flags) {
    if (flags & OF_Pure) out << "|pure";
    if (flags & OF_Read) out << "|read";
    if (flags & OF_Write) out << "|write";
    if (flags & OF_Call) out << "|call";
    if (flags & OF_MayThrow) out << "|maythrow";
    if (flags & OF_MayTrap) out << "|maytrap";
    if (flags & OF_Volatile) out << "|volatile";
    if (flags & OF_Atomic) out << "|atomic";
    if (flags & OF_Lifetime) out << "|lifetime";
}

void dump_instruction(const MIRFunction& fn, const MIRInstruction& inst,
                      std::ostream& out, const char* indent) {
    out << indent;
    if (inst.result_or_place != MIR_NULL) {
        const MIROpcode op = inst.opcode();
        const bool producesValue =
            op == MIROpcode::ConstInt || op == MIROpcode::ConstFloat ||
            op == MIROpcode::ConstDouble || op == MIROpcode::ConstBool ||
            op == MIROpcode::ConstNull || op == MIROpcode::ConstString ||
            op == MIROpcode::Load || op == MIROpcode::AddressOf ||
            op == MIROpcode::Gep || op == MIROpcode::GlobalAddr ||
            op == MIROpcode::FunctionAddr || op == MIROpcode::Param ||
            op == MIROpcode::Unary || op == MIROpcode::Binary ||
            op == MIROpcode::Compare || op == MIROpcode::Cast ||
            op == MIROpcode::Select || op == MIROpcode::SizeOf ||
            op == MIROpcode::AlignOf || op == MIROpcode::OffsetOf;
        if (producesValue) out << "%v" << inst.result_or_place << " = ";
        else out << "p" << inst.result_or_place << " = ";
    }
    out << opcode_name(inst.opcode());
    for (uint16_t i = 0; i < inst.operand_count; ++i) {
        const uint32_t idx = inst.operand_offset + i;
        if (idx >= fn.operands.size()) {
            out << " <bad-operand>";
            break;
        }
        out << (i == 0 ? " " : ", ");
        write_operand(out, fn.operands[idx]);
    }
    out << "  ;";
    write_flags(out, inst.flags());
    if (inst.source_index != 0xFFFF) out << " src=" << inst.source_index;
    out << "\n";
}

} // namespace

void dump_function(const MIRFunction& function, const MIRModule& module,
                   std::ostream& out) {
    (void) module; // reserved for symbol/type name resolution in the dump
    out << "func s" << function.symbol << " : ty" << function.function_type << " {\n";
    for (uint32_t b = 0; b < function.blocks.size(); ++b) {
        const MIRBlock& block = function.blocks[b];
        out << "bb" << block.id << ":";
        if (block.arg_count) {
            out << "(";
            for (uint16_t a = 0; a < block.arg_count; ++a) {
                if (a) out << ", ";
                out << "ty" << function.block_arg_types[block.arg_type_offset + a];
            }
            out << ")";
        }
        out << "\n";

        for (uint32_t i = 0; i < block.inst_count; ++i) {
            const uint32_t idx = block.inst_start + i;
            if (idx >= function.instructions.size()) break;
            dump_instruction(function, function.instructions[idx], out, "  ");
        }
        const uint32_t term_idx = block.inst_start + block.inst_count;
        if (term_idx < function.instructions.size()) {
            dump_instruction(function, function.instructions[term_idx], out, "  ");
        }
    }
    out << "}\n";
}

std::string dump_function_str(const MIRFunction& function, const MIRModule& module) {
    std::ostringstream ss;
    dump_function(function, module, ss);
    return ss.str();
}

void dump_module(const MIRModule& module, std::ostream& out) {
    out << "module {\n";
    out << "  types: " << module.types.size()
        << ", constants: " << module.constants.size()
        << ", symbols: " << module.symbols.size()
        << ", functions: " << module.functions.size() << "\n";
    for (const MIRFunction& fn : module.functions) {
        dump_function(fn, module, out);
    }
    out << "}\n";
}

} // namespace mir
