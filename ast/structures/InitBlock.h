// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"
#include "ast/structures/Scope.h"

class InitBlock : public ASTNode {
public:

    Scope scope;
    ASTNode* parent_node;
    CSTToken* token;

    InitBlock(Scope scope, ASTNode* parent_node, CSTToken* token);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::InitBlock;
    }

    ASTNode* parent() override {
        return parent_node;
    }

    CSTToken* cst_token() override {
        return token;
    }

    void declare_and_link(SymbolResolver &linker, std::unique_ptr<ASTNode> &node_ptr) override;

};