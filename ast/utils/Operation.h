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


    IsEqual,
    IsNotEqual,
    LessThan,
    LessThanOrEqual,
    GreaterThan,
    GreaterThanOrEqual,

    LeftShift,
    RightShift,

    Equal,

    Increment,
    Decrement,

    And,
    Or,

    // Xor should always be the last Operation because XOR's index (uint8_t) is used as length of this enum
    Xor

};

std::string to_string(Operation operation);