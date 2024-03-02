// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 28/02/2024.
//

#include "parser/Parser.h"
#include "lexer/model/tokens/NumberToken.h"
#include "ast/values/CharValue.h"
#include "ast/values/StringValue.h"

lex_ptr<ArrayValue> Parser::parseArrayValue() {
    if(consume_op('[')) {
        std::vector<std::unique_ptr<Value>> params;
        do {
            auto param = parseExpression();
            if(param.has_value()) {
                params.emplace_back(std::move(param.value()));
            } else {
                break;
            }
        } while(consume_op(','));
        if(consume_op(']')) {
            return std::make_unique<ArrayValue>(std::move(params));
        } else {
            error("expected a ']' after the array declaration");
        }
    }
    return std::nullopt;
}

lex_ptr<IntValue> Parser::parseIntValue() {
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

lex_ptr<BoolValue> Parser::parseBoolValue() {
    if(consume("true")) {
        return std::make_unique<BoolValue>(true);
    } else if(consume("false")){
        return std::make_unique<BoolValue>(false);
    }
    return std::nullopt;
}

lex_ptr<Value> Parser::parseValue() {
    auto strToken = consume_str_token();
    if(strToken.has_value()) {
        return std::make_unique<StringValue>(strToken.value());
    }
    auto charToken = consume_char_token();
    if(charToken.has_value()) {
        return std::make_unique<CharValue>(charToken.value());
    }
    auto boolVal = parseBoolValue();
    if(boolVal.has_value()) {
        return boolVal;
    }
    auto arrVal = parseArrayValue();
    if(arrVal.has_value()) {
        return arrVal;
    }
    return parseIntValue();
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