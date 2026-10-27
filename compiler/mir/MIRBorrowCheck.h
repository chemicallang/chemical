// Copyright (c) Chemical Language Foundation 2025.
//
// MIR borrow checking, first slice (S4.4a). This is a report-only loan
// analysis: it tracks references created by `Borrow`/`BorrowMut`, follows them
// through reference locals, and reports overlapping mutable loans and
// moves/drops of a borrowed place.
//
// The live range is derived from a linear walk over the function's instructions
// (the last use of a loan or a value copied from it). That is precise for
// straight-line code and conservative-enough for a first report; branch/loop
// precise NLL and interprocedural escape come next (S4.4b/S4.5).
//
// See lang/docs/safety-implementation-plan.md §7.

#pragma once

#include "MIRModule.h"

#include <string>
#include <vector>

namespace mir {

struct MIRBorrowDiagnostic {
    uint32_t function_index = MIR_INVALID_ID;
    uint32_t block_index = MIR_INVALID_ID;
    uint32_t inst_index = MIR_INVALID_ID;
    const char* code = nullptr;
    std::string message;
    MIRSourceLoc loc{};
    bool has_loc = false;
};

struct MIRBorrowCheckResult {
    std::vector<MIRBorrowDiagnostic> diagnostics;
    bool ok() const { return diagnostics.empty(); }
};

/** Analyze one function's loans. */
MIRBorrowCheckResult check_function_borrows(const MIRModule& module,
                                            const MIRFunction& function,
                                            uint32_t function_index = MIR_INVALID_ID);

/** Analyze every function in the module. */
MIRBorrowCheckResult check_module_borrows(const MIRModule& module);

} // namespace mir
