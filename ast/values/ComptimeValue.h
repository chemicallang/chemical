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
    ) : allocator(allocator), value(value) {

    }

    SourceLocation encoded_location() override {
        return value->encoded_location();
    }

    ValueKind val_kind() override {
        return ValueKind::SymResValue;
    }

    void accept(Visitor *visitor) override {
        throw std::runtime_error("ComptimeValue cannot be visited, as it should be replaced during symbol resolution");
    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override {
        InterpretScope scope(nullptr, *allocator, &linker.comptime_scope);
        // replacing
        const auto eval = value->evaluated_value(scope);
        if(eval) {
            // move the allocated values from interpret scope to the allocator
            // so they are destroyed when the allocator is destroyed
            for(const auto val : scope.allocated) {
                allocator->store_ptr(val);
            }
            scope.allocated.clear();
            // replace value
            value_ptr = eval;
            return true;
        } else {
            return false;
        }
    }

};