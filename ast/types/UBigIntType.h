// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class UBigIntType : public IntNType {
public:

    static const UBigIntType instance;

    UBigIntType(SourceLocation location) : IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UBigInt;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 64;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    UBigIntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UBigIntType>()) UBigIntType(encoded_location());
    }

};