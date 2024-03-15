// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"

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
    StringValue(std::string  value) : value(std::move(value)) {}

    std::string interpret_representation() const override {
        return value;
    }

    bool primitive() override {
        return true;
    }

    std::string as_string() override {
        return value;
    }

#ifdef COMPILER_BUILD
    llvm::Value * llvm_value(Codegen &gen) override {
        return gen.builder->CreateGlobalStringPtr(value);
    }
#endif

    Value * copy() override {
        return new StringValue(value);
    }

    std::string representation() const override {
        return "\"" + value + "\"";
    }

    void * get_value() override {
        return &value;
    }

private:
    std::string value; ///< The string value.
};