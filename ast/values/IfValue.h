// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/structures/If.h"

class IfValue : public Value {
public:

    IfStatement stmt;

    /**
     * constructor
     */
    IfValue(
        Value* condition,
        ASTNode* parent_node,
        SourceLocation loc
    ) : Value(ValueKind::IfValue, loc), stmt(condition, parent_node, loc) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<IfValue>()) IfValue(
                stmt.condition->copy(allocator),
                stmt.parent(),
                encoded_location()
        );
        stmt.copy_into(allocator, &stmt);
        return copied;
    }

    bool compile_time_computable() override {
        return stmt.compile_time_computable();
    }

    ASTNode* linked_node() override;

    Value* evaluated_value(InterpretScope &scope) override;


};