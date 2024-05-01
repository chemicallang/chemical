// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"

class BigIntValue : public IntNumValue {
public:

    long long value;

    BigIntValue(long long value) : value(value) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    Value *copy() override {
        return new BigIntValue(value);
    }

    std::string representation() const override {
        return std::to_string(value);
    }

    unsigned int get_num_bits(bool is64Bit) override {
        return 64;
    }

    uint64_t get_num_value() override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::BigInt;
    }

};