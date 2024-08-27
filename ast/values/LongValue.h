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

    ValueKind val_kind() override {
        return ValueKind::Long;
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { known_type(), false };
    }

    BaseType* known_type() override {
        return (BaseType*) (is64Bit ? &LongType::instance64Bit : &LongType::instance32Bit);
    }

    uint64_t byte_size(bool is64Bit_) override {
        return is64Bit_ ? 8 : 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    LongValue *copy() override {
        return new LongValue(value, is64Bit);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
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

    [[nodiscard]] int64_t get_num_value() const override {
        return value;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::Long;
    }

};