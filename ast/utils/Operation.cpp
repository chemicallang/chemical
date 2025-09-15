// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "Operation.h"

chem::string_view to_string(Operation operation) {
    switch (operation) {
        case Operation::Grouping:
            return "()";
        case Operation::ScopeResolutionUnary:
            return "::";
        case Operation::ScopeResolutionBinary:
            return "::";
        case Operation::FunctionCall:
            return "()";
        case Operation::Subscript:
            return "[]";
        case Operation::StructureMember:
            return ".";
        case Operation::StructurePointerMember:
            return "->";
        case Operation::PostfixIncrement:
            return "++";
        case Operation::PostfixDecrement:
            return "--";
        case Operation::LogicalNegate:
            return "!";
        case Operation::OnesComplement:
            return "~";
        case Operation::UnaryPlus:
            return "+";
        case Operation::UnaryMinus:
            return "-";
        case Operation::PrefixIncrement:
            return "++";
        case Operation::PrefixDecrement:
            return "--";
        case Operation::Indirection:
            return "*";
        case Operation::AddressOf:
            return "&";
        case Operation::Sizeof:
            return "sizeof";
        case Operation::TypeConversion:
            return "(type)";
        case Operation::Multiplication:
            return "*";
        case Operation::Division:
            return "/";
        case Operation::Modulus:
            return "%";
        case Operation::Addition:
            return "+";
        case Operation::Subtraction:
            return "-";
        case Operation::LeftShift:
            return "<<";
        case Operation::RightShift:
            return ">>";
        case Operation::GreaterThan:
            return ">";
        case Operation::GreaterThanOrEqual:
            return ">=";
        case Operation::LessThan:
            return "<";
        case Operation::LessThanOrEqual:
            return "<=";
        case Operation::IsEqual:
            return "==";
        case Operation::IsNotEqual:
            return "!=";
        case Operation::BitwiseAND:
            return "&";
        case Operation::BitwiseXOR:
            return "^";
        case Operation::BitwiseOR:
            return "|";
        case Operation::LogicalAND:
            return "&&";
        case Operation::LogicalOR:
            return "||";
        case Operation::Conditional:
            return "?:";
        case Operation::Assignment:
            return "=";
        case Operation::AddTo:
            return "+=";
        case Operation::SubtractFrom:
            return "-=";
        case Operation::MultiplyBy:
            return "*=";
        case Operation::DivideBy:
            return "/=";
        case Operation::ModuloBy:
            return "%=";
        case Operation::ShiftLeftBy:
            return "<<=";
        case Operation::ShiftRightBy:
            return ">>=";
        case Operation::ANDWith:
            return "&=";
        case Operation::ExclusiveORWith:
            return "^=";
        case Operation::InclusiveORWith:
            return "|=";
        default:
            return "";
    }
}


uint8_t to_precedence(Operation op) {
    switch(op) {
        case Operation::Grouping:
        case Operation::ScopeResolutionUnary:
        case Operation::ScopeResolutionBinary:
            return 16;

        // Function call, subscript, structure member, structure pointer member
        case Operation::FunctionCall:
        case Operation::Subscript:
        case Operation::StructureMember:
        case Operation::StructurePointerMember:
            return 15;

        // Postfix increment and decrement
        case Operation::PostfixIncrement:
        case Operation::PostfixDecrement:
            return 14;

        // Unary operators
        case Operation::LogicalNegate:
        case Operation::OnesComplement:
        case Operation::UnaryPlus:
        case Operation::UnaryMinus:
        case Operation::PrefixIncrement:
        case Operation::PrefixDecrement:
        case Operation::Indirection:
        case Operation::AddressOf:
        case Operation::Sizeof:
        case Operation::TypeConversion:
            return 13;

        // Multiplicative operators
        case Operation::Division:
        case Operation::Multiplication:
        case Operation::Modulus:
            return 12;

        // Additive operators
        case Operation::Addition:
        case Operation::Subtraction:
            return 11;

        // Shift operators
        case Operation::LeftShift:
        case Operation::RightShift:
            return 10;

        // Relational operators
        case Operation::GreaterThan:
        case Operation::GreaterThanOrEqual:
        case Operation::LessThan:
        case Operation::LessThanOrEqual:
            return 9;

        // Equality operators
        case Operation::IsEqual:
        case Operation::IsNotEqual:
            return 8;

        // Bitwise AND
        case Operation::BitwiseAND:
            return 7;

        // Bitwise exclusive OR
        case Operation::BitwiseXOR:
            return 6;

        // Bitwise inclusive OR
        case Operation::BitwiseOR:
            return 5;

        // Logical AND
        case Operation::LogicalAND:
            return 4;

        // Logical OR
        case Operation::LogicalOR:
            return 3;

        // Conditional operator
        case Operation::Conditional:
            return 2;

        // Assignment operators
        case Operation::Assignment:
        case Operation::AddTo:
        case Operation::SubtractFrom:
        case Operation::MultiplyBy:
        case Operation::DivideBy:
        case Operation::ModuloBy:
        case Operation::ShiftLeftBy:
        case Operation::ShiftRightBy:
        case Operation::ANDWith:
        case Operation::ExclusiveORWith:
        case Operation::InclusiveORWith:
            return 1;
    }
}

bool is_assoc_left_to_right(Operation op) {
    switch (op) {
        // Unary operators (right-to-left associativity)
        case Operation::LogicalNegate:
        case Operation::OnesComplement:
        case Operation::UnaryPlus:
        case Operation::UnaryMinus:
        case Operation::PrefixIncrement:
        case Operation::PrefixDecrement:
        case Operation::Indirection:
        case Operation::AddressOf:
        case Operation::Sizeof:
        case Operation::TypeConversion:
            return false;

            // Assignment operators (right-to-left associativity)
        case Operation::Assignment:
        case Operation::AddTo:
        case Operation::SubtractFrom:
        case Operation::MultiplyBy:
        case Operation::DivideBy:
        case Operation::ModuloBy:
        case Operation::ShiftLeftBy:
        case Operation::ShiftRightBy:
        case Operation::ANDWith:
        case Operation::ExclusiveORWith:
        case Operation::InclusiveORWith:
            return false;

            // All other binary operators (left-to-right associativity)
        default:
            return true;
    }
}

OperatorImplInformation operator_impl_info(Operation op) {
    switch(op) {
        case Operation::Addition:
            return { .name = "add" };
        case Operation::Subtraction:
            return { .name = "sub" };
        case Operation::Multiplication:
            return { .name = "mul" };
        case Operation::Division:
            return { .name = "div" };
        case Operation::Modulus:
            return { .name = "rem" };
        case Operation::BitwiseAND:
            return { .name = "bitand" };
        case Operation::BitwiseOR:
            return { .name = "bitor" };
        case Operation::BitwiseXOR:
            return { .name = "bitxor" };
        case Operation::LeftShift:
            return { .name = "shl" };
        case Operation::RightShift:
            return { .name = "shr" };
        case Operation::IsEqual:
            return { .name = "eq" };
        case Operation::IsNotEqual:
            return { .name = "ne" };
        case Operation::GreaterThan:
            return { .name = "gt" };
        case Operation::LessThan:
            return { .name = "lt" };
        case Operation::GreaterThanOrEqual:
            return { .name = "gte" };
        case Operation::LessThanOrEqual:
            return { .name = "lte" };
        default:
            return { .name = "" };
    }
}