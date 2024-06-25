// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"
#include "ast/types/UInt128Type.h"

class UInt128Value : public IntNumValue, public UInt128Type {
public:

    uint64_t low;
    uint64_t high;

    UInt128Value(uint64_t low, uint64_t high) : low(low), high(high) {

    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    uint64_t byte_size(bool is64Bit) {
        return 16;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *copy() override {
        return new UInt128Value(low, high);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<UInt128Type>();
    }

    unsigned int get_num_bits() override {
        return 128;
    }

    bool is_unsigned() override {
        return true;
    }

    int64_t get_num_value() const override {
        if (high > 0) {
            // Overflow: The UInt128 value is too large to fit into a uint64_t
            throw std::overflow_error("UInt128 value exceeds uint64_t range");
        }
        return low;
    }

    [[nodiscard]] ValueType value_type() const override {
        return ValueType::UInt128;
    }

};