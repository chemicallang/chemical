// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/statements/SwitchStatement.h"

class SwitchValue : public Value {
public:

    SwitchStatement stmt;

    /**
     * constructor
     */
    SwitchValue(
        Value* expression,
        ASTNode* parent_node,
        SourceLocation loc
    ) : Value(ValueKind::SwitchValue, loc), stmt(expression, parent_node, loc) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto copied = new (allocator.allocate<SwitchValue>()) SwitchValue(
                stmt.expression->copy(allocator),
                stmt.parent(),
                encoded_location()
        );
        stmt.copy_into(allocator, &stmt);
        return copied;
    }

    ASTNode* linked_node() override;

    Value* evaluated_value(InterpretScope &scope) override;


};