// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UIntType.h"

class UIntValue : public IntNumValue {
public:

    unsigned int value;

    UIntValue(unsigned int value) : value(value) {

    }

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    Value *copy() override {
        return new UIntValue(value);
    }

    std::string representation() const override {
        return std::to_string(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<UIntType>();
    }

    bool is_unsigned() override {
        return true;
    }

    unsigned int get_num_bits() override {
        return 32;
    }

    uint64_t get_num_value() override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UInt;
    }

};