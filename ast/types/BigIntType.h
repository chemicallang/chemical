// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class BigIntType : public IntNType {
public:

    static const BigIntType instance;

    using IntNType::IntNType;

    uint64_t byte_size(bool is64Bit) final {
        return 8;
    }

    [[nodiscard]]
    unsigned int num_bits() const final {
        return 64;
    }

    bool is_unsigned() final {
        return false;
    }

    void accept(Visitor *visitor) final {
        visitor->visit(this);
    }

    Value *create(int64_t value) final;

    [[nodiscard]]
    ValueType value_type() const final {
        return ValueType::BigInt;
    }

    BigIntType *copy(ASTAllocator& allocator) const final {
        return new (allocator.allocate<BigIntType>()) BigIntType(token);
    }

};