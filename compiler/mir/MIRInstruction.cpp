// Copyright (c) Chemical Language Foundation 2025.
//
// Static opcode tables. Flags, names and portability are fixed per opcode and
// are never stored per instruction (mir-implementation-plan.md §6.10).

#include "MIRInstruction.h"

namespace mir {

uint16_t opcode_flags(MIROpcode op) {
    switch (op) {
        case MIROpcode::Nop:
            return 0;

        case MIROpcode::ConstInt:
        case MIROpcode::ConstFloat:
        case MIROpcode::ConstDouble:
        case MIROpcode::ConstBool:
        case MIROpcode::ConstNull:
        case MIROpcode::ConstString:
            return OF_Pure;

        case MIROpcode::Alloca:
            return OF_Write;
        case MIROpcode::GlobalAddr:
        case MIROpcode::FunctionAddr:
            return OF_Pure;
        case MIROpcode::Param:
            return OF_Pure;

        case MIROpcode::Load:
            return OF_Read | OF_MayTrap;
        case MIROpcode::Store:
            return OF_Write | OF_MayTrap;
        case MIROpcode::AddressOf:
            return OF_Pure;
        case MIROpcode::Gep:
            return OF_Pure | OF_MayTrap;
        case MIROpcode::FieldAddr:
        case MIROpcode::IndexAddr:
            return OF_Pure | OF_MayTrap;

        case MIROpcode::Init:
            return OF_Lifetime | OF_Call | OF_Write | OF_MayThrow;
        case MIROpcode::CopyInit:
            return OF_Lifetime | OF_Write;
        case MIROpcode::MoveInit:
            return OF_Lifetime | OF_Write;
        case MIROpcode::Assign:
            return OF_Lifetime | OF_Write;
        case MIROpcode::Copy:
            return OF_Lifetime | OF_Read | OF_Write;
        case MIROpcode::Move:
            return OF_Lifetime | OF_Read | OF_Write;
        case MIROpcode::Drop:
        case MIROpcode::Destroy:
            return OF_Lifetime | OF_Call | OF_Write | OF_MayThrow;
        case MIROpcode::SetDrop:
            return OF_Lifetime | OF_Write;
        case MIROpcode::MemCpy:
            return OF_Read | OF_Write | OF_MayTrap;
        case MIROpcode::MemSet:
            return OF_Write | OF_MayTrap;

        case MIROpcode::Unary:
            return OF_Pure | OF_MayTrap;
        case MIROpcode::Binary:
            return OF_Pure | OF_MayTrap;
        case MIROpcode::Compare:
            return OF_Pure;
        case MIROpcode::Cast:
            return OF_Pure;
        case MIROpcode::Select:
            return OF_Pure;
        case MIROpcode::SizeOf:
        case MIROpcode::AlignOf:
        case MIROpcode::OffsetOf:
            return OF_Pure;

        case MIROpcode::Call:
        case MIROpcode::CallIndirect:
            // Safe default for a call to code whose contract is not known yet.
            return OF_Call | OF_Read | OF_Write | OF_MayThrow;

        case MIROpcode::Br:
            return OF_Terminator;
        case MIROpcode::CondBr:
            return OF_Terminator;
        case MIROpcode::Switch:
            return OF_Terminator;
        case MIROpcode::Return:
            return OF_Terminator;
        case MIROpcode::Unreachable:
            return OF_Terminator;
        case MIROpcode::Throw:
            return OF_Terminator | OF_MayThrow;

        case MIROpcode::Count:
            return 0;
    }
    return 0;
}

const char* opcode_name(MIROpcode op) {
    switch (op) {
        case MIROpcode::Nop: return "nop";
        case MIROpcode::ConstInt: return "const.int";
        case MIROpcode::ConstFloat: return "const.float";
        case MIROpcode::ConstDouble: return "const.double";
        case MIROpcode::ConstBool: return "const.bool";
        case MIROpcode::ConstNull: return "const.null";
        case MIROpcode::ConstString: return "const.string";
        case MIROpcode::Alloca: return "alloca";
        case MIROpcode::GlobalAddr: return "global_addr";
        case MIROpcode::FunctionAddr: return "function_addr";
        case MIROpcode::Param: return "param";
        case MIROpcode::Load: return "load";
        case MIROpcode::Store: return "store";
        case MIROpcode::AddressOf: return "address_of";
        case MIROpcode::Gep: return "gep";
        case MIROpcode::FieldAddr: return "field_addr";
        case MIROpcode::IndexAddr: return "index_addr";
        case MIROpcode::Init: return "init";
        case MIROpcode::CopyInit: return "copy_init";
        case MIROpcode::MoveInit: return "move_init";
        case MIROpcode::Assign: return "assign";
        case MIROpcode::Copy: return "copy";
        case MIROpcode::Move: return "move";
        case MIROpcode::Drop: return "drop";
        case MIROpcode::Destroy: return "destroy";
        case MIROpcode::SetDrop: return "set_drop";
        case MIROpcode::MemCpy: return "memcpy";
        case MIROpcode::MemSet: return "memset";
        case MIROpcode::Unary: return "unary";
        case MIROpcode::Binary: return "binary";
        case MIROpcode::Compare: return "compare";
        case MIROpcode::Cast: return "cast";
        case MIROpcode::Select: return "select";
        case MIROpcode::SizeOf: return "sizeof";
        case MIROpcode::AlignOf: return "alignof";
        case MIROpcode::OffsetOf: return "offsetof";
        case MIROpcode::Call: return "call";
        case MIROpcode::CallIndirect: return "call_indirect";
        case MIROpcode::Br: return "br";
        case MIROpcode::CondBr: return "cond_br";
        case MIROpcode::Switch: return "switch";
        case MIROpcode::Return: return "return";
        case MIROpcode::Unreachable: return "unreachable";
        case MIROpcode::Throw: return "throw";
        case MIROpcode::Count: return "<count>";
    }
    return "<unknown>";
}

MIRPortability opcode_portability(MIROpcode op) {
    switch (op) {
        case MIROpcode::AddressOf:
        case MIROpcode::Gep:
        case MIROpcode::FieldAddr:
        case MIROpcode::IndexAddr:
        case MIROpcode::Alloca:
        case MIROpcode::Load:
        case MIROpcode::Store:
        case MIROpcode::MemCpy:
        case MIROpcode::MemSet:
        case MIROpcode::GlobalAddr:
        case MIROpcode::FunctionAddr:
        case MIROpcode::Param:
        case MIROpcode::Call:
        case MIROpcode::CallIndirect:
            return MIRPortability::NativeOnly;
        case MIROpcode::SizeOf:
        case MIROpcode::AlignOf:
        case MIROpcode::OffsetOf:
            return MIRPortability::TargetLayoutDependent;
        default:
            return MIRPortability::Portable;
    }
}

} // namespace mir
