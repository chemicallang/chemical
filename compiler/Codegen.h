// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#ifdef COMPILER_BUILD

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "llvmfwd.h"
#include "ASTDiagnoser.h"
#include "OutputMode.h"
#include "CodegenEmitterOptions.h"
#include "std/chem_string_view.h"
#include "backend/DebugInfoBuilder.h"
#include "CodegenOptions.h"

class ASTAllocator;

class CompilerBinder;

class GlobalInterpretScope;

class NameMangler;

class CoreNodes;

class ImplementationsIndex;

namespace mir {
struct MIRModule;
struct MIRFunction;
struct MIRTypeRecord;
}

/**
 * Per-task state of the LLVM target.
 *
 * Code generation is MIR-driven: `ASTProcessor` builds the canonical MIR module
 * from the resolved AST and `CodegenMir.cpp` lowers that MIR to LLVM IR (types,
 * globals, function bodies and debug info). The AST-based lowering — the per-node
 * `llvm_value` / `llvm_pointer` / `llvm_type` methods, the destructible queue and
 * the coroutine contexts — has been deleted, so this class only carries:
 *
 *  - the shared compiler services the lowering needs (binder, comptime scope,
 *    options, mangler, allocator, core nodes, implementations index),
 *  - the LLVM context / module / IRBuilder plumbing,
 *  - the target machine and the object/IR/link driver,
 *  - the debug info builder,
 *  - the per-module MIR -> LLVM caches.
 */
class Codegen : public ASTDiagnoser {
public:

    /**
     * used to get the functions invoked for embedded nodes and values
     */
    CompilerBinder& binder;

    /**
     * global interpret scope, is the scope used for compile time evaluation
     * of code
     */
    GlobalInterpretScope& comptime_scope;

    /**
     * the codegen options determine what kind of code is generated
     */
    CodegenOptions& options;

    /**
     * the name mangler is used to mangle names
     */
    NameMangler& mangler;

    /**
     * allocator is used to allocate ast things during code generation
     */
    ASTAllocator& allocator;

    /**
     * the core module contains the interfaces and function definitions for operator overloading...
     */
    CoreNodes& coreNodes;

    /**
     * implementations index contains the impl blocks, where we can find
     * exact implementations of operator overloading
     */
    ImplementationsIndex& implsIndex;

    /**
     * the code generation mode
     */
    OutputMode mode;

    /**
     * the debug info builder
     */
    DebugInfoBuilder di;

    // ── MIR → LLVM caches (per module; cleared by mir_reset_caches) ─────────
    std::unordered_map<uint32_t, llvm::Type*> mir_type_cache_;
    std::unordered_map<uint32_t, llvm::Function*> mir_fn_cache_;
    std::unordered_map<uint32_t, llvm::GlobalVariable*> mir_global_cache_;
    std::unordered_map<std::string, llvm::GlobalVariable*> mir_named_globals_;
    std::unordered_map<std::string, llvm::StructType*> mir_vtable_types_;
    std::unordered_map<std::string, std::vector<std::string>> mir_vtable_methods_;
    std::unordered_map<std::string, std::vector<llvm::FunctionType*>> mir_vtable_slot_types_;

    /**
     * MIR-authoritative aggregate element layout for the LLVM backend (action
     * of mir-migration-evaluation.md: the backend consumes the MIR layout, it
     * does not re-derive one). One entry per LLVM struct element, in body order:
     * `field == 0xFFFFFFFF` is a padding element, otherwise it is the first MIR
     * field index mapped here and `field_count` is the number of overlapping
     * fields (1 for a normal member, > 1 for a union/variant overlap group).
     * Filled by `mir_type`, read by `gep_path` and the constant builder.
     */
    struct MIRAggElem {
        uint32_t field = 0xFFFFFFFFu;
        uint32_t field_count = 0;
    };
    std::unordered_map<uint32_t, std::vector<MIRAggElem>> mir_agg_elems_;
    /** aggregate type id -> MIR field index -> LLVM element (GEP) index. */
    std::unordered_map<uint32_t, std::vector<uint32_t>> mir_field_elem_;

    // MIR → LLVM debug info (per module / per function, when DI is enabled).
    llvm::DIFile* mir_di_file_ = nullptr;      // current module's source file
    llvm::DISubprogram* mir_di_sp_ = nullptr;  // current function's DISubprogram
    std::unordered_map<std::string, llvm::DIFile*> mir_di_files_;
    std::unordered_map<uint32_t, llvm::DIType*> mir_di_types_;

    /**
     * TargetTriple , which we are generating code for !
     */
    std::string target_triple;

    /**
     * the target machine
     */
    llvm::TargetMachine* TargetMachine = nullptr;

    /**
     * LLVM context that holds modules
     */
    std::unique_ptr<llvm::LLVMContext> ctx;

    /**
     * module holds functions, global vars and stuff
     */
    std::unique_ptr<llvm::Module> module;

    /**
     * updated everytime module changes
     */
    std::unique_ptr<llvm::DIBuilder> diBuilder;

    /**
     * the builder that builds ir
     */
    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter> *builder;

    /**
     * constructor
     */
    Codegen(
            CodegenOptions& options,
            CompilerBinder& binder,
            GlobalInterpretScope& comptime_scope,
            NameMangler& mangler,
            CoreNodes& coreNodes,
            ImplementationsIndex& implsIndex,
            std::string target_triple,
            std::string curr_exe_path,
            bool debug_info,
            ASTAllocator& allocator
    );

    /**
     * initializes the llvm module and context
     */
    void module_init(const chem::string_view& scope_name, const chem::string_view& module_name);

    /**
     * determine whether the system we are compiling for is 64bit or 32bit
     */
    static bool is_arch_64bit(const std::string_view &target_triple);

    // ── MIR → LLVM lowering ─────────────────────────────────────────────────
    // The MIR-driven LLVM backend. Types are lowered from the canonical MIR
    // type table (with aggregate layout), and function bodies are lowered from
    // MIR instructions. No AST virtual dispatch is used.
    /** Lower a MIR type id to an LLVM type (cached). */
    llvm::Type* mir_type(const mir::MIRModule& module, uint32_t id);
    /** The LLVM FunctionType for a MIR function type id (no pointer wrapper). */
    llvm::FunctionType* mir_fn_type(const mir::MIRModule& module, uint32_t fn_type);
    /** Create/intern an LLVM vtable struct type (one function-pointer per slot). */
    llvm::StructType* mir_vtable_struct(const std::string& name,
                                        const std::vector<llvm::FunctionType*>& slots);
    /** The function type of a vtable slot (for indirect calls). */
    llvm::FunctionType* mir_vtable_slot_type(const std::string& type_name, uint32_t index) const;
    /** Resolve a vtable method name to its slot index. */
    bool mir_vtable_method_index(const std::string& type_name, const std::string& method,
                                 uint32_t& out) const;
    /** Register a vtable global by name (for `GlobalAddr` of vtable symbols). */
    void mir_register_named_global(const std::string& name, llvm::GlobalVariable* gv) {
        mir_named_globals_[name] = gv;
    }
    /** Declare/define the LLVM function for a MIR function symbol. */
    llvm::Function* mir_function(const mir::MIRModule& module, uint32_t symbol);
    /** Emit the LLVM definition of a MIR function body. */
    bool mir_emit_function(const mir::MIRModule& module, const mir::MIRFunction& fn);

    /** LLVM DIType for a MIR type id (used by the MIR debug-info emission). */
    llvm::DIType* mir_di_type(const mir::MIRModule& module, uint32_t type);

    /**
     * The LLVM struct element index for MIR field `field` of aggregate `agg`
     * (identity when the aggregate has no padding/overlap). The mapping is
     * published by `mir_type`, so every GEP into an aggregate goes through it.
     */
    uint32_t mir_field_elem(uint32_t agg, uint32_t field) const;
    /** Declare LLVM globals for the module's MIR global symbols. */
    void mir_declare_globals(const mir::MIRModule& module);
    /**
     * Apply each `MIRGlobal`'s constant-expression initializer to the LLVM global
     * `mir_declare_globals` declared for it (action A3 of
     * mir-migration-evaluation.md). The initializer tree is target-neutral and
     * produced once by `MIRLowerer`; this pass reads no AST.
     */
    void mir_apply_global_initializers(const mir::MIRModule& module);
    /** Clear per-module MIR caches (call on module switch). */
    void mir_reset_caches() {
        mir_type_cache_.clear();
        mir_fn_cache_.clear();
        mir_global_cache_.clear();
        mir_named_globals_.clear();
        mir_vtable_types_.clear();
        mir_vtable_methods_.clear();
        mir_vtable_slot_types_.clear();
        mir_agg_elems_.clear();
        mir_field_elem_.clear();
        mir_di_file_ = nullptr;
        mir_di_sp_ = nullptr;
        mir_di_files_.clear();
        mir_di_types_.clear();
    }

    /**
     * prints the current module to console
     */
    void print_to_console();

    /**
     * sets up the module for the given target
     */
    llvm::TargetMachine *setup_for_target(const std::string &TargetTriple, bool isDebug);

    /**
     * options will be used to save to files
     */
    bool save_with_options(CodegenEmitterOptions* options);

    /**
     * prints the current module as LLVM IR to a .ll file with given out_path
     */
    bool save_to_ll_file_for_debugging(const std::string &out_path) const;

    /**
     * save as a bitcode file
     */
    bool save_to_bc_file(std::string &out_path, OutputMode mode);

    /**
     * saves as assembly file to this path
     */
    bool save_to_assembly_file(std::string &out_path, OutputMode mode);

    /**
     * saves as object file to this path
     */
    bool save_to_object_file(std::string &out_path, OutputMode mode);

    ~Codegen();

};

#endif
