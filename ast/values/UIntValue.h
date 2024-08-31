// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/UIntType.h"

class UIntValue : public IntNumValue {
public:

    unsigned int value;
    CSTToken* token;

    explicit UIntValue(unsigned int value, CSTToken* token) : value(value), token(token) {

    }

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::UInt;
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { (BaseType*) &UIntType::instance, false };
    }

    BaseType* known_type() override {
        return (BaseType*) &UIntType::instance;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    UIntValue *copy() override {
        return new UIntValue(value, token);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<UIntType>(nullptr);
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