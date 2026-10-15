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

#include <string>

namespace mir {

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

/**
 * The one-line body of a synthesized inline (anonymous or locally-named) struct
 * or union: `struct Name { int a; int b; };` — the form the driver writes for
 * `MIRModule::inline_structs`. No leading newline/indent (the caller writes
 * those); returns empty when `id` is not a named struct/union.
 *
 * Inherited bases are skipped, matching the member walk this replaces.
 */
std::string inline_aggregate_c(const MIRModule& module, TypeId id);

/**
 * The declarator text of one member (no indent, includes the trailing `;`):
 * `__chx_align(8) const int* p;`, `volatile int f;`,
 * `__chemical_fat_pointer__* cb;`, or the inline `struct { ... } name;` form for
 * an anonymous member. Used by both printers above; exported so the
 * declaration emitter can spell a lone field from MIR too.
 */
std::string member_declarator_c(const MIRModule& module, const MIRFieldInfo& field);

} // namespace mir
