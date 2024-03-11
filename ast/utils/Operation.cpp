// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include <string>
#include "Operation.h"

std::string to_string(Operation operation) {
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
            return "[to_string::UnknownOperation]"; // Handle unknown operation
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