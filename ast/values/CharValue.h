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

    std::string representation() const override {
        std::string rep;
        rep.append(1, '\'');
        rep.append(1, value);
        rep.append(1, '\'');
        return rep;
    }

private:
    char value; ///< The character value.
};