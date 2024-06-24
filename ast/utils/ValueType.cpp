// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 05/03/2024.
//

#include "ast/base/ValueType.h"

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
        case ValueType::UInt:
            return "uint";
        case ValueType::Short:
            return "short";
        case ValueType::UShort:
            return "ushort";
        case ValueType::Long:
            return "long";
        case ValueType::ULong:
            return "ulong";
        case ValueType::BigInt:
            return "bigint";
        case ValueType::UBigInt:
            return "ubigint";
        case ValueType::Int128:
            return "int128";
        case ValueType::UInt128:
            return "uint128";
        case ValueType::Double:
            return "double";
        case ValueType::Expression:
            return "[expression]";
        case ValueType::Array:
            return "[array]";
        case ValueType::Struct:
            return "[struct]";
        case ValueType::Vector:
            return "[vector]";
        case ValueType::Lambda:
            return "[lambda]";
        case ValueType::Pointer:
            return "[pointer]";
        default:
            return "[to_string:NotInFunction]";
    }
}