// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/types/IntNType.h"
#include "IntNumValue.h"
#include "ast/types/IntType.h"

/**
 * @brief Class representing an integer value.
 */
class IntValue : public IntNumValue, public IntType {
public:

    /**
     * @brief Construct a new IntValue object.
     *
     * @param value The integer value.
     */
    IntValue(int value) : value(value) {}

    uint64_t byte_size(bool is64Bit) {
        return 4;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    unsigned int get_num_bits() override {
        return 32;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    IntValue *copy() override {
        return new IntValue(value);
    }

    bool is_unsigned() override {
        return false;
    }

    [[nodiscard]] std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<IntType>();
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    int as_int() override {
        return value;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Int;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::IntN;
    }

    int value; ///< The integer value.
};