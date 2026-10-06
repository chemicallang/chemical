// Copyright (c) Chemical Language Foundation 2025.
//
// Core MIR (mid-level intermediate representation) identifier and enum types.
// See lang/docs/mir-design.md and lang/docs/mir-implementation-plan.md.
//
// MIR is the compiler's C (and later LLVM) lowering path. It is built from the
// resolved, type-checked AST and is the only path (no feature flag, no legacy
// fallback). This header is deliberately dependency-free so it can be included
// by the C backend, the LLVM backend, and the interpreter.
//
// All MIR enum types are prefixed with `MIR` so that `using namespace mir;` in a
// consumer translation unit cannot collide with non-MIR enums of the same name.

#pragma once

#include <cstdint>
#include <cstddef>

namespace mir {

/** module-level type identity (index into MIRTypeTable) */
using TypeId = uint32_t;
/** function-local SSA value identity (index into MIRFunction::values) */
using ValueId = uint32_t;
/** function-local addressable-storage identity (index into MIRFunction::places) */
using PlaceId = uint32_t;
/** function-local basic-block identity (index into MIRFunction::blocks) */
using BlockId = uint32_t;
/** function-local instruction identity (index into the instruction stream) */
using InstId = uint32_t;
/** module-level symbol identity (index into MIRSymbolTable) */
using SymbolId = uint32_t;
/** module-level constant identity (index into MIRConstantPool) */
using ConstantId = uint32_t;

/** sentinel for "no id" */
constexpr uint32_t MIR_INVALID_ID = 0xFFFFFFFFu;

/** a null value/place/block operand */
constexpr uint32_t MIR_NULL = 0u;

/**
 * What an operand refers to. Stored (4 bits) alongside a TypeId (28 bits) in
 * MIROperand::kind_and_type.
 */
enum class MIROperandKind : uint8_t {
    Value = 0,     // SSA value
    Place = 1,     // addressable storage
    Symbol = 2,    // function / global / intrinsic symbol
    Constant = 3,  // interned constant
    Block = 4,     // block id (terminator edges)
    Type = 5,      // type operand (sizeof, cast destination, alloca)
};

/**
 * Storage class of a place. Recorded in the place table and used by cleanup
 * and by the C emitter when choosing a C declaration.
 */
enum class MIRStorageClass : uint8_t {
    Local = 0,      // a source-level local
    Temporary = 1,  // a compiler temporary with a precise lifetime region
    Parameter = 2,  // a by-value parameter spilled to memory
    Global = 3,     // a module-level variable
    FrameField = 4, // a coroutine frame field (`__chx__af->__chx_slot_N`)
    FramePtr = 5,   // the coroutine frame pointer local (`__chx__af`)
};

/**
 * Ownership / initialization state of a place or move-path node. Checked by the
 * verifier; the builder may use it for conservative local checks.
 */
enum class MIRInitState : uint8_t {
    Uninitialized = 0,
    Initialized = 1,
    Moved = 2,
    PartiallyMoved = 3,
    Destroyed = 4,
};

/** bit flags stored in MIRValueDef::flags */
enum MIRValueFlags : uint8_t {
    VF_NONE = 0x00,
    VF_CONSTANT = 0x01,    // value came from a constant
    VF_PURE = 0x02,        // value is a pure scalar result
    VF_ADDRESSABLE = 0x04, // value is (or derives from) a place address
};

/** portability classification of an operation, for future non-native targets */
enum class MIRPortability : uint8_t {
    Portable = 0,             // representable on all targets
    NativeOnly = 1,           // requires native pointer/ABI support
    TargetLayoutDependent = 2 // depends on target sizeof/offsetof/layout
};

/** alignment used for MIR arena allocations by default */
constexpr size_t MIR_DEFAULT_ALIGN = alignof(std::max_align_t);

} // namespace mir
