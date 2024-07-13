// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

/**
 * multi function node is created when two functions with same name exist
 * however parameter types may be different, this allows us c++ like behavior of
 * overriding
 */
class MultiFunctionNode : public ASTNode {
public:

    std::string name;
    std::vector<std::unique_ptr<FunctionDeclaration>> functions;

    /**
     * constructor
     */
    MultiFunctionNode(std::string name);

    void declare_top_level(SymbolResolver &linker) override;

    void declare_and_link(SymbolResolver &linker) override;

};