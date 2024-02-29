// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/NumberToken.h"

std::optional<std::unique_ptr<IntValue>> Parser::parseIntNode() {
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

std::optional<std::unique_ptr<Value>> Parser::parseValueNode() {
    return parseIntNode();
}

std::optional<std::unique_ptr<Value>> Parser::parseAccessChainOrValue() {
    auto value = parseValueNode();
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
}