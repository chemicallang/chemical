// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"
#include "ast/types/CharType.h"

/**
 * @brief Class representing a character value.
 */
class CharValue : public Value, public CharType {
public:

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    CharValue(char value) : value(value) {}

    uint64_t byte_size(bool is64Bit) {
        return 1;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    hybrid_ptr<BaseType> get_base_type() override {
        return hybrid_ptr<BaseType> { this, false };
    }

    std::unique_ptr<BaseType> create_type() override {
        return std::make_unique<CharType>();
    }

    Value *copy() override {
        return new CharValue(value);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    char as_char() override {
        return value;
    }

    ValueType value_type() const override {
        return ValueType::Char;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Char;
    }

    char value; ///< The character value.
};