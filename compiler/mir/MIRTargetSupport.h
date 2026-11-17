#pragma once

// Shared per-target support table for MIR.
//
// A backend must not decide on its own whether MIR is expressible for its target:
// the same answer is needed by (a) the emitter, which refuses to write code, (b)
// `--check` runs that ask "does this compile to <target>?" without writing
// anything, and (c) the LSP, which must report the construct *before* emission.
// Asking one table keeps those three from drifting apart.
//
// This is deliberately data, not a virtual method per backend: the reason strings
// are the user-facing diagnostics, so they live in one place and are testable
// without standing up an emitter.

#include "compiler/mir/MIRInstruction.h"

namespace mir {

    enum class MIRTargetKind : uint8_t { C, LLVM, JS };

    struct MIRTargetRule {
        MIRTargetKind target;
        MIROpcode opcode;
        const char* reason;   // nullptr / empty = supported
    };

    // The rule table. Exposed for tooling that wants to enumerate a target's gaps.
    const MIRTargetRule* mir_target_rules();
    std::size_t mir_target_rule_count();

    // nullptr when `opcode` is supported for `target`, otherwise the reason the
    // construct cannot be emitted (a complete user-facing sentence fragment).
    const char* mir_unsupported_reason(MIRTargetKind target, MIROpcode opcode);

} // namespace mir
