// Copyright (c) Chemical Language Foundation 2025.


#pragma once

#include "ast/structures/Scope.h"

class ComptimeBlock : public ASTNode {
public:

    Scope body;
    ASTNode* parent_node;

    /**
     * constructor
     */
    constexpr ComptimeBlock(
            ASTNode* parent,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::ComptimeBlock, location), body(this, location), parent_node(parent) {

    }

    ASTNode* parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};