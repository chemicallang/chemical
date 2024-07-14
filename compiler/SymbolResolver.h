// Copyright (c) Qinetik 2024.

#pragma once

#include "preprocess/BaseSymbolResolver.h"
#include "ASTDiagnoser.h"
#include <memory>

class ASTNode;

class BaseFunctionType;

class FunctionDeclaration;

/**
 * ASTLinker provides a way for the nodes to be linked
 * SemanticLinker however provides a way for the tokens to be linked
 * This doesn't link up modules like Linker does which is used for exporting executables
 */
class SymbolResolver : public BaseSymbolResolver<ASTNode>, public ASTDiagnoser {
public:

    /**
     * is the codegen for arch 64bit
     */
    bool is64Bit;

    /**
     * when true, re-declaring same symbol will override it
     */
    bool override_symbols = false;

    /**
     * current function type, for which code is being linked
     */
    BaseFunctionType* current_func_type = nullptr;

    /**
     * nodes that are created during symbol resolution, that must be retained
     * during symbol resolution, for example when a function with two names
     * exists, the second function creates a MultiFunctionNode and put's it on this
     * vector, which is declared on the symbol map, when function calls link with it
     * they are re-linked with correct function, based on arguments
     */
    std::vector<std::unique_ptr<ASTNode>> helper_nodes;

    /**
     * constructor
     */
    explicit SymbolResolver(bool is64Bit);

    /**
     * duplicate symbol error
     */
    void dup_sym_error(const std::string& name, ASTNode* previous, ASTNode* new_node);

    /**
     * declares a node with string : name
     */
    void declare(const std::string &name, ASTNode *node);

    /**
     * helper method that should be used to declare functions that takes into account
     * multiple methods with same names
     */
    void declare_function(const std::string& name, FunctionDeclaration* declaration);

    /**
     * tag
     */
    std::string TAG() override {
        return "SymRes";
    }

};