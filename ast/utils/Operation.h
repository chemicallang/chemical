// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 27/02/2024.
//

#pragma once

#include <cstdint>
#include <optional>

/**
 * @brief Enum class representing operations between two expressions.
 */
enum class Operation : uint8_t {
    // Grouping and scope resolution operators
    Grouping = 1,
    ScopeResolutionUnary,
    ScopeResolutionBinary,

    // Function call, subscript, structure member, structure pointer member
    FunctionCall,
    Subscript,
    StructureMember,
    StructurePointerMember,

    // Postfix increment and decrement
    PostfixIncrement,
    PostfixDecrement,

    // Unary operators
    LogicalNegate,
    OnesComplement,
    UnaryPlus,
    UnaryMinus,
    PrefixIncrement,
    PrefixDecrement,
    Indirection,
    AddressOf,
    Sizeof,
    TypeConversion,

    // Multiplicative operators
    Multiplication,
    Division,
    Modulus,

    // Additive operators
    Addition,
    Subtraction,

    // Shift operators
    LeftShift,
    RightShift,

    // Relational operators
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual,

    // Equality operators
    IsEqual,
    IsNotEqual,

    // Bitwise AND
    BitwiseAND,

    // Bitwise exclusive OR
    BitwiseXOR,

    // Bitwise inclusive OR
    BitwiseOR,

    // Logical AND
    LogicalAND,

    // Logical OR
    LogicalOR,

    // Conditional operator
    Conditional,

    // Assignment operators
    Assignment,
    AddTo,
    SubtractFrom,
    MultiplyBy,
    DivideBy,
    ModuloBy,
    ShiftLeftBy,
    ShiftRightBy,
    ANDWith,
    ExclusiveORWith,
    InclusiveORWith
};

std::string to_string(Operation operation);

/**
 * This tells whether the associativity of the given operator is left to right
 * @param op
 * @return true if the associativity is LTR
 */
bool is_assoc_left_to_right(Operation op);