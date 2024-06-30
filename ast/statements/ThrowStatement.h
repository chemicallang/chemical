// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class Value;

class ThrowStatement : public ASTNode {
public:

    std::unique_ptr<Value> value;
    ASTNode* parent_node;

    /**
     * constructor
     */
    ThrowStatement(std::unique_ptr<Value> value, ASTNode* parent_node);

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};