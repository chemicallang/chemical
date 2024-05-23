// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UBigIntType.h"

class UBigIntValue : public IntNumValue {
public:

    unsigned long long value;

    UBigIntValue(unsigned long long value) : value(value) {

    }

    uint64_t byte_size(bool is64Bit) const {
        return 8;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *copy() override {
        return new UBigIntValue(value);
    }

    std::string representation() const override {
        return std::to_string(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<UBigIntType>();
    }

    unsigned int get_num_bits() override {
        return 64;
    }

    bool is_unsigned() override {
        return true;
    }

    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UBigInt;
    }

};