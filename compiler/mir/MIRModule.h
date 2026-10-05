// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level MIR container and the sealed read-only context passed to
// lowering/emission workers. See mir-implementation-plan.md §2.5 and §3.11.

#pragma once

#include "MIRTypes.h"
#include "MIRTypeTable.h"
#include "MIRConstantPool.h"
#include "MIRSymbolTable.h"
#include "MIRFunction.h"

#include <span>
#include <vector>

namespace mir {

/** compilation mode the MIR layer is running under */
enum class MIROutputMode : uint8_t {
    DebugQuick = 0,
    DebugComplete,
    Release,
    Interpret,
};

/** target-independent layout facts the emitters need */
struct MIRModuleLayout {
    bool is64Bit = true;
    uint32_t pointer_size = 8;
    uint32_t pointer_alignment = 8;
};

/**
 * A module's MIR. Tables are populated serially before workers start, then
 * treated as immutable. Functions are appended by the coordinator after their
 * artifacts are validated.
 */
struct MIRModule {
    MIRTypeTable types;
    MIRConstantPool constants;
    MIRSymbolTable symbols;
    MIRModuleLayout layout;
    std::vector<MIRFunction> functions;

    void clear() {
        types = {};
        constants = {};
        symbols = {};
        functions.clear();
    }
};

/**
 * Read-only context shared by all workers (see mir-implementation-plan.md
 * §3.11). All fields are references or spans into data owned by MIRModule or
 * the compiler infrastructure; a worker never mutates them.
 */
struct MIRModuleContext {
    const MIRTypeTable& types;
    const MIRConstantPool& constants;
    const MIRSymbolTable& symbols;
    const MIRModuleLayout& layout;
    MIROutputMode mode = MIROutputMode::DebugQuick;
    bool emit_debug_info = false;
    bool is64Bit = true;
};

} // namespace mir
