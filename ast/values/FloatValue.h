// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"

/**
 * @brief Class representing a floating-point value.
 */
class FloatValue : public Value {
public:
    /**
     * @brief Construct a new FloatValue object.
     *
     * @param value The floating-point value.
     */
    FloatValue(float value) : value(value) {}

    std::string representation() const override {
        std::string rep;
        rep.append(std::to_string(value));
        return rep;
    }

#ifdef COMPILER_BUILD
    llvm::Type * llvm_type(Codegen &gen) override;

    llvm::Value * llvm_value(Codegen &gen) override;
#endif

    float as_float() override {
        return value;
    }

    Value *copy(InterpretScope& scope) override {
        return new FloatValue(value);
    }

    void *get_value() override {
        return &value;
    }

private:
    float value; ///< The floating-point value.
};