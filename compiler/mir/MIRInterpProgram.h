// Copyright (c) Chemical Language Foundation 2025.
//
// The persistent MIR tables for the interpretation job. Unlike the C/LLVM
// backends, which build a fresh `MIRModule` per source module and discard it
// after emitting, the interpreter keeps one module (and one lowering arena)
// alive for the whole job so every function's MIR is available at run time.
//
// Because the module is shared across source modules, cross-module references
// are resolved by mangled name (the same way the linker resolves C/LLVM
// symbols), never by symbol id — the lowerer may intern one name more than once.

#pragma once

#include "MIRModule.h"
#include "MIRTypeBuilder.h"
#include "MIRTaskContext.h"

#include <memory>
#include <mutex>

namespace mir {

struct MIRInterpProgram {
    MIRModule module;
    // Both hold references into `module`, so they are heap-allocated after it
    // and never copied.
    std::unique_ptr<MIRTypeBuilder> types;
    std::unique_ptr<MIRTaskContext> task;
    // One lock for every access to the shared `CTranslationContext`, exactly as
    // `implement_module` uses it for the compile paths.
    std::recursive_mutex visitor_mutex;

    MIRInterpProgram()
        : types(new MIRTypeBuilder(module)),
          task(new MIRTaskContext(module, *types)) {
        // One thread lowers (and the interpreter only reads), so the module
        // tables never need their locks (see MIRTableLock.h).
        module.set_single_threaded(true);
    }
};

} // namespace mir
