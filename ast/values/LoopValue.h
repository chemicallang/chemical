// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/LoopBlock.h"

class LoopValue : public Value {
public:

    LoopBlock stmt;

    /**
     * constructor
     */
    LoopValue(
            ASTNode* parent_node,
            SourceLocation loc
    ) : Value(ValueKind::LoopValue, loc), stmt(parent_node, loc) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<LoopValue>()) LoopValue(
                stmt.parent(),
                encoded_location()
        );
        stmt.copy_into(allocator, &copied->stmt);
        return copied;
    }

    Value* evaluated_value(InterpretScope& scope) override;


};