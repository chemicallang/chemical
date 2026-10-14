// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "compiler/llvmfwd.h"
#include "std/chem_string_view.h"
#include <vector>

class LocationManager;

class Codegen;

/**
 * Emits DWARF debug information for the LLVM target.
 *
 * The builder is created once per task and driven by the MIR → LLVM emitter
 * (`compiler/CodegenMir.cpp`): that emitter builds the compile unit, the
 * `DISubprogram` per function and the `DIType`s from the MIR type/layout tables,
 * and attaches locations to instructions itself. The AST-shaped API
 * (`create_struct_type`, `create(FunctionTypeBody*, ...)`, file/lexical/function
 * scopes, `instr(...)`) was part of the AST LLVM lowering and went away with it.
 */
class DebugInfoBuilder {
public:

    /**
     * is this enabled, debug calls will be forwarded
     */
    bool isEnabled;

    /**
     * is the code being generated optimized
     * (are optimizations enabled)
     */
    bool isOptimized;

    /**
     * the llvm di builder
     */
    llvm::DIBuilder* builder;

    /**
     * a reference to code generator
     */
    Codegen& gen;

    /**
     * the scopes currently open. The MIR path only ever opens the compile unit
     * of the module being emitted; it is kept as a stack so a future nested
     * scope (lexical block / nested subprogram) can be pushed.
     */
    std::vector<llvm::DIScope*> diScopes;

    /**
     * the compile unit is the unit associated with the current file
     * created via call to createDiCompileUnit
     */
    llvm::DICompileUnit* diCompileUnit = nullptr;

    /**
     * create a debug info builder for a single task
     */
    DebugInfoBuilder(
            llvm::DIBuilder* builder,
            Codegen& gen,
            bool isEnabled
    );

    /**
     * a compile unit is created for every file
     */
    llvm::DICompileUnit* createDiCompileUnit(const chem::string_view& file_abs_path);

    /**
     * this function should be called every time the module (and therefore its
     * DIBuilder) changes
     */
    void update_builder(llvm::DIBuilder* builder);

    /**
     * finalizes the debug information builder
     */
    void finalize();

    /**
     * when you start the di compile unit, you must end the compile unit as well
     */
    void start_di_compile_unit(llvm::DICompileUnit* unit);

    /**
     * this ends the current di compile unit
     */
    void end_di_compile_unit();

};
