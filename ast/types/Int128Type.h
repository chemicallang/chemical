// Copyright (c) Qinetik 2024.

#pragma once

#include "ast/types/IntNType.h"

class Int128Type : public IntNType {
public:

    unsigned int num_bits() const override {
        return 128;
    }

    bool is_unsigned() override {
        return false;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 16;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *create(int64_t value) override;

    ValueType value_type() const override {
        return ValueType::Int128;
    }

    BaseType *copy() const override {
        return new Int128Type();
    }

};