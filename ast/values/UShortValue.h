// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UShortType.h"

class UShortValue : public IntNumValue, public UShortType {
public:

    unsigned short value;

    UShortValue(unsigned short value) : value(value) {

    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    uint64_t byte_size(bool is64Bit) {
        return 2;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *copy() override {
        return new UShortValue(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<UShortType>();
    }

    unsigned int get_num_bits() override {
        return 16;
    }

    bool is_unsigned() override {
        return true;
    }

    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UShort;
    }

};