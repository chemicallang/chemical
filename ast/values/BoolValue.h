// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#pragma once

#include "ast/base/Value.h"

class BoolValue : public Value {
public:

    /**
     * @brief Construct a new CharValue object.
     *
     * @param value The character value.
     */
    BoolValue(bool value) : value(value) {}

    bool primitive() override {
        return true;
    }

    Value * copy() override {
        return new BoolValue(value);
    }

    std::string representation() const override {
        std::string rep;
        if(value) {
            rep.append("true");
        } else {
            rep.append("false");
        }
        return rep;
    }

    bool as_bool() override {
        return value;
    }

    void * get_value() override {
        return &value;
    }

private:
    bool value;
};