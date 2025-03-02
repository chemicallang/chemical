// Copyright (c) Chemical Language Foundation 2025.


#pragma once

#include "ast/structures/Scope.h"

class ComptimeBlock : public ASTNode {
public:

    Scope body;

    /**
     * constructor
     */
    constexpr ComptimeBlock(
            ASTNode* parent,
            SourceLocation location
    ) : ASTNode(ASTNodeKind::ComptimeBlock, parent, location), body(this, location) {

    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};