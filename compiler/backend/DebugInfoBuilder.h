// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/ast_fwd.h"
#include "compiler/llvmfwd.h"
#include "std/chem_string_view.h"
#include "integration/common/Position.h"
#include "cst/SourceLocation.h"

class LocationManager;

class Codegen;

/**
 * the debug information visitor is created for emitting debug information
 * the object of debug info builder is created once for each task (executable)
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
     * the location manager provides the locations for every
     */
    LocationManager& loc_man;

    /**
     * the llvm di builder
     */
    llvm::DIBuilder* builder;

    /**
     * a reference to code generator
     */
    Codegen& gen;

    /**
     * the compile unit is the unit associated with current file
     * created via call to createDiCompileUnit
     */
    llvm::DICompileUnit* diCompileUnit = nullptr;

    /**
     * the di scope
     */
    llvm::DIScope* diScope = nullptr;

    /**
     * the di file
     */
    llvm::DIFile* diFile = nullptr;

    /**
     * create a info visitor for a single module
     */
    DebugInfoBuilder(
            LocationManager& loc_man,
            llvm::DIBuilder* builder,
            Codegen& gen
    );

    /**
     * a compile unit is created for every file
     */
    void createDiCompileUnit(const chem::string_view& file_abs_path);

    /**
     * this function should be called every time module changes
     */
    void update_builder(llvm::DIBuilder* builder);

    /**
     * finalizes the debug information builder
     */
    void finalize();

    //-------------------------------------------
    //       these are useful methods
    //-------------------------------------------

    /**
     * get a pointer to di location
     */
    llvm::DILocation* di_loc(const Position& position);

protected:

    // -------------------------------------------
    //       the info methods
    //  every info method has an implementation
    //  to add debug info for the given node
    // --------------------------------------------

    void instr(llvm::Instruction* inst, const Position& position);

    void info(FunctionDeclaration *decl, llvm::Function* func);

    void info(VarInitStatement *init, llvm::AllocaInst* allocaInst);

    void info(FunctionCall* call, llvm::CallInst* callInst);

public:

    void instr(llvm::Instruction* inst, SourceLocation location);

    template <typename NodeT>
    inline void instr(llvm::Instruction* inst, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        instr(inst, node->encoded_location());
    }

    // -------------------------------------------
    //       the add methods
    // they just call the info methods above
    // unless the debug info visitor is disabled
    // -------------------------------------------

    inline void add(FunctionDeclaration *functionDeclaration, llvm::Function* func) {
        if(isEnabled) {
            info(functionDeclaration, func);
        }
    }

    inline void add(VarInitStatement *init, llvm::AllocaInst* inst) {
        if(isEnabled) {
            info(init, inst);
        }
    }

};