// Copyright (c) Qinetik 2024.


#pragma once

#include "ast/structures/Scope.h"

class ComptimeBlock : public ASTNode {
public:

    Scope body;
    ASTNode* parent_node;

    /**
     * constructor
     */
    ComptimeBlock(
        Scope body,
        ASTNode* parent,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ComptimeBlock, location), body(std::move(body)), parent_node(parent) {

    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }


    ASTNode* parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};