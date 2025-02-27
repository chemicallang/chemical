// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class UIntType : public IntNType {
public:

    static const UIntType instance;

    /**
     * constructor
     */
    constexpr UIntType(SourceLocation location) : IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UInt;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 32;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    UIntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UIntType>()) UIntType(encoded_location());
    }

};