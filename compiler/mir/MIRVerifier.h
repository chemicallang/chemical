// Copyright (c) Chemical Language Foundation 2025.
//
// Structural MIR verifier. These are the cheap checks that must hold for the
// emitted MIR to be well-formed. Full dominance/lifetime verification is a
// later, opt-in mode (--verify-mir). See mir-design.md §12 and
// mir-implementation-plan.md §12.

#pragma once

#include "MIRModule.h"

#include <string>
#include <vector>

namespace mir {

struct MIRVerifyDiagnostic {
    uint32_t function_index = MIR_INVALID_ID;
    uint32_t block_index = MIR_INVALID_ID;
    uint32_t inst_index = MIR_INVALID_ID;
    std::string message;
};

struct MIRVerifyResult {
    std::vector<MIRVerifyDiagnostic> diagnostics;

    bool ok() const { return diagnostics.empty(); }
};

/** Structural checks over a single function. */
MIRVerifyResult verify_function(const MIRFunction& function);

/** Structural checks over a whole module (calls verify_function per function). */
MIRVerifyResult verify_module(const MIRModule& module);

} // namespace mir
