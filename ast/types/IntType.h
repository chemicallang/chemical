// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class IntType : public IntNType {
public:

    static const IntType instance;

    /**
     * constructor
     */
    constexpr IntType() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::Int;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return 32;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    Value *create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    IntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<IntType>()) IntType();
    }

};