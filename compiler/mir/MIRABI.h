// Copyright (c) Chemical Language Foundation 2025.
//
// Backend-neutral ABI classification. Every emitter (C, LLVM, JVM, JS) and the
// lowerer must agree on these *exactly*, or a definition and its callers stop
// matching; they are therefore defined once here rather than re-derived per
// backend (evaluation §1.4 and action B). Before this header the same rules were
// written out nine times across three translation units — `MIRLowerer`
// (`needs_aggregate_path`, `needs_sret_return`, `is_struct_like`),
// `MIRDeclEmitter` (`needs_aggregate`, `needs_sret`) and `CodegenMir`
// (`mir_is_aggregate`, `mir_needs_sret`, `mir_is_ptr_like`, `mir_is_void`) — and
// had to be kept in sync by hand.
//
// The rules are the legacy prototype ABI:
//
//   * a struct-like or array **return** goes through a hidden sret pointer, and
//     typedef structs are included — the `dyn<I>` fat pointer (`TF_TYPEDEF`) is
//     returned through sret like any other aggregate;
//   * a struct-like or array **parameter** is passed by pointer, but a typedef
//     struct is passed *by value* (it is two words: vtable + object);
//   * pointers and references share a C spelling (`T*`, differing only in the
//     const on the pointee), so `is_pointer_like` covers both.
//
// These are pure functions of the interned type table — no AST, no target
// state — so a non-native backend can use them unchanged.

#pragma once

#include "MIRModule.h"

namespace mir {
namespace abi {

/** True when `t` is a valid, in-range type id. */
inline bool is_valid(const MIRModule& m, TypeId t) {
    return t != MIR_INVALID_ID && t < m.types.size();
}

/** Void (or an unknown type id, which behaves as void everywhere). */
inline bool is_void(const MIRModule& m, TypeId t) {
    return !is_valid(m, t) || m.types.get(t).kind == MIRTypeKind::Void;
}

/**
 * A struct-like or array value: the set that travels through memory rather than
 * in registers. A typedef struct is *not* included here for parameter purposes
 * — see `is_aggregate` versus `needs_sret`, which differ exactly on that.
 */
inline bool is_struct_like(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant;
}

/**
 * Whether a value of `t` is returned through a hidden sret pointer. Every
 * struct-like type *including* typedef structs, plus arrays.
 */
inline bool needs_sret(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant || k == MIRTypeKind::Array;
}

/**
 * Whether a parameter of type `t` is passed by pointer. Struct-like and arrays,
 * but **not** a typedef struct (the dynamic fat pointer is passed by value).
 */
inline bool is_aggregate(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    if (m.types.get(t).flags & TF_TYPEDEF) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Struct || k == MIRTypeKind::Union ||
           k == MIRTypeKind::Variant || k == MIRTypeKind::Array;
}

/** Pointer or reference. */
inline bool is_pointer_like(const MIRModule& m, TypeId t) {
    if (!is_valid(m, t)) return false;
    const MIRTypeKind k = m.types.get(t).kind;
    return k == MIRTypeKind::Pointer || k == MIRTypeKind::Reference;
}

} // namespace abi
} // namespace mir
