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

    std::string representation() const override {
        return value;
    }

private:
    std::string value; ///< The string value.
};