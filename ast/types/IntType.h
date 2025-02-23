// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class IntType : public IntNType {
public:

    static const IntType instance;

    /**
     * constructor
     */
    IntType(SourceLocation location) : IntNType(location) {

    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 32;
    }

    bool is_unsigned() final {
        return false;
    }

    uint64_t byte_size(bool is64Bit) final {
        return 4;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(ASTAllocator& allocator, uint64_t value) final;

    [[nodiscard]]
    IntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<IntType>()) IntType(encoded_location());
    }

};