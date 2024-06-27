// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class IntType : public IntNType {
public:

    unsigned int num_bits() const override {
        return 32;
    }

    bool is_unsigned() override {
        return false;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 4;
    }

    bool satisfies(Value *value) override;

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::Int;
    }

    BaseType *copy() const override {
        return new IntType();
    }

};