// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class ULongType : public IntNType {
public:

    bool is64Bit;

    static const ULongType instance64Bit;
    static const ULongType instance32Bit;

    /**
     * constructor
     */
    constexpr ULongType(bool is64Bit, SourceLocation location) : is64Bit(is64Bit), IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::ULong;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return is64Bit ? 64 : 32;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    ULongType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ULongType>()) ULongType(is64Bit, encoded_location());
    }

};