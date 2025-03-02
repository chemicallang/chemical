// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/ASTNode.h"

class Value;

class ThrowStatement : public ASTNode {
public:

    Value* value;

    /**
     * constructor
     */
    constexpr ThrowStatement(
        Value* value,
        ASTNode* parent_node,
        SourceLocation location
    ) : ASTNode(ASTNodeKind::ThrowStmt, parent_node, location), value(value) {

    }

    ThrowStatement* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<ThrowStatement>()) ThrowStatement(value->copy(allocator), parent(), encoded_location());
    }

#ifdef COMPILER_BUILD

    void code_gen(Codegen &gen) final;

#endif

};