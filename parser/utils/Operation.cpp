// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 01/03/2024.
//

#include "Operation.h"

std::string to_string(Operation operation) {
    switch (operation) {
        case Operation::Addition:
            return "+";
        case Operation::Subtraction:
            return "-";
        case Operation::Multiplication:
            return "*";
        case Operation::Division:
            return "/";
        case Operation::Modulus:
            return "%";
        case Operation::Equal:
            return "==";
        case Operation::NotEqual:
            return "!=";
        case Operation::LessThan:
            return "<";
        case Operation::LessThanOrEqual:
            return "<=";
        case Operation::GreaterThan:
            return ">";
        case Operation::GreaterThanOrEqual:
            return ">=";
        case Operation::LeftShift:
            return "<<";
        case Operation::RightShift:
            return ">>";
        case Operation::And:
            return "&";
        case Operation::Or:
            return "|";
        case Operation::Xor:
            return "^";
        default:
            return "[MissingInFunction(toString#Operation)]";
    }
}