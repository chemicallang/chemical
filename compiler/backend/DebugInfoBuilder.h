// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/ast_fwd.h"
#include "compiler/llvmfwd.h"
#include "std/chem_string_view.h"
#include "integration/common/Position.h"

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
    ) : loc_man(loc_man), builder(builder), gen(gen) {

    }

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

    void info(Value *value);

    void info(FunctionDeclaration *decl, llvm::Function* func);

    void info(StructDefinition *structDefinition);

    void info(InterfaceDefinition *interfaceDefinition);

    void info(VarInitStatement *init, llvm::AllocaInst* allocaInst);

    void info(VarInitStatement* init, llvm::GlobalVariable* variable);

    void info(ImplDefinition *implDefinition);

    void info(UnionDef *def);

    void info(VariantDefinition *variant_def);

    void info(Scope *scope);

    void info(IfStatement *ifStatement);

    void info(ValueWrapperNode *node);

    void info(ForLoop *forLoop);

    void info(LoopBlock *scope);

    void info(WhileLoop *whileLoop);

    void info(DoWhileLoop *doWhileLoop);

    void info(AssignStatement *assign);

    void info(SwitchStatement *statement);

    void info(BreakStatement *breakStatement);

    void info(ContinueStatement *continueStatement);

    void info(ReturnStatement *returnStatement);

    void info(InitBlock *initBlock);

    void info(StructMemberInitializer *init);

    void info(DestructStmt *delStmt);

    void info(UnreachableStmt *stmt);

    void info(LambdaFunction *func);

    void info(ExtensionFuncReceiver *receiver);

    void info(FunctionParam *functionParam);

    void info(Namespace *ns);

public:

    // -------------------------------------------
    //       the add methods
    // they just call the info methods above
    // unless the debug info visitor is disabled
    // -------------------------------------------

    inline void add(Value *value) {
        if(isEnabled) {
            info(value);
        }
    }

    inline void add(FunctionDeclaration *functionDeclaration, llvm::Function* func) {
        if(isEnabled) {
            info(functionDeclaration, func);
        }
    }

    inline void add(StructDefinition *structDefinition) {
        if(isEnabled) {
            info(structDefinition);
        }
    }

    inline void add(InterfaceDefinition *interfaceDefinition) {
        if(isEnabled) {
            info(interfaceDefinition);
        }
    }

    inline void add(VarInitStatement *init, llvm::AllocaInst* inst) {
        if(isEnabled) {
            info(init, inst);
        }
    }

    inline void add(VarInitStatement* init, llvm::GlobalVariable* variable) {
        if(isEnabled) {
            info(init, variable);
        }
    }

    inline void add(ImplDefinition *implDefinition) {
        if(isEnabled) {
            info(implDefinition);
        }
    }

    inline void add(UnionDef *def) {
        if(isEnabled) {
            info(def);
        }
    }

    inline void add(VariantDefinition *variant_def) {
        if(isEnabled) {
            info(variant_def);
        }
    }

    inline void add(Scope *scope) {
        if(isEnabled) {
            info(scope);
        }
    }

    inline void add(IfStatement *ifStatement) {
        if(isEnabled) {
            info(ifStatement);
        }
    }

    inline void add(ValueWrapperNode *node) {
        if(isEnabled) {
            info(node);
        }
    }

    inline void add(ForLoop *forLoop) {
        if(isEnabled) {
            info(forLoop);
        }
    }

    inline void add(LoopBlock *scope) {
        if(isEnabled) {
            info(scope);
        }
    }

    inline void add(WhileLoop *whileLoop) {
        if(isEnabled) {
            info(whileLoop);
        }
    }

    inline void add(DoWhileLoop *doWhileLoop) {
        if(isEnabled) {
            info(doWhileLoop);
        }
    }

    inline void add(AssignStatement *assign) {
        if(isEnabled) {
            info(assign);
        }
    }

    inline void add(SwitchStatement *statement) {
        if(isEnabled) {
            info(statement);
        }
    }

    inline void add(BreakStatement *breakStatement) {
        if(isEnabled) {
            info(breakStatement);
        }
    }

    inline void add(ContinueStatement *continueStatement) {
        if(isEnabled) {
            info(continueStatement);
        }
    }

    inline void add(ReturnStatement *returnStatement) {
        if(isEnabled) {
            info(returnStatement);
        }
    }

    inline void add(InitBlock *initBlock) {
        if(isEnabled) {
            info(initBlock);
        }
    }

    inline void add(StructMemberInitializer *init) {
        if(isEnabled) {
            info(init);
        }
    }

    inline void add(DestructStmt *delStmt) {
        if(isEnabled) {
            info(delStmt);
        }
    }

    inline void add(UnreachableStmt *stmt) {
        if(isEnabled) {
            info(stmt);
        }
    }

    inline void add(LambdaFunction *func) {
        if(isEnabled) {
            info(func);
        }
    }

    inline void add(ExtensionFuncReceiver *receiver) {
        if(isEnabled) {
            info(receiver);
        }
    }

    inline void add(FunctionParam *functionParam) {
        if(isEnabled) {
            info(functionParam);
        }
    }

    inline void add(Namespace *ns) {
        if(isEnabled) {
            info(ns);
        }
    }



};