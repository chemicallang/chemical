// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class UShortType : public IntNType {
public:

    static const UShortType instance;

    /**
     * constructor
     */
    constexpr UShortType() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UShort;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return 16;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    Value *create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    UShortType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UShortType>()) UShortType();
    }

};