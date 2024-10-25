// Copyright (c) Qinetik 2024.

#pragma once

#include "Scope.h"

class UnsafeBlock : public ASTNode {
public:

    Scope scope;
    CSTToken* token;

    explicit UnsafeBlock(Scope scope, CSTToken* token);

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    CSTToken* cst_token() final {
        return token;
    }

    ASTNode* parent() final {
        return scope.parent_node;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::UnsafeBlock;
    }

    void declare_and_link(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        scope.code_gen(gen);
    }

#endif

};