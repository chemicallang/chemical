// Copyright (c) Chemical Language Foundation 2025.
//
// Stable textual MIR dump. Debug/tooling only; the formatter is a compact
// switch, never stored per instruction. See mir-design.md §15.

#pragma once

#include "MIRModule.h"

#include <ostream>

namespace mir {

/** Write the whole module (tables + functions) as text. */
void dump_module(const MIRModule& module, std::ostream& out);

/** Write a single function as text. */
void dump_function(const MIRFunction& function, const MIRModule& module,
                   std::ostream& out);

/** Return the dump as a std::string (convenience for tests/tools). */
std::string dump_function_str(const MIRFunction& function, const MIRModule& module);

} // namespace mir
