// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

/**
 * multi function node is created when two functions with same name exist
 * however parameter types may be different, this allows us c++ like behavior of
 * overriding, multi function node only exists during symbol resolution
 */
class MultiFunctionNode : public ASTNode {
public:

    std::string name;
    std::vector<FunctionDeclaration*> functions;

    /**
     * constructor
     */
    MultiFunctionNode(std::string name);

    FunctionDeclaration* func_for_call(std::vector<std::unique_ptr<Value>>& args);

    MultiFunctionNode *as_multi_func_node() override {
        return this;
    }

    void accept(Visitor *visitor) override {
        // don't do anything
    }

    ASTNode *parent() override {
        return nullptr;
    }

    void declare_and_link(SymbolResolver &linker) override;

};