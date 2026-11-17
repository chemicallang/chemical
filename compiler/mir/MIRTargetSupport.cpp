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
    } // namespace

    const MIRTargetRule* mir_target_rules() { return kRules; }

    std::size_t mir_target_rule_count() { return sizeof(kRules) / sizeof(kRules[0]); }

    const char* mir_unsupported_reason(MIRTargetKind target, MIROpcode opcode) {
        for (const MIRTargetRule& rule : kRules) {
            if (rule.target == target && rule.opcode == opcode) return rule.reason;
        }
        return nullptr;
    }

} // namespace mir
