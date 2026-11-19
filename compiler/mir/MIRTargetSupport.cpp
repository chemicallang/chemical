#include "compiler/mir/MIRTargetSupport.h"

namespace mir {

    namespace {
        // Only *unsupported* constructs appear here; everything absent is supported.
        // The strings are the diagnostics, so they are written as the emitter says
        // them: "<thing> is not supported by the <target> backend".
        const MIRTargetRule kRules[] = {
            // ── JavaScript ────────────────────────────────────────────────────
            { MIRTargetKind::JS, MIROpcode::DynamicCall,
              "interface dispatch (dynamic_call) is not supported by the JS backend yet" },
            { MIRTargetKind::JS, MIROpcode::Throw,
              "exceptions are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::InlineAsm,
              "inline assembly is not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AtomicLoad,
              "atomics are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AtomicStore,
              "atomics are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AtomicCmpXchg,
              "atomics are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AtomicRMW,
              "atomics are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AtomicFence,
              "atomics are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AsyncAwait,
              "coroutines (async) are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AsyncFinish,
              "coroutines (async) are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AsyncRampFinish,
              "coroutines (async) are not supported by the JS backend" },
            { MIRTargetKind::JS, MIROpcode::AsyncFrameAlloc,
              "coroutines (async) are not supported by the JS backend" },
        };
        // Symbols a target simply does not have. Same contract as the opcode
        // table: only *unsupported* names appear, and the string is the
        // diagnostic. Prefixes keep a whole host API to one rule.
        const MIRTargetSymbolRule kSymbolRules[] = {
            // The typed DOM (`lang/libs/dom`): its `chx_dom_*` externs are
            // implemented by the JS runtime and by nothing else. A server or
            // shared module reaching one is refused here instead of failing at
            // link time with an undefined reference.
            { MIRTargetKind::C, "chx_dom_",
              "the 'dom' host API is client-only (JavaScript); call it from a 'client' "
              "module, not from server/shared code" },
            { MIRTargetKind::LLVM, "chx_dom_",
              "the 'dom' host API is client-only (JavaScript); call it from a 'client' "
              "module, not from server/shared code" },
        };
    } // namespace

    const MIRTargetRule* mir_target_rules() { return kRules; }

    std::size_t mir_target_rule_count() { return sizeof(kRules) / sizeof(kRules[0]); }

    const char* mir_unsupported_reason(MIRTargetKind target, MIROpcode opcode) {
        for (const MIRTargetRule& rule : kRules) {
            if (rule.target == target && rule.opcode == opcode) return rule.reason;
        }
        return nullptr;
    }

    const MIRTargetSymbolRule* mir_target_symbol_rules() { return kSymbolRules; }

    std::size_t mir_target_symbol_rule_count() {
        return sizeof(kSymbolRules) / sizeof(kSymbolRules[0]);
    }

    const char* mir_unsupported_symbol_reason(MIRTargetKind target, std::string_view name) {
        for (const MIRTargetSymbolRule& rule : kSymbolRules) {
            if (rule.target != target) continue;
            const std::string_view prefix(rule.name_prefix);
            if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0) {
                return rule.reason;
            }
        }
        return nullptr;
    }

} // namespace mir
