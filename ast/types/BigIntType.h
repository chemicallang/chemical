// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class BigIntType : public IntNType {
public:

    static const BigIntType instance;

    constexpr BigIntType() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::BigInt;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return 64;
    }

    bool is_unsigned() final {
        return false;
    }

    Value *create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) final;

    BigIntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<BigIntType>()) BigIntType();
    }

};