// Copyright (c) Chemical Language Foundation 2025.
//
// MIR-to-C emitter. Instruction-per-line output; no GNU `({ ... })` statement
// expressions. Straight-line functions first; control flow/aggregates follow.
// See mir-implementation-plan.md §4 Stage 4 and §6.14.

#pragma once

#include "MIRModule.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace mir {

/** Append an unsigned integer without building a temporary `std::string`. */
inline void append_uint(std::string& out, uint64_t v) {
    char buf[20];
    int n = 0;
    do {
        buf[n++] = static_cast<char>('0' + (v % 10));
        v /= 10;
    } while (v);
    while (n) out.push_back(buf[--n]);
}

/**
 * Emits one C function from MIR into `out`.
 *
 * On an unsupported construct, returns false and sets `error`; `out` may be
 * partially written and must be discarded by the caller (transactional
 * emission). Never emits a compound statement expression.
 */
bool emit_function_c(const MIRFunction& function, const MIRModule& module,
                     std::string& out, std::string& error);

/** C spelling of a MIR type id (primitives/pointers; aggregates use handle). */
std::string c_type_of(const MIRModule& module, TypeId type);

/**
 * Append forms of the spellers above. A declaration/emission path that is
 * printing into a buffer should always use these: the string-returning
 * versions exist for callers that need to store or compare a spelling.
 */
void append_c_type_of(const MIRModule& module, TypeId type, std::string& out);
void append_c_declarator(const MIRModule& module, TypeId type, std::string_view name,
                         std::string& out);
/**
 * `append_c_declarator` for a `name` that lives in `decl_scratch(0)`: starts at
 * scratch depth 1 so the nested declarators of `type` (which acquire slot 0 at
 * depth 0) cannot clobber the buffer `name` points into.
 */
void append_c_declarator_scratched(const MIRModule& module, TypeId type, std::string_view name,
                                   std::string& out);
void append_c_decl_type(const MIRModule& module, TypeId type, std::string_view name,
                        std::string& out);
void append_c_function_proto(const MIRModule& module, TypeId function_type,
                             std::string_view name, std::string& out);

/**
 * C declarator for an entity named `name` of MIR type `type`. Handles
 * function-pointer and pointer-to-array declarators and the sret convention
 * for function types returning an aggregate. Shared with the declaration
 * emitter so prototypes and definitions spell types identically.
 */
std::string c_decl_type(const MIRModule& module, TypeId type, std::string_view name);


/**
 * Full C declarator for `name` of type `type`, handling arrays and
 * function-pointer/pointer-to-array declarators (e.g. `long name[3]`,
 * `int (*name)(int)`). Use this for variable/field declarations; `c_decl_type`
 * is the function-pointer flavor used inside prototypes.
 */
std::string c_declarator(const MIRModule& module, TypeId type, std::string_view name);

/**
 * C prototype spelling for a MIR function type (`ret name(params)`), as opposed
 * to `c_decl_type` which spells a function-pointer declarator (`ret (*name)(...)`).
 * Aggregate returns use the sret convention (`void name(struct T*, ...)`).
 */
std::string c_function_proto(const MIRModule& module, TypeId function_type, std::string_view name);

} // namespace mir
