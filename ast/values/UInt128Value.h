// Copyright (c) Qinetik 2024.

#pragma once

#include "IntNumValue.h"
#include "ast/types/BigIntType.h"
#include "ast/types/UInt128Type.h"

class UInt128Value : public IntNumValue {
public:

    uint64_t low;
    uint64_t high;

    UInt128Value(uint64_t low, uint64_t high) : low(low), high(high) {

    }

    uint64_t byte_size(bool is64Bit) const {
        return 16;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    Value *copy() override {
        return new UInt128Value(low, high);
    }

    std::string representation() const override {
        // TODO representation of __128int
        return std::to_string(get_num_value());
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() const override {
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