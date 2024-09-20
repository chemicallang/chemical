// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class Value;

class ThrowStatement : public ASTNode {
public:

    Value* value;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * constructor
     */
    ThrowStatement(Value* value, ASTNode* parent_node, CSTToken* token);

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ThrowStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};