// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class LongType : public IntNType {
public:

    static const LongType instance64Bit;
    static const LongType instance32Bit;

    bool is64Bit;

    explicit LongType(bool is64Bit, SourceLocation location) : is64Bit(is64Bit), IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::Long;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return is64Bit ? 64 : 32;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    LongType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<LongType>()) LongType(is64Bit, encoded_location());
    }


};