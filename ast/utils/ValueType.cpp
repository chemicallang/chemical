// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#include "ValueType.h"

std::string to_string(ValueType type) {
    switch (type) {
        case ValueType::Int:
            return "int";
        case ValueType::Float:
            return "float";
        case ValueType::Bool:
            return "bool";
        case ValueType::Char:
            return "char";
        case ValueType::String:
            return "string";
        case ValueType::Unknown:
            return "unknown";
        default:
            return "[to_string:NotInFunction]";
    }
}