// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

/**
 * enum class representing kind of values
 */
enum class ValueKind : uint8_t {

    IntN,
    Float,
    Double,
    Bool,
    String,
    Expression,
    ArrayValue,
    StructValue,
    LambdaFunc,
    IfValue,
    SwitchValue,
    LoopValue,
    NewTypedValue,
    NewValue,
    PlacementNewValue,
    IncDecValue,
    IsValue,
    InValue,
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
    UnsafeValue,
    ComptimeValue,
    AlignOfValue,
    VariantCase,
    AddrOfValue,
    PointerValue,
    BlockValue,
    TypeInsideValue,
    PatternMatchExpr,
    ExtractionValue,
    WrapValue,

    DestructValue,
    EmbeddedValue,

    ExpressiveString,

};