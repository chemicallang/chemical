// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"

/**
 * @brief Class representing an integer value.
 */
class IntValue : public IntNumValue {
public:

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    IntValue(int value) : value(value) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    std::string representation() const override {
        return std::to_string(value);
    }

    unsigned int get_num_bits(bool is64Bit) override {
        return 32;
    }

    uint64_t get_num_value() override {
        return value;
    }

    Value *copy() override {
        return new IntValue(value);
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<IntNType>(32);
    }

    int as_int() override {
        return value;
    }

    ValueType value_type() const override {
        return ValueType::Int;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::IntN;
    }


private:
    int value; ///< The integer value.
};