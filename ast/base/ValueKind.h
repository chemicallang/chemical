// Copyright (c) Qinetik 2024.

#pragma once

#include <cstdint>

/**
 * enum class representing kind of values
 */
enum class ValueKind : uint8_t {

    // integer types
    // signed integer types
    Char,
    Short,
    Int,
    Long,
    BigInt,
    Int128,
    // unsigned integer types
    UChar,
    UShort,
    UInt,
    ULong,
    UBigInt,
    UInt128,
    // unsigned integers end
    NumberValue,
    // integer number values end here

    Float,
    Double,
    Bool,
    String,
    Expression,
    ArrayValue,
    StructValue,
    UnnamedStructValue,
    LambdaFunc,
    IfValue,
    SwitchValue,
    LoopValue,
    NewTypedValue,
    NewValue,
    PlacementNewValue,
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
    SymResValue,
    AlignOfValue,
    VariantCall,
    VariantCase,
    AddrOfValue,
    WrapValue,

    SignedIntNStart = Char,
    SignedIntNEnd = Int128,
    UnsignedIntNStart = UChar,
    UnsignedIntNEnd = UInt128,
    IntNStart = Char,
    IntNEnd = NumberValue,

};