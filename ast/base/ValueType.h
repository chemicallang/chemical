// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <cstdint>

/**
 * @brief Enum class representing values contained in an expression.
 */
enum class ValueType : uint8_t {

    Int,
    Float,
    Double,
    Bool,
    Char,
    String,
    Unknown

};