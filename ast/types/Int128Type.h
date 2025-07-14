// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class Int128Type : public IntNType {
public:

    static const Int128Type instance;

    /**
     * constructor
     */
    constexpr Int128Type() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::Int128;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return 128;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    Value *create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    Int128Type *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<Int128Type>()) Int128Type();
    }

};