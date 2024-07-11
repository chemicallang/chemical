// Copyright (c) Qinetik 2024.

#pragma once

#include "preprocess/BaseSymbolResolver.h"
#include "ASTDiagnoser.h"

class ASTNode;

class BaseFunctionType;

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
     * constructor
     */
    explicit SymbolResolver(bool is64Bit);

    /**
     * declares a node with string : name
     */
    void declare(const std::string &name, ASTNode *node);

    /**
     * tag
     */
    std::string TAG() override {
        return "SymRes";
    }

};