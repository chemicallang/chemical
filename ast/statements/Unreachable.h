// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/base/LoopASTNode.h"

class UnreachableStmt : public ASTNode {
public:

    ASTNode* parent_node;
    CSTToken* token;

    /**
     * Construct a new ContinueStatement object.
     */
    UnreachableStmt(ASTNode* parent_node, CSTToken* token) : parent_node(parent_node), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::UnreachableStmt;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override;
#endif

};