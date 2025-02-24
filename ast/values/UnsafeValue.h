// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"

/**
 * unsafe values replace themselves during symbol resolution
 */
class UnsafeValue : public Value {
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
    inline UnsafeValue(
        ASTAllocator* allocator,
        Value* value
    ) : Value(ValueKind::UnsafeValue, value->encoded_location()), allocator(allocator), value(value) {

    }

    bool link(SymbolResolver &linker, Value *&value_ptr, BaseType *expected_type = nullptr) override {
        const auto prev = linker.safe_context;
        linker.safe_context = false;
        const auto linked = value->link(linker, value, expected_type);
        linker.safe_context = prev;
        value_ptr = value;
        return linked;
    }

};