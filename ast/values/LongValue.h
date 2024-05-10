// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/LongType.h"

class LongValue : public IntNumValue {
public:

    long value;
    bool is64Bit;

    LongValue(long value, bool is64Bit) : value(value), is64Bit(is64Bit) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    Value *copy() override {
        return new LongValue(value, is64Bit);
    }

    std::string representation() const override {
        return std::to_string(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<LongType>(is64Bit);
    }

    unsigned int get_num_bits() override {
        if(is64Bit) {
            return 64;
        } else {
            return 32;
        }
    }

    bool is_unsigned() override {
        return false;
    }

    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::Long;
    }

};