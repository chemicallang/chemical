// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class UShortType : public IntNType {
public:

    static const UShortType instance;

    /**
     * constructor
     */
    constexpr UShortType(SourceLocation location) : IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UShort;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 16;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    UShortType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UShortType>()) UShortType(encoded_location());
    }

};