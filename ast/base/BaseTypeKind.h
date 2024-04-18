// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <cstdint>

/**
 * @brief Enum class representing values contained in an expression.
 */
enum class BaseTypeKind : uint8_t {

    Any,
    Array,
    Bool,
    Char,
    Double,
    Float,
    Function,
    Generic,
    Int32,
    Pointer,
    Referenced,
    String,
    Void,
    Unknown
};