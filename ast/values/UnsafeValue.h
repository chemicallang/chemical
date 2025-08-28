// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * unsafe values replace themselves during symbol resolution
 * TODO: not yet ready
 */
class UnsafeValue : public Value {
private:

    /**
     * the actual value
     */
    Value* value;

public:

    /**
     * the allocator used at the time of creation
     */
    ASTAllocator* allocator;

    /**
     * constructor
     */
    inline constexpr UnsafeValue(
        ASTAllocator* allocator,
        Value* value
    ) : Value(ValueKind::UnsafeValue, value->getType(), value->encoded_location()), allocator(allocator), value(value) {

    }

    inline Value* getValue() {
        return value;
    }

    void setValue(Value* newValue) {
        value = newValue;
        setType(newValue->getType());
    }

    Value* copy(ASTAllocator &allocator) override {
        return new (allocator.allocate<UnsafeValue>()) UnsafeValue(
            &allocator, value->copy(allocator)
        );
    }

};