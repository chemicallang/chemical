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

#include <string_view>

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

    /**
     * A whole symbol that a target cannot provide, matched by mangled-name
     * *prefix* (one rule covers a whole family: `chx_dom_` is the typed-DOM
     * host surface).
     */
    struct MIRTargetSymbolRule {
        MIRTargetKind target;
        const char* name_prefix;
        const char* reason;   // never empty: the rule only exists for unsupported names
    };

    // The symbol rule table. Exposed for tooling (`--check`, the LSP) that must
    // answer "is this symbol available on <target>?" before emission.
    const MIRTargetSymbolRule* mir_target_symbol_rules();
    std::size_t mir_target_symbol_rule_count();

    // nullptr when `name` is available for `target`, otherwise the same kind of
    // user-facing reason as `mir_unsupported_reason`.
    const char* mir_unsupported_symbol_reason(MIRTargetKind target, std::string_view name);

} // namespace mir
