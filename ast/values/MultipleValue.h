// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/base/Value.h"

/**
 * multiple value
 */
class MultipleValue : public Value {
public:

    std::vector<Value*> values;

    /**
     * constructor
     */
    MultipleValue(
        BaseType* type,
        SourceLocation location
    ) : Value(ValueKind::Multiple, type, location) {

    }

    Value* copy(ASTAllocator &allocator) override {
        const auto other = new (allocator.allocate<MultipleValue>()) MultipleValue(
            getType(), encoded_location()
        );
        for(const auto val : values) {
            other->values.emplace_back(val->copy(allocator));
        }
        return other;
    }


};