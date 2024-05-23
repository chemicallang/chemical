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
class CharValue : public Value {
public:

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    CharValue(char value) : value(value) {}

    uint64_t byte_size(bool is64Bit) const {
        return 1;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    std::unique_ptr<BaseType> create_type() const override {
        return std::make_unique<CharType>();
    }

    Value *copy() override {
        return new CharValue(value);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

#endif

    char as_char() override {
        return value;
    }

    std::string interpret_representation() const override {
        std::string rep;
        rep.append(1, value);
        return rep;
    }

    std::string representation() const override {
        std::string rep;
        rep.append(1, '\'');
        rep.append(1, value);
        rep.append(1, '\'');
        return rep;
    }

    ValueType value_type() const override {
        return ValueType::Char;
    }

    BaseTypeKind type_kind() const override {
        return BaseTypeKind::Char;
    }

    char value; ///< The character value.
};