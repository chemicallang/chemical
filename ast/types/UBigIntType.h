// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class UBigIntType : public IntNType {
public:

    static const UBigIntType instance;

    /**
     * constructor
     */
    constexpr UBigIntType() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UBigInt;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return 64;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    Value *create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    UBigIntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType();
    }

};