// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class UBigIntType : public IntNType {
public:

    static const UBigIntType instance;

    using IntNType::IntNType;

    [[nodiscard]]
    unsigned int num_bits() const override {
        return 64;
    }

    bool is_unsigned() override {
        return true;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    bool satisfies(Value *value) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::UBigInt;
    }

    [[nodiscard]]
    UBigIntType *copy() const override {
        return new UBigIntType(token);
    }

};