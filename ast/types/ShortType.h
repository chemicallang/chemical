// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class ShortType : public IntNType {
public:

    static const ShortType instance;

    /**
     * constructor
     */
    constexpr ShortType(SourceLocation location) : IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::Short;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 16;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 2;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    ShortType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<ShortType>()) ShortType(encoded_location());
    }

};