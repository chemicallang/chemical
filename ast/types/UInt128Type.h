// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UInt128Type : public IntNType {
public:

    static const UInt128Type instance;

    UInt128Type(SourceLocation location) : IntNType(location) {

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

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    UInt128Type *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<UInt128Type>()) UInt128Type(encoded_location());
    }

};