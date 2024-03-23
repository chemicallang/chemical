// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "TypeUtils.h"

std::string toTypeString(LexTokenType token) {
    switch (token) {
        case LexTokenType::CharOperator:
            return "LexTokenType::CharOperator";
        case LexTokenType::Char:
            return "LexTokenType::Char";
        case LexTokenType::Class:
            return "LexTokenType::Class";
        case LexTokenType::Comment:
            return "LexTokenType::Comment";
        case LexTokenType::EnumMember:
            return "LexTokenType::EnumMember";
        case LexTokenType::Enum:
            return "LexTokenType::Enum";
        case LexTokenType::Function:
            return "LexTokenType::Function";
        case LexTokenType::Interface:
            return "LexTokenType::Interface";
        case LexTokenType::Keyword:
            return "LexTokenType::Keyword";
        case LexTokenType::Method:
            return "LexTokenType::Method";
        case LexTokenType::Modifier:
            return "LexTokenType::Modifier";
        case LexTokenType::MultilineComment:
            return "LexTokenType::MultilineComment";
        case LexTokenType::Number:
            return "LexTokenType::Number";
        case LexTokenType::Parameter:
            return "LexTokenType::Parameter";
        case LexTokenType::Property:
            return "LexTokenType::Property";
        case LexTokenType::StringOperator:
            return "LexTokenType::StringOperator";
        case LexTokenType::String:
            return "LexTokenType::String";
        case LexTokenType::Struct:
            return "LexTokenType::Struct";
        case LexTokenType::Type:
            return "LexTokenType::Type";
        case LexTokenType::Variable:
            return "LexTokenType::Variable";
        case LexTokenType::RawToken:
            return "LexTokenType::Raw";
        default:
            return "Undocumented token in TypeUtils";
    }
}