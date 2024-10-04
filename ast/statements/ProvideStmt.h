// Copyright (c) Qinetik 2024.


#pragma once

#include "ast/structures/Scope.h"

class ProvideStmt : public ASTNode {
public:

    Value* value;
    std::string identifier;
    Scope body;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * constructor
     */
    ProvideStmt(
        Value* value,
        std::string identifier,
        Scope body,
        ASTNode* parent,
        CSTToken* token
    );

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    CSTToken* cst_token() override {
        return token;
    }

    ASTNode* parent() override {
        return parent_node;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::ProvideStmt;
    }

    void declare_and_link(SymbolResolver &linker) override;

};