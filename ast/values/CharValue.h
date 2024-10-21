// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/values/IntNumValue.h"
#include "ast/types/CharType.h"

/**
 * @brief Class representing a character value.
 */
class CharValue : public IntNumValue {
public:

    char value; ///< The character value.
    CSTToken* token;

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    CharValue(char value, CSTToken* token) : value(value), token(token) {

    }

    unsigned int get_num_bits() override {
        return 8;
    }

    [[nodiscard]]
    int64_t get_num_value() const override {
        return value;
    }

    bool is_unsigned() override {
        return false;
    }

    CSTToken* cst_token() override {
        return token;
    }

    ValueKind val_kind() override {
        return ValueKind::Char;
    }

    uint64_t byte_size(bool is64Bit) override {
        return 1;
    }

    void accept(Visitor *visitor) override {
        visitor->visit(this);
    }

    BaseType* known_type() override {
        return (BaseType*) &CharType::instance;
    }

    BaseType* create_type(ASTAllocator& allocator) override {
        return new (allocator.allocate<CharType>()) CharType(nullptr);
    }

    CharValue *copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<CharValue>()) CharValue(value, token);
    }

#ifdef COMPILER_BUILD

    llvm::Type *llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen, BaseType* expected_type) override;

#endif

    [[nodiscard]]
    ValueType value_type() const override {
        return ValueType::Char;
    }

    [[nodiscard]]
    BaseTypeKind type_kind() const override {
        return BaseTypeKind::IntN;
    }

};