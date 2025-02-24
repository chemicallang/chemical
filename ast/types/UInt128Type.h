// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "ast/types/IntNType.h"

class UInt128Type : public IntNType {
public:

    static const UInt128Type instance;

    UInt128Type(SourceLocation location) : IntNType(location) {

    }

    IntNTypeKind IntNKind() const override {
        return IntNTypeKind::UInt128;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 128;
    }

    bool is_unsigned() final {
        return true;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 16;
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    UInt128Type *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UInt128Type>()) UInt128Type(encoded_location());
    }

};