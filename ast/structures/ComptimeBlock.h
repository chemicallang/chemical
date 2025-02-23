// Copyright (c) Qinetik 2024.


#pragma once

#include "ast/structures/Scope.h"

class ComptimeBlock : public ASTNode {
public:

    Scope body;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * constructor
     */
    ComptimeBlock(
        Scope body,
        ASTNode* parent,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ComptimeBlock), body(std::move(body)), parent_node(parent), location(location) {

    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNode* parent() final {
        return parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};