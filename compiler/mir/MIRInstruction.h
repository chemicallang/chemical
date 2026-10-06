// Copyright (c) Chemical Language Foundation 2025.
//
// The MIR instruction encoding. Fixed 16-byte header + operands stored in a
// contiguous per-function operand array. Effects, portability and compaction
// safety are properties of the OPCODE (static tables), never stored per
// instruction. See mir-implementation-plan.md §2.3 and §6.10.

#pragma once

#include "MIRTypes.h"

namespace mir {

/** Every MIR opcode. A uint16_t; explicit values for stable dumps. */
enum class MIROpcode : uint16_t {
    Nop = 0,

    // ── constants ───────────────────────────────────────────────────────────
    ConstInt = 1,     // result: value;    operand: Type
    ConstFloat = 2,   // result: value;    operand: Type
    ConstDouble = 3,  // result: value;    operand: Type
    ConstBool = 4,    // result: value;    operand: Type
    ConstNull = 5,    // result: value;    operand: Type
    ConstString = 6,  // result: value;    operand: Constant, Type

    // ── storage ─────────────────────────────────────────────────────────────
    Alloca = 7,       // result: place;    operand: Type
    GlobalAddr = 8,   // result: value;    operand: Symbol, Type
    FunctionAddr = 9, // result: value;    operand: Symbol, Type
    Param = 10,       // result: value/place; operand: Type

    Load = 11,        // result: value;    operands: Place, Type
    Store = 12,       // result: none;     operands: Place, Value
    AddressOf = 13,   // result: value;    operands: Place, Type(ptr)
    Gep = 14,         // result: value;    operands: Value, indices..., Type
    FieldAddr = 15,   // result: place;    operands: Place, Constant(field), Type
    IndexAddr = 16,   // result: place;    operands: Place, Value(index), Type

    // ── lifetime ────────────────────────────────────────────────────────────
    Init = 17,       // result: none;   operands: Place(dest), Symbol(ctor), args...
    CopyInit = 18,   // result: none;   operands: Place(dest), Place/Value(src), Type
    MoveInit = 19,   // result: none;   operands: Place(dest), Place(src), Type
    Assign = 20,     // result: none;   operands: Place(dest), Value(src), Type
    Copy = 21,       // result: value/place; operands: src, Type
    Move = 22,       // result: value/place; operands: src, Type
    Drop = 23,       // result: none;   operands: Place, Symbol(dtor)[, Value(flag)]
    Destroy = 24,    // result: none;   operands: Place, Symbol(dtor)
    SetDrop = 25,    // result: none;   operands: Place(flag), Constant(bool)
    MemCpy = 26,     // result: none;   operands: Place/V dest, Place/V src, Value(size)
    MemSet = 27,     // result: none;   operands: Place/V dest, Value(byte), Value(size)

    // ── arithmetic / conversion ─────────────────────────────────────────────
    Unary = 28,      // result: value;  operands: Value, Constant(op), Type
    Binary = 29,     // result: value;  operands: Value, Value, Constant(op), Type
    Compare = 30,    // result: value;  operands: Value, Value, Constant(pred), Type
    Cast = 31,       // result: value;  operands: Value, Type
    Select = 32,     // result: value;  operands: Value(cond), Value, Value, Type
    SizeOf = 33,     // result: value;  operand: Type, Type(result)
    AlignOf = 34,    // result: value;  operand: Type, Type(result)
    OffsetOf = 35,   // result: value;  operand: Type, Constant(field), Type(result)

    // ── calls ───────────────────────────────────────────────────────────────
    Call = 36,         // result: value/place; operands: Symbol(callee), args...
    CallIndirect = 37, // result: value/place; operands: Value(fn), args...

    // ── terminators ─────────────────────────────────────────────────────────
    Br = 38,          // operands: Block[, args...]
    CondBr = 39,      // operands: Value(cond), Block(then)[, args], Block(else)[, args]
    Switch = 40,      // operands: Value(scrutinee), Block(default), cases...
    Return = 41,      // operands: [Value]
    Unreachable = 42, // no operands
    Throw = 43,       // operands: Value(exception), Block(unwind)

    // ── async / coroutine ───────────────────────────────────────────────────
    // Poll the child future at one `await` site and either take its result or
    // store the resume state, spill the live locals and return `Pending`.
    // operands: Place(frame), Place(child), Place(result), Type(poll_type),
    //           Constant(resume_state; 0 = resume pass), Place(ret),
    //           Type(fn_poll_type), Constant(spill_count),
    //           [Place(local), Constant(field_name)]*spill_count
    AsyncAwait = 44,
    // Complete the poll with `Ready(frame->__result)`.
    // operands: Place(frame), Place(ret), Type(fn_poll_type)
    AsyncFinish = 45,
    // Ramp epilogue: set the final state and return the `FutureHandle<T>`.
    // operands: Place(frame), Place(sret), Constant(state), Constant(vtbl_name),
    //           Type(handle_type)
    AsyncRampFinish = 46,
    // Allocate the coroutine frame. result: value(frame*); operand: Type(frame)
    AsyncFrameAlloc = 47,

    Count = 48,
};

/** fixed opcode effects/attributes (see mir-design.md §4.6) */
enum MIROpcodeFlags : uint16_t {
    OF_Pure = 0x0001,       // no observable state or control effect
    OF_Volatile = 0x0002,   // volatile access (barrier)
    OF_Atomic = 0x0004,     // atomic operation
    OF_MayThrow = 0x0008,   // may transfer control to an unwind target
    OF_MayTrap = 0x0010,    // may trap (div by zero, bounds)
    OF_Read = 0x0020,       // reads memory
    OF_Write = 0x0040,      // writes memory
    OF_Call = 0x0080,       // calls unknown code (barrier)
    OF_Terminator = 0x0100, // transfers control
    OF_Lifetime = 0x0200,   // init/move/drop/destroy/set-drop family
};

uint16_t opcode_flags(MIROpcode op);
const char* opcode_name(MIROpcode op);
MIRPortability opcode_portability(MIROpcode op);

/**
 * Arithmetic/comparison operators for Unary/Binary/Compare. Encoded (as an
 * integer) in a Constant operand, so the emitter never needs the AST Operation
 * enum. Kept in sync with the lowerer.
 */
enum class MIRBinaryOp : uint8_t {
    Add, Sub, Mul, Div, Rem, Shl, Shr,
    BitAnd, BitOr, BitXor,
    Lt, Le, Gt, Ge, Eq, Ne,
};

enum class MIRUnaryOp : uint8_t {
    Neg, Plus, Not, BitNot,
};

inline bool is_terminator(MIROpcode op) {
    return (opcode_flags(op) & OF_Terminator) != 0;
}
inline bool opcode_is_pure(MIROpcode op) {
    return (opcode_flags(op) & OF_Pure) != 0;
}

/**
 * A single operand. `kind_and_type` packs the operand kind (high 4 bits) with a
 * TypeId (low 28 bits).
 */
struct MIROperand {
    uint32_t id = MIR_NULL;
    uint32_t kind_and_type = 0;

    MIROperandKind kind() const {
        return static_cast<MIROperandKind>(kind_and_type >> 28);
    }
    TypeId type() const { return kind_and_type & 0x0FFFFFFFu; }

    static MIROperand make(MIROperandKind k, TypeId t, uint32_t id) {
        MIROperand o;
        o.id = id;
        o.kind_and_type = (static_cast<uint32_t>(k) << 28) | (t & 0x0FFFFFFFu);
        return o;
    }
    static MIROperand value(ValueId v, TypeId t) { return make(MIROperandKind::Value, t, v); }
    static MIROperand place(PlaceId p, TypeId t) { return make(MIROperandKind::Place, t, p); }
    static MIROperand symbol(SymbolId s, TypeId t) { return make(MIROperandKind::Symbol, t, s); }
    static MIROperand constant(ConstantId c, TypeId t) { return make(MIROperandKind::Constant, t, c); }
    static MIROperand block(BlockId b) { return make(MIROperandKind::Block, 0, b); }
    static MIROperand type(TypeId t) { return make(MIROperandKind::Type, t, t); }
};

/**
 * The fixed 16-byte MIR instruction header. Operands live in the contiguous
 * function operand array at [operand_offset, operand_offset + operand_count).
 */
struct MIRInstruction {
    uint32_t opcode_and_flags = 0;
    uint32_t result_or_place = MIR_NULL;
    uint32_t operand_offset = 0;
    uint16_t operand_count = 0;
    uint16_t source_index = 0xFFFF; // index into MIRFunction::sources, 0xFFFF = none

    MIROpcode opcode() const { return static_cast<MIROpcode>(opcode_and_flags >> 16); }
    uint16_t flags() const { return static_cast<uint16_t>(opcode_and_flags & 0xFFFF); }

    static MIRInstruction make(MIROpcode op) {
        MIRInstruction i;
        i.opcode_and_flags = (static_cast<uint32_t>(op) << 16) | opcode_flags(op);
        return i;
    }
};

} // namespace mir
