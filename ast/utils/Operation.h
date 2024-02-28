// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <cstdint>

/**
 * @brief Enum class representing operations between two expressions.
 */
enum class Operation : uint8_t {
    Addition,
    Subtraction,
    Multiplication,
    Division,
    Modulus,
    Equal,
    NotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,
    And,
    Or
};