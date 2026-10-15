// Copyright (c) Chemical Language Foundation 2025.
//
// D5 measurement tooling: operation/phase counters for MIR lowering.
//
// There is no `perf` on the project's Windows/most CI machines, so targeting
// action D ("fix the lowering hot path") needs counters rather than a sampling
// profiler. Build with `-DMIR_OPS_COUNTERS=ON` (see CMakeLists) and run with
// `MIR_OPS=1` to print a one-line tally of lowering work at process exit.
//
// Zero cost when the option is off: every site is `MIR_OP(...)`, which compiles
// to nothing, and neither the counter object nor the printer exists.

#pragma once

#ifdef MIR_OPS_COUNTERS

namespace mir {

struct MIRHotOps {
    unsigned long long functions = 0;     // lower_function calls
    unsigned long long instrs = 0;        // MIRBuilder::emit_impl calls
    unsigned long long lower_ns = 0;      // total time inside lower_function
    unsigned long long place_lookup = 0;  // var_places_ (ASTNode* -> PlaceId) finds
    unsigned long long place_bind = 0;    // var_places_ inserts
    unsigned long long name_lookup = 0;   // name_places_ (string -> PlaceId) finds
    unsigned long long name_bind = 0;     // name_places_ inserts
    unsigned long long intern = 0;        // intern_function calls
    unsigned long long type_map = 0;      // MIRTypeBuilder::map calls (memo_)
};

extern MIRHotOps g_hot_ops;

} // namespace mir

#define MIR_OP(field) (++::mir::g_hot_ops.field)

#else

#define MIR_OP(field) ((void) 0)

#endif
