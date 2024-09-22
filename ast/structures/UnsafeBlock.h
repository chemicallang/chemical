// Copyright (c) Qinetik 2024.

#pragma once

#include "Scope.h"

class UnsafeBlock : public ASTNode {
public:

    Scope scope;
    CSTToken* token;

    explicit UnsafeBlock(Scope scope);

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    CSTToken* cst_token() override {
        return token;
    }

    ASTNode* parent() override {
        return scope.parent_node;
    }

    ASTNodeKind kind() override {
        return ASTNodeKind::UnsafeBlock;
    }

    void declare_and_link(SymbolResolver &linker) override;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) override {
        scope.code_gen(gen);
    }

#endif

};