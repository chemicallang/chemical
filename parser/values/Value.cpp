// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/NumberToken.h"
#include "ast/values/CharValue.h"

lex_ptr<IntValue> Parser::parseIntNode() {
    if (tokens[position]->type() == LexTokenType::Number) {
        auto number = consume<NumberToken>();
        try {
            return std::make_unique<IntValue>(std::stoi(number->value));
        } catch (...) {
            error("invalid integer token");
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }
}

lex_ptr<Value> Parser::parseValue() {
    auto charToken = consume_char_token();
    if(charToken.has_value()) {
        return std::make_unique<CharValue>(charToken.value());
    }
    return parseIntNode();
}

lex_ptr<Value> Parser::parseAccessChainOrValue() {
    auto value = parseValue();
    if (value.has_value()) {
        return value;
    }
    auto chain = parseAccessChain();
    if (chain.has_value()) {
        if(chain.value()->values.size() == 1) {
            return std::move(chain.value()->values[0]);
        }
        return chain;
    }
    return std::nullopt;
}