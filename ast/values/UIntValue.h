// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UIntType.h"

class UIntValue : public IntNumValue, public UIntType {
public:

    unsigned int value;

    explicit UIntValue(unsigned int value) : value(value) {

    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    BaseType* known_type() override {
        return this;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UIntValue *copy() override {
        return new UIntValue(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<UIntType>();
    }

    bool is_unsigned() override {
        return true;
    }

    unsigned int get_num_bits() override {
        return 32;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UInt;
    }

};