// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Enum class representing values contained in an expression.
 */
enum class ValueType : uint8_t {

    // integer number values start here
    Int,
    UInt,
    Char,
    UChar,
    Short,
    UShort,
    Long,
    ULong,
    BigInt,
    UBigInt,
    Int128,
    UInt128,
    // integer number values end here

    Float,
    Double,
    Bool,
    String,
    Expression,
    Array,
    Struct,
    Union,
    Vector,
    Lambda,
    Pointer,
    Unknown,

    IntNStart = Int,
    IntNEnd = UInt128,

};

std::string to_string(ValueType type);