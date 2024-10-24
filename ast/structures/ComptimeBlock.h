// Copyright (c) Qinetik 2024.


#pragma once

#include "ast/structures/Scope.h"

class ComptimeBlock : public ASTNode {
public:

    Scope body;
    ASTNode* parent_node;
    CSTToken* token;

    /**
     * constructor
     */
    ComptimeBlock(
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
        return ASTNodeKind::ComptimeBlock;
    }

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override;

#endif

};