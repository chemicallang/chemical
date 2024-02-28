// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/NumberToken.h"

std::optional<IntValue> Parser::parseIntNode() {
    if (tokens[position]->type() == LexTokenType::Number) {
        auto number = consume<NumberToken>();
        try {
            return std::stoi(number->value);
        } catch (...) {
            error("invalid integer token", number->position);
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
}

std::optional<Value> Parser::parseValueNode() {
    return parseIntNode();
}