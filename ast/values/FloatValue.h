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

private:
    float value; ///< The floating-point value.
};