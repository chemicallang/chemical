// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class LongType : public IntNType {
public:

    /**
     * constructor
     */
    constexpr LongType() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::Long;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return is64Bit ? 64 : 32;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    Value *create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    LongType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<LongType>()) LongType();
    }


};