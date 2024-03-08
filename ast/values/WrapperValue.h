// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 06/03/2024.
//

#pragma once

#include "ast/base/Value.h"

/**
 * A wrapper value holds a value and delegates to it
 */
class WrapperValue : public Value {
public:

    WrapperValue(Value* value) : value(value) {

    }

    std::string representation() const override {
        return value->representation();
    }

    std::string interpret_representation() const override {
        return value->interpret_representation();
    }

    Value * evaluated_value(InterpretScope& scope) override {
        return value->evaluated_value(scope);
    }

    int as_int() override {
        return value->as_int();
    }

    bool as_bool() override {
        return value->as_bool();
    }

    ValueType value_type() const override {
        return value->value_type();
    }

    ~WrapperValue() {
        delete value;
    }

protected:
    Value* value;

};