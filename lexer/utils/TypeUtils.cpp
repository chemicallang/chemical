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
        case LexTokenType::Comment:
            return "LexTokenType::Comment";
        case LexTokenType::Keyword:
            return "LexTokenType::Keyword";
        case LexTokenType::MultilineComment:
            return "LexTokenType::MultilineComment";
        case LexTokenType::Number:
            return "LexTokenType::Number";
        case LexTokenType::StringOperator:
            return "LexTokenType::StringOperator";
        case LexTokenType::String:
            return "LexTokenType::String";
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