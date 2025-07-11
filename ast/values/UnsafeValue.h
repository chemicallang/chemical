// Copyright (c) Qinetik 2025.

#pragma once

#include "ast/base/Value.h"
#include "compiler/SymbolResolver.h"

/**
 * unsafe values replace themselves during symbol resolution
 * TODO: not yet ready
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
    inline constexpr UnsafeValue(
        ASTAllocator* allocator,
        Value* value
    ) : Value(ValueKind::UnsafeValue, value->encoded_location()), allocator(allocator), value(value) {

    }

    bool link(SymbolResolver &linker, BaseType *expected_type = nullptr) override {
        const auto prev = linker.safe_context;
        linker.safe_context = false;
        const auto linked = value->link(linker, expected_type);
        linker.safe_context = prev;
        return linked;
    }

};