// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level MIR container and the sealed read-only context passed to
// lowering/emission workers. See mir-implementation-plan.md §2.5 and §3.11.

#pragma once

#include "MIRTypes.h"
#include "MIRTypeTable.h"
#include "MIRLayout.h"
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

/**
 * `MIRModuleLayout` (the layout facts the emitters need) lives in MIRLayout.h,
 * together with the aggregate layout pass that turns it into `MIRTypeRecord`
 * sizes/alignments and per-field offsets.
 */

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
    /**
     * Absolute source file paths, indexed by `MIRSourceLoc::file`. Shared by the
     * whole module (a `MIRSourceLoc` stores only the id, keeping the core
     * AST-free). Populated by the lowerer's source-location provider.
     */
    std::vector<std::string> source_files;
    /** Intern a source file path; returns its id. */
    uint32_t source_file(const std::string& path) {
        for (uint32_t i = 0; i < source_files.size(); ++i) {
            if (source_files[i] == path) return i;
        }
        source_files.push_back(path);
        return static_cast<uint32_t>(source_files.size() - 1);
    }
    /**
     * Inline (anonymous or locally-named) struct types discovered while mapping
     * types. They have no top-level C declaration, so the driver emits one.
     * `second` is a `BaseType*` (a `StructType`).
     */
    std::vector<std::pair<uint32_t, void*>> inline_structs;
    void record_inline_struct(uint32_t id, void* type) {
        for (auto& p : inline_structs) {
            if (p.first == id) return;
        }
        inline_structs.emplace_back(id, type);
    }

    void clear() {
        types = {};
        constants = {};
        symbols = {};
        functions.clear();
        source_files.clear();
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
