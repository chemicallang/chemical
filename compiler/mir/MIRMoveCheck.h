// Copyright (c) Chemical Language Foundation 2025.
//
// MIR initialization/move dataflow. This is the first safety pass over the
// move-path trees built by MIRBuilder: it propagates each path's state over the
// CFG and rejects explicit lifetime operations that consume a value which is
// not definitely initialized (double move, drop/destroy of an uninitialized or
// moved path, and reads of a definitely uninitialized path).
//
// The pass is deliberately conservative at joins: states that disagree become
// `Maybe`, and an operation that requires a definite state reports a diagnostic
// instead of guessing. See mir-design.md §4.3.1 and
// lang/docs/safety-implementation-plan.md S3.1b/S3.2.

#pragma once

#include "MIRModule.h"

#include <string>
#include <vector>

namespace mir {

enum class MIRMoveState : uint8_t {
    Uninitialized = 0,
    Initialized,
    Moved,
    Destroyed,
    PartiallyMoved, // some owned children were moved out of the aggregate
    Maybe,          // the paths reaching this point disagree
};

struct MIRMoveDiagnostic {
    uint32_t function_index = MIR_INVALID_ID;
    uint32_t block_index = MIR_INVALID_ID;
    uint32_t inst_index = MIR_INVALID_ID;
    uint32_t path = MIR_INVALID_ID;
    const char* code = nullptr; // stable SAFE-* code
    std::string message;
    MIRSourceLoc loc{};
    bool has_loc = false;
};

struct MIRMoveCheckResult {
    std::vector<MIRMoveDiagnostic> diagnostics;
    bool ok() const { return diagnostics.empty(); }
};

/** Analyze one function's init/move/drop state. */
MIRMoveCheckResult check_function_moves(const MIRModule& module,
                                        const MIRFunction& function,
                                        uint32_t function_index = MIR_INVALID_ID);

/** Analyze every function in the module. */
MIRMoveCheckResult check_module_moves(const MIRModule& module);

} // namespace mir
