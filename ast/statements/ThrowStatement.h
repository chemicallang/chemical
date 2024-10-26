// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/base/ASTNode.h"

class Value;

class ThrowStatement : public ASTNode {
public:

    Value* value;
    ASTNode* parent_node;
    SourceLocation location;

    /**
     * constructor
     */
    ThrowStatement(
            Value* value,
            ASTNode* parent_node,
            SourceLocation location
    );

    SourceLocation encoded_location() final {
        return location;
    }

    ASTNodeKind kind() final {
        return ASTNodeKind::ThrowStmt;
    }

    void set_parent(ASTNode* new_parent) final {
        parent_node = new_parent;
    }

    ASTNode *parent() final {
        return parent_node;
    }

    void accept(Visitor *visitor) final;

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};