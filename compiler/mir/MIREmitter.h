// Copyright (c) Chemical Language Foundation 2025.
//
// MIR-to-C emitter. Instruction-per-line output; no GNU `({ ... })` statement
// expressions. Straight-line functions first; control flow/aggregates follow.
// See mir-implementation-plan.md §4 Stage 4 and §6.14.

#pragma once

#include "MIRModule.h"

#include <string>

namespace mir {

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

} // namespace mir
