// Copyright (c) Chemical Language Foundation 2025.

#include "Assignment.h"

chem::string_view AssignStatement::overload_op_name(Operation op) {
    switch(op) {
        case Operation::Addition:
            return "add_assign";
        case Operation::Subtraction:
            return "sub_assign";
        case Operation::Multiplication:
            return "mul_assign";
        case Operation::Division:
            return "div_assign";
        case Operation::Modulus:
            return "rem_assign";
        case Operation::BitwiseAND:
            return "bitand_assign";
        case Operation::BitwiseOR:
            return "bitor_assign";
        case Operation::BitwiseXOR:
            return "bitxor_assign";
        case Operation::LeftShift:
            return "shl_assign";
        case Operation::RightShift:
            return "shr_assign";
        default:
            return "";
    }
}