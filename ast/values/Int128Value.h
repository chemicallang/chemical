// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"
#include "ast/types/Int128Type.h"

class Int128Value : public IntNumValue, public Int128Type {
public:

    uint64_t magnitude;
    bool is_negative;

    Int128Value(uint64_t magnitude, bool is_negative) : magnitude(magnitude), is_negative(is_negative) {

    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    uint64_t byte_size(bool is64Bit) {
        return 16;
    }

    Value *copy() override {
        return new Int128Value(magnitude, is_negative);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<Int128Type>();
    }

    unsigned int get_num_bits() override {
        return 128;
    }

    bool is_unsigned() override {
        return false;
    }

    int64_t get_num_value() const override {
        if(magnitude < UINT_MAX) {
            if(is_negative) {
                return -magnitude;
            } else {
                return magnitude;
            }
        } else {
            if(is_negative) {
                // Overflow: The Int128 value is too large to fit into uint64_t
                throw std::overflow_error("Int128 value exceeds uint64_t range");
            } else {
                return magnitude;
            }
        }
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::Int128;
    }

};