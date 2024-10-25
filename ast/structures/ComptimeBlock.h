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

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    CSTToken* cst_token() final {
        return token;
    }

    ASTNode* parent() final {
        return parent_node;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::ComptimeBlock;
    }

    void declare_and_link(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};