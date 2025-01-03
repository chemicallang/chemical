// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Visitor.h"
#include "compiler/llvmfwd.h"
#include "std/chem_string_view.h"

class LocationManager;

/**
 * Debug info context is a class that is retained during emittance of debug information for
 * multiple modules, allowing us to store any information that is required for multiple modules
 */
class DebugInfoContext {
public:

    /**
     * is the code being generated optimized
     * (are optimizations enabled)
     */
    bool isOptimized;

};

/**
 * the debug information visitor is created for emitting debug information of a single module
 */
class DebugInfoBuilder {
public:

    /**
     * is this enabled, debug calls will be forwarded
     */
    bool isEnabled;

    /**
     * the debug info context that is retained during emittance of debug information for a single
     * executable (multiple modules)
     */
    DebugInfoContext& context;

    /**
     * the location manager provides the locations for every
     */
    LocationManager& loc_man;

    /**
     * the llvm di builder
     */
    llvm::DIBuilder* builder;

    /**
     * the compile unit is the unit associated with current file
     * created via call to createDiCompileUnit
     */
    llvm::DICompileUnit* diCompileUnit;

    /**
     * the di scope
     */
    llvm::DIScope* diScope;

    /**
     * the di file
     */
    llvm::DIFile* diFile;

    /**
     * create a info visitor for a single module
     */
    DebugInfoBuilder(
            DebugInfoContext& context,
            LocationManager& loc_man,
            llvm::DIBuilder* builder
    ) : context(context), loc_man(loc_man), builder(builder) {

    }

    /**
     * a compile unit is created for every file
     */
    void createDiCompileUnit(const chem::string_view& file_abs_path);

protected:

    // -------------------------------------------
    //       the info methods
    //  every info method has an implementation
    //  to add debug info for the given node
    // --------------------------------------------

    void info(Value *value);

    void info(FunctionDeclaration *functionDeclaration);

    void info(ExtensionFunction *extensionFunc);

    void info(StructDefinition *structDefinition);

    void info(InterfaceDefinition *interfaceDefinition);

    void info(VarInitStatement *init);

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

    inline void add(FunctionDeclaration *functionDeclaration) {
        if(isEnabled) {
            info(functionDeclaration);
        }
    }

    inline void add(ExtensionFunction *extensionFunc) {
        if(isEnabled) {
            info(extensionFunc);
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

    inline void add(VarInitStatement *init) {
        if(isEnabled) {
            info(init);
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