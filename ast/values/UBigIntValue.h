// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UBigIntType.h"

class UBigIntValue : public IntNumValue, public UBigIntType {
public:

    unsigned long long value;

    explicit UBigIntValue(unsigned long long value) : value(value) {

    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    uint64_t byte_size(bool is64Bit) override {
        return 8;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UBigIntValue *copy() override {
        return new UBigIntValue(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<UBigIntType>();
    }

    unsigned int get_num_bits() override {
        return 64;
    }

    bool is_unsigned() override {
        return true;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UBigInt;
    }

};