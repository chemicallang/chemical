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
    Struct,
    Union,

    // bool, char and uchar are llvm / c intN types, however
    // we do not make them intN because they must not satisfy each other or any other type
    Bool,

    Double,
    Float,
    LongDouble,

    Complex,

    Float128,
    Function,
    Generic,
    IntN,
    Pointer,
    Reference,
    Linked,
    String,
    Literal,
    Dynamic,
    Void,
    Unknown

};