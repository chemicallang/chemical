// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 09/03/2024.
//

#pragma once

#include "ast/base/ASTNode.h"

class Comment : public ASTNode {
public:

    ASTNode* parent_node;
    std::string comment;
    bool multiline;
    CSTToken* token;

    Comment(std::string comment, bool multiline, ASTNode* parent, CSTToken* token) : comment(std::move(comment)), multiline(multiline), parent_node(parent), token(token) {}

    ASTNodeKind kind() override {
        return ASTNodeKind::CommentStmt;
    }

    CSTToken *cst_token() override {
        return token;
    }

    void set_parent(ASTNode* new_parent) override {
        parent_node = new_parent;
    }

    ASTNode *parent() override {
        return parent_node;
    }

    void interpret(InterpretScope &scope) override {
        // do nothing, since it's a comment
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

#ifdef COMPILER_BUILD
    void code_gen(Codegen &gen) override {
        // doesn't generate anything
    }
#endif

};