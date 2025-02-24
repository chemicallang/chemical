// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"

/**
 * comptime value value replaces itself during symbol resolution by evaluating
 * the contained value
 */
class ComptimeValue : public Value {
public:

    /**
     * the allocator used at the time of creation
     */
    ASTAllocator* allocator;

    /**
     * the actual value
     */
    Value* value;

    /**
     * constructor
     */
    inline ComptimeValue(
        ASTAllocator* allocator,
        Value* value
    ) : Value(ValueKind::ComptimeValue, value->encoded_location()), allocator(allocator), value(value) {

    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override {
        if(!value->link(linker, value, expected_type)) {
            return false;
        }
        InterpretScope scope(nullptr, *allocator, &linker.comptime_scope);
        // replacing
        const auto eval = value->evaluated_value(scope);
        if(!eval) {
            return false;
        }
        // move the allocated values from interpret scope to the allocator
        // so they are destroyed when the allocator is destroyed
        for(const auto val : scope.allocated) {
            allocator->store_ptr(val);
        }
        scope.allocated.clear();
        // replace value
        value_ptr = eval;
        return true;
    }

};