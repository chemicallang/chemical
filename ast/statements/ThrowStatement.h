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
    ) : ASTNode(ASTNodeKind::ThrowStmt), value(value), parent_node(parent_node), location(location) {

    }

    SourceLocation encoded_location() final {
        return location;
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