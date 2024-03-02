// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 02/03/2024.
//

#pragma once

#include <utility>

#include "ast/base/Value.h"

/**
 * @brief Class representing a VariableIdentifier.
 */
class VariableIdentifier : public Value {
public:
    /**
     * @brief Construct a new VariableIdentifier object.
     *
     * @param value The string value.
     */
    VariableIdentifier(std::string  value) : value(std::move(value)) {}

    std::string representation() const override {
        return value;
    }

private:
    std::string value; ///< The string value.
};