// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"
#include "ast/types/PointerType.h"

class NewValue : public Value {
public:

    Value* value;
    PointerType ptr_type;

    /**
     * constructor
     */
    constexpr NewValue(
        Value* value,
        SourceLocation location
    ) : Value(ValueKind::NewValue, &ptr_type, location), value(value), ptr_type(value->getType(), true) {

    }

    NewValue* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<NewValue>()) NewValue(
                value->copy(allocator),
                encoded_location()
        );
    }

    ASTNode* linked_node() override {
        return value->linked_node();
    }

    Value* evaluated_value(InterpretScope &scope) override;


};