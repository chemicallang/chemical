// Copyright (c) Chemical Language Foundation 2025.
//
// Module-level MIR container and the sealed read-only context passed to
// lowering/emission workers. See mir-implementation-plan.md §2.5 and §3.11.

#pragma once

#include "MIRTypes.h"
#include "MIRTypeTable.h"
#include "MIRLayout.h"
#include "MIRConstantPool.h"
#include "MIRConstExpr.h"
#include "MIRSymbolTable.h"
#include "MIRVtableTable.h"
#include "MIRGlobalDecl.h"
#include "MIRDeclList.h"
#include "MIRFunction.h"
#include "MIRRegionSignature.h"

#include <span>
#include <mutex>
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
 * A top-level global *definition*: its symbol (name, type and linkage live on
 * the symbol record) plus its constant initializer. Populated by the
 * front-end collection pass so both backends define globals from MIR rather
 * than walking the AST — action A3 of mir-migration-evaluation.md. The C
 * emitter prints `const_expr_c(module, init)` and the LLVM emitter builds a
 * `Constant` from the same tree. `flags` uses `MIRGlobalDeclFlags` (MIRGlobalDecl.h).
 */
struct MIRGlobal {
    SymbolId symbol = MIR_INVALID_ID;
    ConstExprId init = MIR_INVALID_ID;
    uint8_t flags = MGD_NONE;
    /**
     * The spelling of the global's *declaration* (name, declared type, the
     * unsized-`T[]` declarator, TLS/static/volatile flags), filled by the
     * front-end collection pass. The C emitter prints the definition from this
     * record instead of re-reading the AST at implement time (action A3).
     */
    MIRGlobalDecl decl;
};

/**
 * `MIRModuleLayout` (the layout facts the emitters need) lives in MIRLayout.h,
 * together with the aggregate layout pass that turns it into `MIRTypeRecord`
 * sizes/alignments and per-field offsets.
 */

/**
 * A module's MIR. The interning tables (`types`, `constants`, `symbols`) are
 * mutex-guarded (action E1), so lowering workers may intern concurrently; the
 * other tables are populated by the coordinator. Functions are appended by the
 * coordinator after their artifacts are validated.
 */
struct MIRModule {
    MIRTypeTable types;
    MIRConstantPool constants;
    MIRSymbolTable symbols;
    MIRModuleLayout layout;
    std::vector<MIRFunction> functions;
    /** Constant-expression trees for global initializers (see MIRGlobal). */
    MIRConstExprTable const_exprs;
    /** Top-level global definitions, with their initializer trees. */
    std::vector<MIRGlobal> globals;
    /** Interface vtable type/definition records (see MIRVtableTable.h). */
    MIRVtableTable vtables;
    /**
     * Per-function region signatures (S4.2). Region ids are function-local; the
     * table itself is module-lifetime so `MIRBorrowCheck` never reads the AST.
     * Append-only; lowering workers may add concurrently.
     */
    std::vector<MIRRegionSignature> region_signatures;
    std::mutex region_signatures_mutex;

    uint32_t add_region_signature(MIRRegionSignature signature) {
        std::lock_guard<std::mutex> lock(region_signatures_mutex);
        region_signatures.push_back(std::move(signature));
        return static_cast<uint32_t>(region_signatures.size() - 1);
    }
    /**
     * Symbol -> region signature (S4.2/S4.5). Filled when a function is
     * interned, so a callee's signature is available even before/without its
     * body. Symbol ids are append-only; the vector grows to the symbol id.
     */
    std::vector<uint32_t> symbol_region_signatures;
    std::mutex symbol_region_mutex;
    void set_symbol_region_signature(SymbolId symbol, uint32_t signature) {
        std::lock_guard<std::mutex> lock(symbol_region_mutex);
        if (symbol >= symbol_region_signatures.size()) {
            symbol_region_signatures.resize(symbol + 1, MIR_INVALID_ID);
        }
        symbol_region_signatures[symbol] = signature;
    }
    uint32_t symbol_region_signature(SymbolId symbol) const {
        return symbol < symbol_region_signatures.size()
                   ? symbol_region_signatures[symbol]
                   : MIR_INVALID_ID;
    }
    /**
     * The module's ordered declaration list (forward tags, aggregate bodies,
     * typedefs, prototypes, global declarations and vtable type/externs), filled
     * by the front-end collection pass and rendered by the AST-free
     * `MIRDeclPrinter::decl_list_c` — action A3's declaration-path endgame.
     */
    MIRDeclList declarations;
    /**
     * Absolute source file paths, indexed by `MIRSourceLoc::file`. Shared by the
     * whole module (a `MIRSourceLoc` stores only the id, keeping the core
     * AST-free). Populated by the lowerer's source-location provider.
     */
    std::vector<std::string> source_files;
    /**
     * Declare that one thread performs every mutation of this module's tables
     * (the shipping pipeline). Every reader then skips its table lock; readers
     * may still run concurrently, because the flag only rules out a concurrent
     * *writer*. Leave it off when lowering is parallelised again (see
     * MIRTableLock.h and `kEnableParallelLowering` in ASTProcessor.cpp).
     */
    void set_single_threaded(bool v) {
        types.set_single_threaded(v);
        constants.set_single_threaded(v);
        symbols.set_single_threaded(v);
    }
    bool single_threaded() const { return types.single_threaded(); }

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
    /**
     * "Already recorded" flags indexed by type id. The dedup used to be a linear
     * scan of `inline_structs` on **every** mapped struct type — `map()` calls
     * this for each struct it resolves — so the cost grew quadratically with the
     * number of aggregate types in a module. Type ids are dense, so a flag per id
     * makes the check O(1).
     */
    std::vector<uint8_t> inline_struct_seen_;
    /** Guards `inline_structs` (appended from parallel lowering workers). */
    std::mutex inline_structs_mtx_;
    void record_inline_struct(uint32_t id, void* type) {
        if (id == MIR_INVALID_ID) return;
        std::unique_lock<std::mutex> lock(inline_structs_mtx_, std::defer_lock);
        if (!single_threaded()) lock.lock();
        if (id >= inline_struct_seen_.size()) inline_struct_seen_.resize(id + 1, 0);
        else if (inline_struct_seen_[id]) return;
        inline_struct_seen_[id] = 1;
        inline_structs.emplace_back(id, type);
    }

    void clear() {
        types.clear();
        constants.clear();
        const_exprs.clear();
        symbols.clear();
        functions.clear();
        globals.clear();
        vtables.clear();
        declarations.clear();
        source_files.clear();
        inline_structs.clear();
        inline_struct_seen_.clear();
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
