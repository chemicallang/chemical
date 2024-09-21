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
class IntValue : public IntNumValue {
public:

    int value; ///< The integer value.
    CSTToken* token;

    /**
     * constructor
     */
    explicit IntValue(int value, CSTToken* token) : value(value), token(token) {}

    CSTToken *cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::Int;
    }

    uint64_t byte_size(bool is64Bit) override {
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

    IntValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<IntValue>()) IntValue(value, token);
    }

    bool is_unsigned() override {
        return false;
    }

    [[nodiscard]]
    BaseType* create_type(ASTAllocator& allocator) override {
        return new (allocator.allocate<IntType>()) IntType(nullptr);
    }

//    hybrid_ptr<BaseType> get_base_type() override {
//        return hybrid_ptr<BaseType> { (BaseType*) &IntType::instance, false };
//    }

    BaseType* known_type() override {
        return (BaseType*) &IntType::instance;
    }

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Int;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::IntN;
    }

};