// Copyright (c) Qinetik 2024.

#pragma once

#include <cstdint>

/**
 * enum class representing kind of values
 */
enum class ValueKind : uint8_t {

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
    ArrayValue,
    StructValue,
//    Union,
//    Vector,
    LambdaFunc,
    Pointer,
    IfValue,
    SwitchValue,
    NumberValue,
    IsValue,
    DereferenceValue,
    RetStructParamValue,
    AccessChain,
    CastedValue,
    Identifier,
    IndexOperator,
    FunctionCall,
    NegativeValue,
    NotValue,
    NullValue,
    SizeOfValue,
    VariantCall,
    VariantCase,
    AddrOfValue,
    WrapValue,

    IntNStart = Int,
    IntNEnd = UInt128,

};