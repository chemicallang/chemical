// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class BigIntType : public IntNType {
public:

    static const BigIntType instance;

    using IntNType::IntNType;

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    [[nodiscard]]
    unsigned int num_bits() const override {
        return 64;
    }

    bool is_unsigned() override {
        return false;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::BigInt;
    }

    BigIntType *copy(ASTAllocator& allocator) const override {
        return new (allocator.allocate<BigIntType>()) BigIntType(token);
    }

};