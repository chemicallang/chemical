// Copyright (c) Chemical Language Foundation 2025.
//
// Declaration printer: the C text of a declaration derived from MIR alone.
// This is action A3 of mir-migration-evaluation.md — the declaration path stops
// walking the AST and printing member by member; it hands MIR a `TypeId` and
// gets C back. The enumeration of *which* declarations a module has (and in
// what order) is still done by the front end; what a declaration looks like is
// MIR's business here.
//
// AST-free on purpose (it reads only the type/field tables), so it lives in MIR
// core and is covered by the standalone MIRTests target. `MIREmitter` owns the
// single C type speller (`c_type_of`/`c_declarator`) this file uses.
//
// The spelling deliberately mirrors what the AST-walking declaration emitter
// wrote byte for byte, because the C output is compared against it: the leading
// newline, the `\t` indent level per member, `__chx_align_struct(N)` on a struct
// header, `__chx_align(N)`/`volatile` prefixes on a member, the variant tag
// field name and the inline anonymous case-payload structs.

#pragma once

#include "MIRModule.h"
#include "MIRDeclList.h"

#include <string>

namespace mir {

// Append forms of the printers below. A caller that is already building a
// declaration buffer should use these: printing a struct with N fields then
// allocates no throwaway `std::string` per member.

/**
 * Full C body of aggregate `id`, exactly as the declaration pass writes it:
 *
 *   "\nstruct Name {\n\tint a;\n};"                      (struct)
 *   "\nunion Name {\n\tint a;\n};"                       (union)
 *   "\nstruct Name {\n\tstruct Base b;\n\tint __chx__vt_621827;\n\tunion {\n\t\t"
 *   "struct {\n\t\t\tint x;\n\t\t} Case;\n\t};\n};"        (variant)
 *
 * A struct's explicit `__chx_align(N)` is written as `__chx_align_struct(N)`
 * (unions/variants keep the natural header — layout only ever raises a struct's
 * alignment today, see `MIRTypeBuilder::fill_aggregate_fields`).
 *
 * Returns an empty string when `id` is not a named struct/union/variant (the
 * caller keeps its own fallback path).
 */
std::string aggregate_body_c(const MIRModule& module, TypeId id);

/** Append form of `aggregate_body_c` (writes nothing for an unnamed type). */
void append_aggregate_body_c(const MIRModule& module, TypeId id, std::string& out);

/**
 * The one-line body of a synthesized inline (anonymous or locally-named) struct
 * or union: `struct Name { int a; int b; };` — the form the driver writes for
 * `MIRModule::inline_structs`. No leading newline/indent (the caller writes
 * those); returns empty when `id` is not a named struct/union.
 *
 * Inherited bases are skipped, matching the member walk this replaces.
 */
std::string inline_aggregate_c(const MIRModule& module, TypeId id);

/** Append form of `inline_aggregate_c`. */
void append_inline_aggregate_c(const MIRModule& module, TypeId id, std::string& out);

/**
 * The declarator text of one member (no indent, includes the trailing `;`):
 * `__chx_align(8) const int* p;`, `volatile int f;`,
 * `__chemical_fat_pointer__* cb;`, or the inline `struct { ... } name;` form for
 * an anonymous member. Used by both printers above; exported so the
 * declaration emitter can spell a lone field from MIR too.
 */
std::string member_declarator_c(const MIRModule& module, const MIRFieldInfo& field);

/**
 * The C text of a global initializer constant expression, derived from MIR
 * alone (action A3 of mir-migration-evaluation.md). This replaces the AST-
 * walking `MIRDeclEmitter::const_value_expr`: a scalar prints its literal, a
 * struct/union prints designated initializers (`.` + field path), an array
 * prints positional elements, a variant prints its tag plus the active case's
 * payload, and a function reference prints its mangled symbol name. Returns
 * `"0"` for a missing/zero expression.
 */
std::string const_expr_c(const MIRModule& module, ConstExprId id);

/**
 * The C text of a whole declaration list, in order (action A3). This is the
 * AST-free printer the C declaration pass now drives: the pass records
 * `MIRDeclItem`s (fwd declarations, aggregate bodies, typedefs, prototypes,
 * global declarations and vtable types/externs) and this renders them. Function
 * bodies, global definitions and vtable definitions are not part of the list —
 * they are emitted inline by the implement pass.
 */
std::string decl_list_c(const MIRModule& module, const MIRDeclList& list);

/**
 * The C declarator of a global/typedef declaration (see MIRGlobalDecl.h):
 * `T name`, `T name[]` for an unsized array, or `void name`. Shared by the
 * declaration list printer and the definition emitter.
 */
std::string global_decl_spelling(const MIRModule& module, const MIRGlobalDecl& d);

/**
 * The C text of a vtable *definition*, derived from a `MIRVtableRecord` alone
 * (action A3): `\nconst <type_name> <name> = {\n(<slot type>) <callee>,\n…\n};`
 * for a container impl (kind 1, explicit function-pointer cast), or
 * `\nconst <type_name> <name> = {\n<callee|0>,\n…\n};` for a primitive impl
 * (kind 2, a null slot is `0`). The callee names are the record's resolved
 * `slot.target`s, so the C emitter no longer resolves them by walking the AST.
 */
std::string vtable_def_c(const MIRModule& module, const MIRVtableRecord& rec);

} // namespace mir
