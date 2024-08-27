// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UCharType.h"

class UCharValue : public IntNumValue {
public:

    unsigned char value;

    explicit UCharValue(unsigned char value) : value(value) {

    }

    ValueKind val_kind() override {
        return ValueKind::UChar;
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { (BaseType*) &UCharType::instance, false };
    }

    BaseType* known_type() override {
        return (BaseType*) &UCharType::instance;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UCharValue *copy() override {
        return new UCharValue(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<UCharType>();
    }

    bool is_unsigned() override {
        return true;
    }

    unsigned int get_num_bits() override {
        return 8;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UChar;
    }

};