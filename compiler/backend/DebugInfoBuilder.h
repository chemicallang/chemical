// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/ast_fwd.h"
#include "compiler/llvmfwd.h"
#include "std/chem_string_view.h"
#include "core/diag/Position.h"
#include "core/source/SourceLocation.h"
#include <vector>
#include <unordered_map>

class LocationManager;

class ASTAny;

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
     * the scopes
     */
    std::vector<llvm::DIScope*> diScopes;

    /**
     * the compile unit is the unit associated with current file
     * created via call to createDiCompileUnit
     */
    llvm::DICompileUnit* diCompileUnit = nullptr;

    /**
     * we store pointers in this case
     */
    std::unordered_map<ASTAny*, llvm::DIType*> cachedTypes;

    /**
     * these are only created when self referential data types are used
     */
    std::vector<std::pair<ASTNode*, llvm::DICompositeType*>> replaceAbleTypes;

    /**
     * create a info visitor for a single module
     */
    DebugInfoBuilder(
            LocationManager& loc_man,
            llvm::DIBuilder* builder,
            Codegen& gen,
            bool isEnabled
    );

    /**
     * struct type will be created
     */
    llvm::DICompositeType* create_struct_type(StructDefinition* def);

    /**
     * creates replaceable composite type
     */
    llvm::DICompositeType* create_replaceable_type(StructDefinition* def);

    /**
     * a compile unit is created for every file
     */
    llvm::DICompileUnit* createDiCompileUnit(const chem::string_view& file_abs_path);

    /**
     * this function should be called every time module changes
     */
    void update_builder(llvm::DIBuilder* builder);

    /**
     * finalizes the debug information builder
     */
    void finalize();

    /**
     * get a pointer to di location
     */
    llvm::DILocation* di_loc(const Position& position);

protected:

    llvm::DIScope* create(FunctionTypeBody* decl, llvm::Function* func);

public:

    /**
     * this sets the debug location for a given instruction
     */
    void instr(llvm::Instruction* inst, const Position& position);

    /**
     * this sets the debug location for a given instruction
     */
    void instr(llvm::Instruction* inst, SourceLocation location);

    /**
     * this is just a helper method for nodes, values and types that have encoded locations
     * for which instructions are generated
     */
    template <typename NodeT>
    inline void instr(llvm::Instruction* inst, NodeT* node)
    requires requires(NodeT n) { n.encoded_location(); }
    {
        instr(inst, node->encoded_location());
    }

    /**
     * when you start the di compile unit, you must end the compile unit as well
     * use the end_current_scope to end the compile unit
     */
    void start_di_compile_unit(llvm::DICompileUnit* unit);

    /**
     * this ends the current di compile unit
     */
    void end_di_compile_unit();

    /**
     * starts a nested function scope
     */
    void start_nested_function_scope(FunctionTypeBody *decl, llvm::Function* func);

    /**
     * this creates the function scope and starts it, so further code generation uses the function scope
     * after calling this, a call to end_current_scope must be made
     */
    void start_function_scope(FunctionTypeBody *decl, llvm::Function* func);

    /**
     * this ends the current function scope
     */
    void end_function_scope();

    /**
     * start a lexical code block, a end_current_scope call is expected after this
     */
    void start_scope(SourceLocation location);

    /**
     * this ends a local scope
     */
    void end_scope();

    /**
     * when a function needs to be declared (has no body, just a prototype)
     */
    inline void declare(FunctionType *decl, llvm::Function* func) {
        if(isEnabled) {
            // there was no debug information associated with a declared function for a c compiled file
            // therefore currently we're not doing anything here
        }
    }

    /**
     * should be called when a var init is declared
     */
    void declare(VarInitStatement *init, llvm::Value* allocaInst);

};