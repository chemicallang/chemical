// Copyright (c) Qinetik 2024.

#pragma once

#include "Scope.h"

class UnsafeBlock : public ASTNode {
public:

    Scope scope;

    explicit UnsafeBlock(
        Scope scope,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::UnsafeBlock, location), scope(std::move(scope)) {

    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }


    ASTNode* parent() final {
        return scope.parent_node;
    }

    void declare_and_link(SymbolResolver &linker, ASTNode*& node_ptr) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        scope.code_gen(gen);
    }

#endif

};