// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 24/02/2024.
//

#include "lexer/Lexer.h"

bool Lexer::lexCharToken() {
    if (provider.increment('\'')) {
        auto back = backPosition(1);
        std::string value = "'";
        provider.readEscaping(value, '\'');
        emplace(LexTokenType::Char, back, std::move(value));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexStringToken() {
    if (provider.increment('"')) {
        auto back = backPosition(1);
        std::string value = "\"";
        provider.readEscaping(value, '"');
        emplace(LexTokenType::String, back, std::move(value));
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexBoolToken() {
    if (provider.increment("true")) {
        emplace(LexTokenType::Bool, backPosition(4), "true");
        return true;
    } else if (provider.increment("false")) {
        emplace(LexTokenType::Bool, backPosition(5), "false");
        return true;
    }
    return false;
}

bool Lexer::lexNull() {
    if (provider.increment("null")) {
        emplace(LexTokenType::Null, backPosition(4), "null");
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexValueToken() {
    return lexCharToken() || lexStringToken() || lexLambdaValue() || lexNumberToken() || lexBoolToken() ||
           lexNull();
}

bool Lexer::lexAccessChainValueToken() {
    return lexCharToken() || lexStringToken() || lexLambdaValue() || lexNumberToken();
}

bool Lexer::lexSwitchCaseValue() {
    return lexCharToken() || lexStringToken() || lexNumberToken() || lexBoolToken() ||
           lexNull() || lexAccessChainOrAddrOf() || lexAnnotationMacro();
}

bool Lexer::lexArrayInit() {
    if (lexOperatorToken('{')) {
        auto start = tokens_size() - 1;
        do {
            lexWhitespaceAndNewLines();
            if (!(lexExpressionTokens(true) || lexArrayInit())) {
                break;
            }
            lexWhitespaceAndNewLines();
        } while (lexOperatorToken(','));
        if (!lexOperatorToken('}')) {
            error("expected a '}' when lexing an array");
        }
        lexWhitespaceToken();
        if (lexTypeTokens()) {
            lexWhitespaceToken();
            if (lexOperatorToken('(')) {
                do {
                    lexWhitespaceToken();
                    if (!lexNumberToken()) {
                        break;
                    }
                    lexWhitespaceToken();
                } while (lexOperatorToken(','));
                lexWhitespaceToken();
                if (!lexOperatorToken(')')) {
                    error("expected a ')' when ending array size");
                }
            }
        }
        compound_from(start, LexTokenType::CompArrayValue);
        return true;
    } else {
        return false;
    }
}

bool Lexer::lexAccessChainOrValue(bool lexStruct) {
    return lexAccessChainValueToken() || lexAccessChainOrAddrOf(lexStruct) || lexAnnotationMacro();
}