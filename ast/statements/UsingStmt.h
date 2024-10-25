// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/values/AccessChain.h"

/**
 * using statement just like in c++, it allows users to bring symbols in current scope
 * from namespaces
 */
class UsingStmt : public AnnotableNode {
public:

    AccessChain chain;
    bool is_namespace = false;
    SourceLocation location;

    UsingStmt(
        std::vector<ChainValue*> values,
        ASTNode* parent_node,
        bool is_namespace,
        SourceLocation location
    );

    UsingStmt(
            AccessChain* chain,
            bool is_namespace,
            SourceLocation location
    );

    SourceLocation encoded_location() override {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::UsingStmt;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    void set_parent(ASTNode* new_parent) final {
        chain.set_parent(new_parent);
    }

    ASTNode *parent() final {
        return chain.parent_node;
    }

    void declare_top_level(SymbolResolver &linker) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final {
        // does nothing
    }

#endif

};