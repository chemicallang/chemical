// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/values/CharValue.h"

/**
 * @brief Class representing a string value.
 */
class StringValue : public Value {
public:

    /**
     * @brief Construct a new StringValue object.
     *
     * @param value The string value.
     */
    StringValue(std::string value) : value(std::move(value)) {}

    void accept(Visitor &visitor) override {
        visitor.visit(this);
    }

    Value *index(InterpretScope &scope, int i) override {
#ifdef DEBUG
        if (i < 0 || i >= value.size()) {
            std::cerr << "[InterpretError] access index " + std::to_string(i) + " out of bounds for string " + value +
                         " of length " + std::to_string(value.size());
        }
#endif
        return new CharValue(value[i]);
    }

    std::string interpret_representation() const override {
        return value;
    }

    std::string as_string() override {
        return value;
    }

#ifdef COMPILER_BUILD

    llvm::Type * llvm_type(Codegen &gen) override;

    llvm::Value *llvm_value(Codegen &gen) override;

    llvm::GlobalVariable * llvm_global_variable(Codegen &gen, bool is_const, const std::string &name) override;

#endif

    Value *copy() override {
        return new StringValue(value);
    }

    std::string representation() const override {
        return "\"" + value + "\"";
    }

    void *get_value() override {
        return &value;
    }

    ValueType value_type() const override {
        return ValueType::String;
    }

private:
    std::string value; ///< The string value.
};