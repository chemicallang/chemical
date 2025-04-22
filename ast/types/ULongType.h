// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class ULongType : public IntNType {
public:

    static const ULongType instance;

    /**
     * constructor
     */
    constexpr ULongType() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::ULong;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return is64Bit ? 64 : 32;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return is64Bit ? 8 : 4;
    }

    Value *create(ASTAllocator& allocator, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    ULongType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ULongType>()) ULongType();
    }

};