// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class UInt128Type : public IntNType {
public:

    static const UInt128Type instance;

    /**
     * constructor
     */
    constexpr UInt128Type() : IntNType() {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UInt128;
    }

    [[nodiscard]]
    unsigned int num_bits(bool is64Bit) const final {
        return 128;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    Value *create(ASTAllocator& allocator, TypeBuilder& typeBuilder, uint64_t value, SourceLocation loc) final;

    [[nodiscard]]
    UInt128Type *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UInt128Type>()) UInt128Type();
    }

};