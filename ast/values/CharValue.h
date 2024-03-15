// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include "ast/base/Value.h"

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

    bool primitive() override {
        return true;
    }

    Value * copy() override {
        return new CharValue(value);
    }

#ifdef COMPILER_BUILD
    llvm::Type * llvm_type(Codegen &gen) override {
        return gen.builder->getInt8Ty();
    }

    llvm::Value * llvm_value(Codegen &gen) override {
        return gen.builder->getInt8((int) value);
    }
#endif

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

    void * get_value() override {
        return &value;
    }

private:
    char value; ///< The character value.
};